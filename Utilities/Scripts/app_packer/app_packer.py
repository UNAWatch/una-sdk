"""
@file   uapp_packer.py

@author Denys Saienko <denys.saienko@droid-technologies.com>

@date   24-10-2024

@brief  This Python script converts an ELF file and a 60x60 PNG icon into
        a custom binary format with the .uapp extension, which stands for
        "UNA Application."
        The binary contains necessary application data, including relocatable
        sections and a compact icon format, while performing CRC checks to
        ensure integrity. The core purpose of the script is to generate a
        self-contained UAPP package for deployment on a target device, such
        as an embedded system.

@details The idea and part of the implementation for dynamic loading
        are borrowed from the following project:
        https://github.com/rgujju/Dynamic_App_Loading/tree/freertos


@copyright
        This code contains elements adapted from the original project
        available at the link above. The original code is subject to the
        license specified in that repository (MIT License or another
        relevant license). You must comply with the terms of that license
        when using this code.

@usage
        python3 uapp_packager.py -e app.elf -i app_icon.png -o Output -v 1.0

"""

import argparse
import logging
import re
import os
import struct
import zlib
from pathlib import Path
from elftools.elf.elffile import ELFFile
from elftools import elf
from PIL import Image


def is_loadable_segment(segment):
    return segment['p_type'] == 'PT_LOAD'


def get_relocatable_sections(elffile):
    relocatable_sections = []
    for section in elffile.iter_sections():
        if section['sh_type'] in ('SHT_REL', 'SHT_RELA'):
            # logging.info(f"Section: {section.name} type {section['sh_type']} size {section.data_size}")
            relocatable_sections.append(section)
    return relocatable_sections


def get_sections_in_loadable_segments(elffile):
    sections_in_loadable_segments = []
    for segment in elffile.iter_segments():
        if is_loadable_segment(segment):
            for section in elffile.iter_sections():
                if segment.section_in_segment(section):
                    sections_in_loadable_segments.append(section)
    return sections_in_loadable_segments


def get_unlisted_nonzero_sections(section_names, loadable_sections):
    unlisted_sections = []

    for section in loadable_sections:
        section_name = section.name
        section_size = section['sh_size']

        if section_size > 0 and section_name not in section_names:
            unlisted_sections.append(section)

    return unlisted_sections


def filter_sections_for_relocation(elffile):
    # Get all the sections included in the segments for download
    loadable_sections = get_sections_in_loadable_segments(elffile)
    # Get all the sections that need to be relocated
    relocatable_sections = get_relocatable_sections(elffile)

    # create a set of downloadable section names for convenience
    loadable_section_names = {section.name for section in loadable_sections}

    # filter the relocated sections, leaving only those with corresponding bootable sections
    sections_for_relocation = [
        section for section in relocatable_sections
        if section.name.replace('.rel', '') in loadable_section_names
    ]

    return sections_for_relocation


def print_section_data(name, data):
    words = [int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4)]
    hex_lines = [f"0x{w:08X}" for w in words]
    line = f"Content of section {name}:"
    logging.debug(line)
    for i in range(0, len(hex_lines), 4):
        logging.debug(" ".join(hex_lines[i:i+4]))

def get_relocated_list(elffile):
    rel_sections = filter_sections_for_relocation(elffile)
    rel_list = bytearray()
    for section in rel_sections:
        if section.name in ['.rel.preinit_array', '.rel.init_array', '.rel.fini_array']:
            logging.debug(f'REL Section: {section.name} size {section.data_size} - skip')
            continue
        logging.debug(f'REL Section: {section.name} size {section.data_size}')

        # print_section_data(section.name, section.data())

        symtab = elffile.get_section_by_name('.symtab')

        for reloc in section.iter_relocations():
            reloc_type = next(key for key, val in elf.enums.ENUM_RELOC_TYPE_ARM.items() if val == reloc["r_info_type"])
            address = reloc["r_offset"]

            symbol = symtab.get_symbol(reloc['r_info_sym'])
            value = symbol.entry["st_value"]
            symtype = symbol.entry["st_info"]["type"]
            symname = symbol.name

            if reloc["r_info_type"] == elf.enums.ENUM_RELOC_TYPE_ARM['R_ARM_ABS32']:  # 2
                rel_list.extend(struct.pack('<I', address))
                # logging.debug(f'0x{address:08x} = 0x{value:08x}, {reloc_type}, {symtype}, {symname}')

    return rel_list


def calc_crc(input):
    return zlib.crc32(input)


def parse_firmware_version(version_string):
    # Format: <major>.<minor>
    match = re.match(r'(\d+)\.(\d+)', version_string)
    if match:
        major, minor = match.groups()
        return int(major), int(minor)
    else:
        raise ValueError('Incorrect version format')

def compute_section_pad(section_sizes, section_addresses, section1, section2):
    try:
        return int((section_addresses[section2] - section_addresses[section1]) / 4 - section_sizes[section1])
    except KeyError as e:
        raise ValueError(f"Missing section: {e}")

def prepare_section(elffile, tin, section_name, section_sizes, section_pads):
    pad_words = section_pads[section_name]
    logging.debug(f"loading section {section_name:<16} size {section_sizes[section_name]:6} pads {pad_words:6}")
    
    if section_sizes[section_name] > 0:
        data = elffile.get_section_by_name(section_name).data()
        tin += data

        # Disable call for some sections
        if section_name not in ['.text', '.data']:
            print_section_data(section_name, data)

    if pad_words > 0:
        pattern = b'\xA5\xA5\xA5\xA5'  # 0xA5A5A5A5 in bytes
        pad_bytes = pattern * pad_words
        tin += pad_bytes


def convert_file(elf_path, app_name, major_version, minor_version):
    """
    The main function that assembles the UAPP binary.
    """
    # Check name
    if len(app_name) <= 15:
        name = app_name.encode('utf-8') + b'\0' * (16 - len(app_name))
    else:
        logging.error('App name is greater than 15 characters.')
        return None

    # Since version fields are 8 bits log, value of it cannot be greater than 255
    if major_version > 255 or minor_version > 255:
        logging.error('Version cannot be greater than 255')
        return None

    # Extract section details
    with open(elf_path, 'rb') as f:
        elffile = ELFFile(f)

        # for section in elffile.iter_sections():
        #     logging.info(f"All Section: {section.name} size {section.data_size}")

        section_names = ['.text', '.preinit_array', '.init_array', '.fini_array', '.plt', '.data', '.got', '.bss',
                         '.stack']
        section_sizes = {}
        section_addresses = {}
        section_pads = {}

        for section_name in section_names:
            section = elffile.get_section_by_name(section_name)
            if section:
                # sections must be aligned in the linker script
                if section['sh_size'] % 4:
                    logging.error(f'ELF section {section_name} size not multiple by 4')
                    return None
                section_sizes[section_name] = section['sh_size'] // 4
                section_addresses[section_name] = section['sh_addr']
                logging.debug(f"{section_name:<16} 0x{section_addresses[section_name]:08X} {section_sizes[section_name]*4:6}")
            else:
                logging.warning(f'ELF file has no {section_name} section')
                section_sizes[section_name] = 0



        for i in range(len(section_names) - 1):
            curr_name = section_names[i]
            next_name = section_names[i + 1]
        
            section_pads[curr_name] = compute_section_pad(section_sizes,
                                                          section_addresses,
                                                          curr_name,
                                                          next_name)
        
            logging.debug(f"pad : {curr_name:<16} 0x{section_addresses[curr_name]:08X} {section_pads[curr_name]:6}")




        # Get all the sections included in the segments for download
        loadable_sections = get_sections_in_loadable_segments(elffile)
        # for section in loadable_sections:
        #     logging.warning(f"loadable_sections: {section.name} size {section.data_size}")

        unlisted_nonzero_sections = get_unlisted_nonzero_sections(section_names, loadable_sections)
        if unlisted_nonzero_sections:
            for section in unlisted_nonzero_sections:
                logging.warning(f"ELF file has unhandled sections: {section.name} size {section.data_size}")

        # Get a list of absolute addresses that require relocation from sections .rel.text, .rel.data and similar.
        # Note: sections .rel.preinit_array, .rel.init_array and .rel.fini_array are excluded from the list,
        # since the modification of addresses occurs directly in sections .preinit_array, .init_array and .fini_array
        # when loading the code.
        rel_addr_list = get_relocated_list(elffile)

        # Create UAPP header
        header = struct.pack(
            '<4s4B4B2I8H10I',
            b'UAPP',  # Always 'UAPP' to identify App
            0,  # Version of this structure
            0,  # reserved 1b
            0,  # reserved 1b
            0,  # reserved 1b
            major_version,  # Major version of App
            minor_version,  # Minor version of App
            0,  # reserved 1b
            0,  # reserved 1b
            0,  # reserved 4b
            # offset 32b
            len(rel_addr_list) // 4,  # Size of list of addresses for relocation
            section_pads['.text'],
            section_pads['.preinit_array'],
            section_pads['.init_array'],
            section_pads['.fini_array'],
            section_pads['.plt'],
            section_pads['.data'],
            section_pads['.got'],
            section_pads['.bss'],
            section_sizes['.text'],
            section_sizes['.preinit_array'],
            section_sizes['.init_array'],
            section_sizes['.fini_array'],
            section_sizes['.plt'],
            section_sizes['.data'],
            section_sizes['.got'],
            section_sizes['.bss'],
            section_sizes['.stack'],
            0,  # reserved 4b
            # offset 80b
        )

        file_size = (len(header) + (len(rel_addr_list) // 4) +
                     section_sizes['.text']       + section_pads['.text'] +
                     section_sizes['.init_array'] + section_pads['.init_array'] +
                     section_sizes['.fini_array'] + section_pads['.fini_array'] +
                     section_sizes['.plt']        + section_pads['.plt'] +
                     section_sizes['.data']       + section_pads['.data'] +
                     section_sizes['.got'])

        ram_size = (section_sizes['.text']       + section_pads['.text'] +
                    section_sizes['.init_array'] + section_pads['.init_array'] +
                    section_sizes['.fini_array'] + section_pads['.fini_array'] +
                    section_sizes['.plt']        + section_pads['.plt'] +
                    section_sizes['.data']       + section_pads['.data'] +
                    section_sizes['.got']        + section_pads['.got'] +
                    section_sizes['.stack'])

        logging.debug(f"File size:  {4 * file_size} bytes, RAM size {4 * ram_size} bytes")

        logging.debug(f"rel_addr_list size:  {len(rel_addr_list) // 4}")
        logging.debug(f".text size:          {section_sizes['.text']}")
        logging.debug(f".preinit_array size: {section_sizes['.preinit_array']}")
        logging.debug(f".init_array size:    {section_sizes['.init_array']}")
        logging.debug(f".fini_array size:    {section_sizes['.fini_array']}")
        logging.debug(f".plt size:           {section_sizes['.plt']}")
        logging.debug(f".data size:          {section_sizes['.data']}")
        logging.debug(f".got size:           {section_sizes['.got']}")
        logging.debug(f".bss size:           {section_sizes['.bss']}")
        logging.debug(f".stack size:         {section_sizes['.stack']}")

        tin = bytearray()

        # Add the header and sections to the UAPP file
        tin += header

        # Relocation table
        if len(rel_addr_list) > 0:
            tin += rel_addr_list
            print_section_data('rel_addr_list', rel_addr_list)

        prepare_section(elffile, tin, '.text',          section_sizes, section_pads)
        prepare_section(elffile, tin, '.preinit_array', section_sizes, section_pads)
        prepare_section(elffile, tin, '.init_array',    section_sizes, section_pads)
        prepare_section(elffile, tin, '.fini_array',    section_sizes, section_pads)
        prepare_section(elffile, tin, '.plt',           section_sizes, section_pads)
        prepare_section(elffile, tin, '.data',          section_sizes, section_pads)
        prepare_section(elffile, tin, '.got',           section_sizes, section_pads)

        # Add CRC
        crc = calc_crc(tin)
        crc_data = struct.pack('=L', crc)
        tin += crc_data

        logging.debug(f"CRC:                 0x{crc:08X}")

        return tin


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Post-build script")
    parser.add_argument("-e", "--elf", help="Elf file to convert", required=True)
    parser.add_argument("-o", "--out", help="Path to the output directory", required=True)
    parser.add_argument("-v", "--version", help="Version <major>.<minor>", default="1.0")

    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO)

    path_elf = Path(args.elf).resolve()
    path_out = Path(args.out).resolve()
    ver_major, ver_minor = parse_firmware_version(args.version)

    logging.info(f"ELF:     {path_elf}")
    logging.info(f"Out:     {path_out}")
    logging.info(f"Version: {ver_major}.{ver_minor}")

    out = convert_file(path_elf, "userapp", ver_major, ver_minor)

    if out:
        uapp_name = 'userapp_' + str(ver_major) + '.' + str(ver_minor)

        # Create binary file which can loaded over UART/BLE/ etc
        uapp_file = uapp_name + '.uapp'
        uapp_file = os.path.normpath(os.path.join(path_out, uapp_file))

        # Create out dir
        if not os.path.exists(path_out):
            os.makedirs(path_out)

        with open(uapp_file, "wb") as f:
            f.write(out)
            logging.info(f"Output file: {uapp_file}")


        # Create header file as well to test easily
        header_file = uapp_name + '_uapp.h'
        header_file = os.path.normpath(os.path.join(path_out, header_file))
        macro_guard = '__' + uapp_name.upper().replace('.', '_') + '_UAPP_H__'

        with open(header_file, "w") as f:
            f.write(f'#ifndef {macro_guard}\n')
            f.write(f'#define {macro_guard}\n\n')
            f.write('const uint8_t app[] = {\n    ')
            for i, hex_val in enumerate(out):
                f.write(f'0x{hex_val:02X},')
                if (i + 1) % 16 == 0:
                    f.write('\n    ')
            f.write('\n};\n\n')
            f.write(f'#endif // {macro_guard}\n')


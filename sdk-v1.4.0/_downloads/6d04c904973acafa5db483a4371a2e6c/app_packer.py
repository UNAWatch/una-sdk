"""
@file   app_packer.py

@author Denys Saienko <denys.saienko@droid-technologies.com>

@date   24-10-2024 (updated 04-09-2025)

@brief  Convert an ELF file into a custom binary format (.uapp) with optional
        C header generation. Additionally supports a custom output extension
        via CLI flag (-ext) so the binary filename is derived from the ELF
        basename plus the provided extension.

@details
    - The binary contains application data including relocatable sections and
      a compact icon format placeholder (no icon processing currently).
    - Performs CRC checks to ensure integrity.
    - Core purpose: generate a self-contained UAPP-like package for deployment
      on an embedded target.

@usage
    # Default behavior (keeps legacy naming):
    # Output file is "<out>/userapp_<major>.<minor>.uapp"
    python3 app_packer.py -e app.elf -o Output -v 1.2

    # With custom extension:
    # Output file is "<out>/<elf_basename>.<ext>"
    python3 app_packer.py -e app.elf -o Output -v 1.2 -ext bin

    # Optionally generate a C header next to the binary:
    python3 app_packer.py -e app.elf -o Output -v 1.2 -header
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
from PIL import Image  # kept for compatibility if icon support returns


def is_loadable_segment(segment):
    """Return True if the ELF segment is loadable (PT_LOAD)."""
    return segment['p_type'] == 'PT_LOAD'


def get_relocatable_sections(elffile):
    """Collect relocation sections (.rel*, .rela*)."""
    relocatable_sections = []
    for section in elffile.iter_sections():
        if section['sh_type'] in ('SHT_REL', 'SHT_RELA'):
            relocatable_sections.append(section)
    return relocatable_sections


def get_sections_in_loadable_segments(elffile):
    """Return list of sections that are included in any loadable segment."""
    sections_in_loadable_segments = []
    for segment in elffile.iter_segments():
        if is_loadable_segment(segment):
            for section in elffile.iter_sections():
                if segment.section_in_segment(section):
                    sections_in_loadable_segments.append(section)
    return sections_in_loadable_segments


def get_unlisted_nonzero_sections(section_names, loadable_sections):
    """Return sections (non-zero size) that are loadable but not in section_names list."""
    unlisted_sections = []
    for section in loadable_sections:
        section_name = section.name
        section_size = section['sh_size']
        if section_size > 0 and section_name not in section_names:
            unlisted_sections.append(section)
    return unlisted_sections


def filter_sections_for_relocation(elffile):
    """
    Filter relocation sections only for those that have related loadable sections.
    Example: .rel.text corresponds to .text in loadable sections.
    """
    loadable_sections = get_sections_in_loadable_segments(elffile)
    relocatable_sections = get_relocatable_sections(elffile)

    loadable_section_names = {section.name for section in loadable_sections}

    sections_for_relocation = [
        section for section in relocatable_sections
        if section.name.replace('.rel', '') in loadable_section_names
    ]

    return sections_for_relocation


def print_section_data(name, data):
    """Debug print section data as 32-bit little-endian words (hex)."""
    words = [int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4)]
    hex_lines = [f"0x{w:08X}" for w in words]
    logging.debug(f"Content of section {name}:")
    for i in range(0, len(hex_lines), 4):
        logging.debug(" ".join(hex_lines[i:i+4]))


def get_relocated_list(elffile):
    """
    Build a list of addresses that require relocation, based on REL/RELA sections.
    Excludes .rel.preinit_array, .rel.init_array, .rel.fini_array (handled at load).
    """
    rel_sections = filter_sections_for_relocation(elffile)
    rel_list = bytearray()
    for section in rel_sections:
        if section.name in ['.rel.preinit_array', '.rel.init_array', '.rel.fini_array']:
            logging.debug(f'REL Section: {section.name} size {section.data_size} - skip')
            continue
        logging.debug(f'REL Section: {section.name} size {section.data_size}')

        symtab = elffile.get_section_by_name('.symtab')

        for reloc in section.iter_relocations():
            reloc_type = next(key for key, val in elf.enums.ENUM_RELOC_TYPE_ARM.items() if val == reloc["r_info_type"])
            address = reloc["r_offset"]

            symbol = symtab.get_symbol(reloc['r_info_sym'])
            value = symbol.entry["st_value"]
            symtype = symbol.entry["st_info"]["type"]
            symname = symbol.name

            if reloc["r_info_type"] == elf.enums.ENUM_RELOC_TYPE_ARM['R_ARM_ABS32']:
                rel_list.extend(struct.pack('<I', address))
                # logging.debug(f'0x{address:08x} = 0x{value:08x}, {reloc_type}, {symtype}, {symname}')

    return rel_list


def calc_crc(data: bytes) -> int:
    """Compute CRC32 of the given bytes-like object."""
    return zlib.crc32(data)


def parse_firmware_version(version_string: str):
    """Parse version string in format '<major>.<minor>' -> (int major, int minor)."""
    match = re.match(r'(\d+)\.(\d+)', version_string)
    if match:
        major, minor = match.groups()
        return int(major), int(minor)
    raise ValueError('Incorrect version format, expected <major>.<minor>')


def compute_section_pad(section_sizes, section_addresses, section1, section2):
    """Compute padding words between two adjacent sections (by addresses)."""
    try:
        return int((section_addresses[section2] - section_addresses[section1]) / 4 - section_sizes[section1])
    except KeyError as e:
        raise ValueError(f"Missing section: {e}")


def prepare_section(elffile, tin: bytearray, section_name, section_sizes, section_pads):
    """Append section data to output with padding pattern 0xA5A5A5A5 if required."""
    pad_words = section_pads[section_name]
    logging.debug(f"loading section {section_name:<16} size {section_sizes[section_name]:6} pads {pad_words:6}")
    
    if section_sizes[section_name] > 0:
        data = elffile.get_section_by_name(section_name).data()
        tin += data

        # Optional debug dump, except for commonly large/noisy sections
        if section_name not in ['.text', '.data']:
            print_section_data(section_name, data)

    if pad_words > 0:
        pattern = b'\xA5\xA5\xA5\xA5'  # 0xA5A5A5A5
        pad_bytes = pattern * pad_words
        tin += pad_bytes


def convert_file(elf_path: Path, app_name: str, major_version: int, minor_version: int):
    """
    Assemble the UAPP-like binary.
    Returns bytes on success, or None on error.
    """
    # Validate and prepare name field (max 15 chars + NUL terminator fits 16 bytes)
    if len(app_name) <= 15:
        name = app_name.encode('utf-8') + b'\0' * (16 - len(app_name))
    else:
        logging.error('App name is greater than 15 characters.')
        return None

    # Version fields are 8-bit each
    if major_version > 255 or minor_version > 255:
        logging.error('Version cannot be greater than 255')
        return None

    with open(elf_path, 'rb') as f:
        elffile = ELFFile(f)

        section_names = [
            '.text', '.preinit_array', '.init_array', '.fini_array',
            '.plt', '.data', '.got', '.bss', '.stack'
        ]
        section_sizes = {}
        section_addresses = {}
        section_pads = {}

        for section_name in section_names:
            section = elffile.get_section_by_name(section_name)
            if section:
                # Sections must be aligned in the linker script
                if section['sh_size'] % 4:
                    logging.error(f'ELF section {section_name} size not multiple of 4')
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
            section_pads[curr_name] = compute_section_pad(section_sizes, section_addresses, curr_name, next_name)
            logging.debug(f"pad : {curr_name:<16} 0x{section_addresses.get(curr_name, 0):08X} {section_pads[curr_name]:6}")

        # Loadable but unlisted sections guard
        loadable_sections = get_sections_in_loadable_segments(elffile)
        unlisted_nonzero_sections = get_unlisted_nonzero_sections(section_names, loadable_sections)
        if unlisted_nonzero_sections:
            for section in unlisted_nonzero_sections:
                logging.warning(f"ELF file has unhandled sections: {section.name} size {section.data_size}")

        # Relocation addresses (.rel.text/.rel.data etc.), except arrays handled at load time
        rel_addr_list = get_relocated_list(elffile)

        # UAPP-like header
        header = struct.pack(
            '<4s4B4B2I8H10I',
            b'UAPP',                 # Magic
            0, 0, 0, 0,              # Reserved/version fields
            major_version,           # Major
            minor_version,           # Minor
            0, 0,                    # Reserved
            0,                       # Reserved (4B)
            len(rel_addr_list) // 4, # Relocation list size (words)
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
            0,                       # Reserved (4B)
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
        for key in ['.text','.preinit_array','.init_array','.fini_array','.plt','.data','.got','.bss','.stack']:
            logging.debug(f"{key:16} size: {section_sizes[key]}")

        tin = bytearray()

        # Header and relocation table
        tin += header
        if len(rel_addr_list) > 0:
            tin += rel_addr_list
            print_section_data('rel_addr_list', rel_addr_list)

        # Sections
        prepare_section(elffile, tin, '.text',          section_sizes, section_pads)
        prepare_section(elffile, tin, '.preinit_array', section_sizes, section_pads)
        prepare_section(elffile, tin, '.init_array',    section_sizes, section_pads)
        prepare_section(elffile, tin, '.fini_array',    section_sizes, section_pads)
        prepare_section(elffile, tin, '.plt',           section_sizes, section_pads)
        prepare_section(elffile, tin, '.data',          section_sizes, section_pads)
        prepare_section(elffile, tin, '.got',           section_sizes, section_pads)

        # CRC footer
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
    parser.add_argument("-header", action="store_true", help="Generate a C header (.h) alongside the binary")
    parser.add_argument("-ext", help="Custom extension for output file (without dot). If provided, binary filename will be '<elf_basename>.<ext>'")

    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO)

    path_elf = Path(args.elf).resolve()
    path_out = Path(args.out).resolve()
    ver_major, ver_minor = parse_firmware_version(args.version)

    logging.info(f"ELF-file    : {path_elf}")
    logging.info(f"Out         : {path_out}")
    logging.info(f"Version     : {ver_major}.{ver_minor}")

    out = convert_file(path_elf, "userapp", ver_major, ver_minor)

    if out:
        # Keep legacy app name for header/macros regardless of -ext (per requirement only the binary filename changes)
        uapp_name = f"userapp_{ver_major}.{ver_minor}"

        # Determine output binary filename
        if args.ext and len(args.ext.strip(".")) > 0:
            ext = args.ext.strip(".")
            base_name = path_elf.stem
            bin_filename = f"{base_name}.{ext}"
        else:
            bin_filename = f"{uapp_name}.uapp"

        uapp_file = os.path.normpath(os.path.join(path_out, bin_filename))

        # Ensure output directory exists
        os.makedirs(path_out, exist_ok=True)

        with open(uapp_file, "wb") as f:
            f.write(out)
            logging.info(f"Output file : {uapp_file}")

        # Optional header generation still uses legacy uapp_name for stability
        if args.header:
            header_file = os.path.normpath(os.path.join(path_out, f"{uapp_name}.h"))
            macro_guard = '__' + uapp_name.upper().replace('.', '_') + '_H__'
            array_name = uapp_name.replace('.', '_').replace('-', '_')

            with open(header_file, "w") as f:
                f.write(f'#ifndef {macro_guard}\n')
                f.write(f'#define {macro_guard}\n\n#include <stdint.h>\n\n')
                f.write(f'const uint8_t {array_name}[] = {{\n    ')
                for i, hex_val in enumerate(out):
                    f.write(f'0x{hex_val:02X},')
                    if (i + 1) % 16 == 0:
                        f.write('\n    ')
                f.write('\n};\n\n')
                f.write(f'#endif // {macro_guard}\n')

    print("")

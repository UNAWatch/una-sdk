INCLUDE "LibC/libc_exports_0.0.3.ld"

/* Entry Point */
ENTRY(AppStart)

/* Memories definition */
MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000,  LENGTH = RAM_LENGTH
}

/* === ADDED: two program segments ===
   text  -> RX (PF_R|PF_X = 5)
   data  -> RW (PF_R|PF_W = 6) */
PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

/* Sections */
SECTIONS
{
  /* Code + RO sections -> segment :text */
  .text :
  {
    . = ALIGN(4);

    *(.AppStart)

    *(.text) *(.text*) *(.glue_7) *(.glue_7t) *(.eh_frame)

    /* ARM unwind tables (RO) */
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    __exidx_start = .;
    *(.ARM.exidx*)               /* was: *(.ARM.exidx) */
    *(.gnu.linkonce.armexidx.*)  /* optional, for compatibility */
    __exidx_end = .;

    *(.RamFunc) *(.RamFunc*)   /* if this is code in RAM — still RX */

    KEEP (*(.init))
    KEEP (*(.fini))

    *(.rodata) *(.rodata*)

    /* TouchGFX RO sections */
    *(FontFlashSection)
    *(FontSearchFlashSection)
    *(TextFlashSection)
    *(IntFlashSection)

    . = ALIGN(4);
    _etext = .;
  } > RAM :text    /* <=== attach to RX segment */

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
    . = ALIGN(4);
  } > RAM :text

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
    . = ALIGN(4);
  } > RAM :text

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
    . = ALIGN(4);
  } > RAM :text

  /* PLT — trampolines -> RX */
  .plt :
  {
    . = ALIGN(4);
    KEEP(*(.plt))
    . = ALIGN(4);
  } > RAM :text

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Data -> RW segment :data */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.sys_calls)
    *(.data) *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM :data

  .got :
  {
    . = ALIGN(4);
    _sgot = .;
    *(.got)
    *(.got.plt)
    . = ALIGN(4);
    _egot = .;
  } > RAM :data

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss) *(.bss*) *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } > RAM :data

  .stack :
  {
    . = ALIGN(4);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + STACK_SIZE;
    . = ALIGN(4);
  } > RAM :data

  _estack = .;

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  /* attributes — also into :text to avoid creating a third LOAD */
  .ARM.attributes 0 : { *(.ARM.attributes) } :text
}

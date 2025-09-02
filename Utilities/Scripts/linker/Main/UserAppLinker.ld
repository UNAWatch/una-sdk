INCLUDE "LibC/libc_exports_0.0.2.ld"

/* Entry Point */
ENTRY(AppStart)

/* This stack size will be allocated by the kernel for the app */
_Max_Stack_Size = 10*1024;	/* required amount of stack */

/* NOTE! A custom application uses a heap from the main firmware (kernel). */
/* Therefore, HEAP is not defined in the linker */
   
   
/* Dummy memories definition */
MEMORY
{
  RAM	(xrw)	: ORIGIN = 0x20000000,	LENGTH = 500K
}

/* === ДОДАНО: два програмні сегменти ===
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
  /* Код + RO-секції -> сегмент :text */
  .text :
  {
    . = ALIGN(4);

    *(.AppStart)

    *(.text) *(.text*) *(.glue_7) *(.glue_7t) *(.eh_frame)

    /* ARM unwind tables (RO) */
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    __exidx_start = .;
    *(.ARM.exidx*)               /* було: *(.ARM.exidx) */
    *(.gnu.linkonce.armexidx.*)  /* опційно, для сумісності */
    __exidx_end = .;

    *(.RamFunc) *(.RamFunc*)   /* якщо це код у RAM — теж RX */

    KEEP (*(.init))
    KEEP (*(.fini))

    *(.rodata) *(.rodata*)

    /* TouchGFX RO-секції */
    *(FontFlashSection)
    *(FontSearchFlashSection)
    *(TextFlashSection)
    *(IntFlashSection)

    . = ALIGN(4);
    _etext = .;
  } > RAM :text    /* <=== прив’язка до RX-сегмента */

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

  /* PLT — код переходів -> RX */
  .plt :
  {
    . = ALIGN(4);
    KEEP(*(.plt))
    . = ALIGN(4);
  } > RAM :text

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Дані -> RW сегмент :data */
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
    . = . + _Max_Stack_Size;
    . = ALIGN(4);
  } > RAM :data

  _estack = .;

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  /* атрибути — теж у :text, щоб не створювати третій LOAD */
  .ARM.attributes 0 : { *(.ARM.attributes) } :text
}
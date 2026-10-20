OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
  RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

PHDRS
{
  text PT_LOAD FLAGS(5);   /* R-X */
  data PT_LOAD FLAGS(6);   /* RW- */
}

SECTIONS
{
  . = ORIGIN(RAM);

  .text : {
    KEEP(*(.text.init))
    *(.text .text.*)
  } > RAM :text

  .rodata : {
    *(.rodata .rodata.*)
    *(.srodata .srodata.*)
  } > RAM :text

  .data : {
    *(.data .data.*)
    *(.sdata .sdata.*)
    PROVIDE(__global_pointer$ = . + 0x800);
  } > RAM :data

  .bss (NOLOAD) : {
    _bss_start = .;
    *(.bss .bss.*)
    *(.sbss .sbss.*)
    *(COMMON)
    _bss_end = .;
  } > RAM :data

  . = ALIGN(16);
  . += 0x4000;              /* 16 KiB stack */
  _stack_top = .;

  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}

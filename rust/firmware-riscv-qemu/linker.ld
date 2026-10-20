OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
  RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
  . = ORIGIN(RAM);

  .text : {
    KEEP(*(.text.init))
    *(.text .text.*)
  } > RAM

  .rodata : {
    *(.rodata .rodata.*)
    *(.srodata .srodata.*)
  } > RAM

  .data : {
    *(.data .data.*)
    *(.sdata .sdata.*)
    PROVIDE(__global_pointer$ = . + 0x800);
  } > RAM

  .bss (NOLOAD) : {
    *(.bss .bss.*)
    *(.sbss .sbss.*)
    *(COMMON)
  } > RAM

  . = ALIGN(16);
  . += 0x4000;              /* 16 KiB stack */
  _stack_top = .;

  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}

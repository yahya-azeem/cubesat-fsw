/*
 * Placeholder memory map for the VORAGO VA41620.
 * Replace ORIGIN/LENGTH with the values from the VA41620 datasheet before
 * flashing real hardware. The build only needs a valid layout, not the
 * final addresses.
 */
ENTRY(reset)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
  .vector_table : {
    KEEP(*(.vector_table))
  } > FLASH

  .text : {
    *(.text .text.*)
  } > FLASH

  .rodata : {
    *(.rodata .rodata.*)
  } > FLASH

  .data : {
    *(.data .data.*)
  } > RAM AT > FLASH

  .bss (NOLOAD) : {
    *(.bss .bss.*)
    *(COMMON)
  } > RAM

  _stack_top = ORIGIN(RAM) + LENGTH(RAM);

  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.ARM.exidx*) }
}

/* qemu-system-riscv32 -machine virt starts executing here at 0x80000000
 * when launched with -bios none. */
    .section .text.init, "ax"
    .globl _start
_start:
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop
    la      sp, _stack_top
    call    main

1:  j       1b

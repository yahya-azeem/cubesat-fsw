//! Bare-metal rv32 Rust firmware for QEMU's `virt` machine.
//!
//! No libc, no allocator, no crates: `_start` is provided by `global_asm!`
//! and the 16550 UART at 0x1000_0000 is driven directly.
//!
//! Run:  make rust-riscv-run
#![no_std]
#![no_main]
#![deny(unsafe_op_in_unsafe_fn)]

use core::arch::global_asm;
use core::fmt::Write as _;

const UART0: usize = 0x1000_0000;

global_asm!(
    r#"
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
"#
);

struct Uart;

fn putc(byte: u8) {
    unsafe {
        // LSR bit 5 = THR empty.
        while (core::ptr::read_volatile((UART0 + 5) as *const u8) & 0x20) == 0 {}
        core::ptr::write_volatile(UART0 as *mut u8, byte);
    }
}

impl core::fmt::Write for Uart {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for b in s.bytes() {
            putc(b);
        }
        Ok(())
    }
}

#[no_mangle]
pub extern "C" fn main() -> ! {
    let mut uart = Uart;
    let _ = writeln!(uart, "rust-riscv: boot");

    let mut ticks: u32 = 0;
    loop {
        let _ = writeln!(uart, "tick {ticks}");
        ticks = ticks.wrapping_add(1);

        // Crude delay; QEMU executes this far faster than silicon.
        for _ in 0..2_000_000u32 {
            core::hint::spin_loop();
        }
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    let mut uart = Uart;
    let _ = writeln!(uart, "rust-riscv: PANIC");
    loop {
        core::hint::spin_loop();
    }
}

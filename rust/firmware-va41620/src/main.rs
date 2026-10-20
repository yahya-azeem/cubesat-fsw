//! Cortex-M4 flight software skeleton for the VORAGO VA41620.
//!
//! Hardware access is deliberately stubbed: there is no vendor PAC in this
//! tree, so this binary proves the toolchain, the vector table and the
//! `cubesat-core` integration. Drop a generated PAC (svd2rust) or an
//! `embassy` HAL underneath `main` next.
//!
//! Build: make rust-arm
#![no_std]
#![no_main]
#![deny(unsafe_op_in_unsafe_fn)]

use core::arch::{asm, global_asm};

use cubesat_core::{fsm::Event, Battery, Mode, Supervisor};

global_asm!(
    r#"
    .syntax unified
    .thumb
    .section .vector_table, "a"
    .global _vector_table
    .type _vector_table, %object
_vector_table:
    .word _stack_top
    .word reset
    .rept 14
    .word default_handler
    .endr

    .section .text.reset, "ax"
    .global reset
    .thumb_func
    .type reset, %function
reset:
    ldr r0, =_stack_top
    mov sp, r0
    bl  main
1:  b   1b

    .thumb_func
    .type default_handler, %function
default_handler:
    b   default_handler
"#
);

/// Minimal EPS model: 2.25 Wh usable, starting at 90 % SOC.
const BATTERY_CAPACITY_MWH: i64 = 2250;

#[no_mangle]
pub extern "C" fn main() -> ! {
    let mut supervisor = Supervisor::new();
    let mut battery = Battery::from_soc(BATTERY_CAPACITY_MWH, 900);

    // POST: clear the fault and start harvesting.
    supervisor.step(Event::FaultCleared);

    loop {
        match supervisor.mode() {
            Mode::Charge => {
                // 1.5 W average harvest for one 90 minute orbit.
                battery.charge(1500, 90 * 60);
            }
            Mode::Downlink => {
                // 13 W S-band burst; fall back to Safe if the guard trips.
                if battery.try_discharge(13_000, 300).is_err() {
                    supervisor.step(Event::Fault);
                }
            }
            Mode::AwaitPass | Mode::Safe => {}
        }

        // Sleep until the next interrupt (timer/radio). Real firmware would
        // program a low-power timer instead of spinning.
        unsafe { asm!("wfi", options(nomem, nostack)) }
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {
        unsafe { asm!("wfi", options(nomem, nostack)) }
    }
}

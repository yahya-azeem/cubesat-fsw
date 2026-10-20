/*
 * Bare-metal RISC-V (rv32) for QEMU's "virt" machine.
 *
 * No libc, no OpenSBI: built with -nostdlib, talking to the 16550 UART at
 * 0x1000_0000 directly. C17, no dynamic allocation.
 */
#include <stdint.h>

static const uintptr_t kUart0 = (uintptr_t)0x10000000u;
static const uintptr_t kUartLsr = 5u;
static const uint8_t kLsrThre = 0x20u;

static void mmio_write(uintptr_t address, uint8_t value)
{
    *(volatile uint8_t *)address = value;
}

static uint8_t mmio_read(uintptr_t address)
{
    return *(const volatile uint8_t *)address;
}

static void uart_putc(char c)
{
    while ((mmio_read(kUart0 + kUartLsr) & kLsrThre) == 0u) {
        /* wait for THR empty */
    }
    mmio_write(kUart0, (uint8_t)c);
}

static void uart_puts(const char *s)
{
    while (*s != '\0') {
        uart_putc(*s);
        ++s;
    }
}

int main(void)
{
    uart_puts("riscv-virt: boot\r\n");

    static volatile uint32_t ticks;
    for (;;) {
        uart_puts("tick\r\n");
        ++ticks;
        if (ticks == 0u) {
            uart_puts("tick-wrap\r\n");
        }
        for (volatile uint32_t d = 0; d < 200000u; ++d) {
            /* crude delay; QEMU runs this much faster than silicon */
        }
    }
}

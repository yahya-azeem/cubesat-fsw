/*
 * Minimal AVR target: blinks PB5 and prints over UART.
 * Runs unchanged on real silicon and under simavr.
 *
 * C17, no dynamic allocation, no implicit narrowing conversions.
 */
#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>

#ifndef F_CPU
#error "Define F_CPU (see Makefile)"
#endif

/* Compile-time constants: macros keep them usable in the delay expansion. */
#define KBAUD 9600u
#define KTICK_MS 500u

static void uart_init(void)
{
    const uint32_t divider = (F_CPU / (16u * KBAUD)) - 1u;
    UBRR0 = (uint16_t)divider;
    UCSR0B = (uint8_t)(1u << TXEN0);
    UCSR0C = (uint8_t)((1u << UCSZ01) | (1u << UCSZ00));
}

static void uart_putc(char c)
{
    while ((UCSR0A & (uint8_t)(1u << UDRE0)) == 0u) {
        /* spin */
    }
    UDR0 = (uint8_t)c;
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
    DDRB |= (uint8_t)(1u << PB5); /* LED pin as output */

    uart_init();
    uart_puts("avr-blink: boot\r\n");

    /* Volatile so the debugger can always read it at -Os. */
    static volatile uint32_t ticks;

    for (;;) {
        PORTB ^= (uint8_t)(1u << PB5);
        uart_puts("tick\r\n");
        _delay_ms(KTICK_MS);
        ++ticks;
        if (ticks == 0u) {
            uart_puts("tick-wrap\r\n");
        }
    }
}

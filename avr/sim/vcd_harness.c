/*
 * vcd_harness - run an AVR firmware image headlessly under libsimavr and
 * dump a VCD of the IO pins.
 *
 * Usage:
 *   vcd_harness <firmware.hex> <out.vcd> [mcu] [freq_hz] [run_ms]
 *
 * Intel HEX is used instead of ELF because simavr may be built without libelf
 * (Arch's package is), while HEX loading is always available.
 *
 * Link with: -lsimavr
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sim_avr.h>
#include <sim_hex.h>
#include <sim_irq.h>
#include <sim_vcd_file.h>
#include <avr_ioport.h>

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s <firmware.hex> <out.vcd> [mcu] [freq_hz] [run_ms]\n"
            "  mcu      default atmega328p\n"
            "  freq_hz  default 16000000\n"
            "  run_ms   simulated milliseconds to run, default 20\n",
            argv0);
}

static void add_port(avr_t *avr, avr_vcd_t *vcd, char port)
{
    char label[8];

    if (!avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(port), 0)) {
        fprintf(stderr, "note: MCU has no port %c, skipping\n", port);
        return;
    }

    for (int pin = 0; pin < 8; pin++) {
        snprintf(label, sizeof(label), "P%c%d", port, pin);
        avr_vcd_add_signal(vcd,
                           avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(port), pin),
                           1, label);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        usage(argv[0]);
        return 2;
    }

    const char *hex_path = argv[1];
    const char *vcd_path = argv[2];
    const char *mcu      = argc > 3 ? argv[3] : "atmega328p";
    uint32_t freq        = argc > 4 ? (uint32_t)strtoul(argv[4], NULL, 0) : 16000000u;
    uint32_t run_ms      = argc > 5 ? (uint32_t)strtoul(argv[5], NULL, 0) : 20u;

    uint32_t size = 0, start = 0;
    uint8_t *code = read_ihex_file(hex_path, &size, &start);
    if (!code || size == 0) {
        fprintf(stderr, "error: cannot read HEX '%s'\n", hex_path);
        return 1;
    }

    avr_t *avr = avr_make_mcu_by_name(mcu);
    if (!avr) {
        fprintf(stderr, "error: unknown MCU '%s'\n", mcu);
        return 1;
    }

    avr_init(avr);
    avr_loadcode(avr, code, size, (avr_flashaddr_t)start);
    avr->frequency = freq;
    free(code);

    avr_vcd_t vcd;
    if (avr_vcd_init(avr, vcd_path, &vcd, 1000) != 0) {
        fprintf(stderr, "error: cannot create '%s'\n", vcd_path);
        return 1;
    }

    add_port(avr, &vcd, 'B');
    add_port(avr, &vcd, 'D');

    avr_vcd_start(&vcd);

    uint64_t max_cycles = (uint64_t)freq * run_ms / 1000u;
    while (avr->state != cpu_Done && avr->state != cpu_Crashed &&
           avr->cycle < max_cycles)
        avr_run(avr);

    avr_vcd_stop(&vcd);

    printf("ran %llu cycles (%.1f ms sim) -> %s\n",
           (unsigned long long)avr->cycle,
           1000.0 * (double)avr->cycle / (double)freq, vcd_path);
    return 0;
}

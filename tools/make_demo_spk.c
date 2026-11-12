/*
 * make_demo_spk - write the synthetic demo kernel used by AstroNav and the
 * tests. This is a *demo* ephemeris, not a flight product:
 *
 *   SUN     relative to SSB        (fixed at origin)
 *   EARTH   relative to SUN        (circular, 1 AU, J2000 XY plane)
 *   CUBESAT relative to EARTH      (circular 550 km, i = 51.6 deg, RAAN 30)
 *
 * Coverage starts at 2026-11-12T00:00:00 UTC and lasts one day for the
 * satellite, one year for Earth.
 *
 *   cc -Ithird_party/cspice/include -o make_demo_spk tools/make_demo_spk.c \
 *      third_party/cspice/lib/cspice.a -lm
 *   ./make_demo_spk Cubesat/Spice/kernels/naif0012.tls \
 *                   Cubesat/Spice/kernels/cubesat_demo.bsp
 */
#include <SpiceUsr.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define AU_KM 149597870.7
#define MU_EARTH 398600.4418
#define R_EARTH 6378.137
#define ALT_KM 550.0
#define DEG (M_PI / 180.0)
#define DAY 86400.0

static void fail(const char* what)
{
    SpiceChar message[184] = {0};
    if (failed_c() != 0) {
        getmsg_c("SHORT", sizeof(message), message);
    }
    fprintf(stderr, "make_demo_spk: %s: %s\n", what, message);
    exit(1);
}

static void add_segment(SpiceInt handle, SpiceInt body, SpiceInt center,
                        const char* segid, SpiceInt degree, SpiceInt n,
                        const double* states, SpiceDouble epoch, SpiceDouble step)
{
    spkw08_c(handle, body, center, "J2000", epoch, epoch + (n - 1) * step,
             segid, degree, n, states, epoch, step);
    if (failed_c() != 0) {
        fail(segid);
    }
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <lsk> <output.bsp>\n", argv[0]);
        return 2;
    }

    SpiceChar action[] = "RETURN";
    erract_c("SET", 6, action);
    furnsh_c(argv[1]);
    if (failed_c() != 0) {
        fail("furnsh(leapseconds)");
    }

    SpiceDouble et0 = 0.0;
    str2et_c("2026-11-12T00:00:00", &et0);
    if (failed_c() != 0) {
        fail("str2et");
    }

    SpiceInt handle = 0;
    remove(argv[2]);  /* spkopn refuses to overwrite */
    spkopn_c(argv[2], "CUBESAT_DEMO", 0, &handle);
    if (failed_c() != 0) {
        fail("spkopn");
    }

    /* Sun fixed at the SSB origin. */
    {
        double states[2 * 6] = {0};
        add_segment(handle, 10, 0, "SUN_SSB", 1, 2, states, et0, DAY);
    }

    /* Earth: circular 1 AU orbit, daily samples for one year. */
    {
        const int n = 367;
        double* states = calloc((size_t)n * 6, sizeof(double));
        if (states == NULL) {
            return 1;
        }
        const double omega = 2.0 * M_PI / (365.256 * DAY);
        const double v = omega * AU_KM;
        for (int i = 0; i < n; ++i) {
            const double theta = omega * i * DAY;
            states[(i * 6) + 0] = AU_KM * cos(theta);
            states[(i * 6) + 1] = AU_KM * sin(theta);
            states[(i * 6) + 2] = 0.0;
            states[(i * 6) + 3] = -v * sin(theta);
            states[(i * 6) + 4] = v * cos(theta);
            states[(i * 6) + 5] = 0.0;
        }
        add_segment(handle, 399, 10, "EARTH_SUN", 3, n, states, et0, DAY);
        free(states);
    }

    /* CubeSat: circular LEO, 60 s samples for one day. */
    {
        const int n = 1441;
        const double a = R_EARTH + ALT_KM;
        const double inc = 51.6 * DEG;
        const double raan = 30.0 * DEG;
        const double rate = sqrt(MU_EARTH / (a * a * a));
        double* states = calloc((size_t)n * 6, sizeof(double));
        if (states == NULL) {
            return 1;
        }
        for (int i = 0; i < n; ++i) {
            const double u = rate * i * 60.0;
            const double x = a * cos(u);
            const double y = a * sin(u);
            const double vx = -a * rate * sin(u);
            const double vy = a * rate * cos(u);

            const double c_raan = cos(raan);
            const double s_raan = sin(raan);
            const double c_inc = cos(inc);
            const double s_inc = sin(inc);

            states[(i * 6) + 0] = (c_raan * x) - (s_raan * c_inc * y);
            states[(i * 6) + 1] = (s_raan * x) + (c_raan * c_inc * y);
            states[(i * 6) + 2] = s_inc * y;
            states[(i * 6) + 3] = (c_raan * vx) - (s_raan * c_inc * vy);
            states[(i * 6) + 4] = (s_raan * vx) + (c_raan * c_inc * vy);
            states[(i * 6) + 5] = s_inc * vy;
        }
        add_segment(handle, -999, 399, "CUBESAT_LEO", 3, n, states, et0, 60.0);
        free(states);
    }

    spkcls_c(handle);
    if (failed_c() != 0) {
        fail("spkcls");
    }

    printf("wrote %s (et0 = %.3f)\n", argv[2], et0);
    return 0;
}

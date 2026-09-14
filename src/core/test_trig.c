/*
 * test_trig.c: the simulation's own trigonometry.
 *
 * Two things have to hold. The answers have to be right, or the game
 * changes. And the bits have to be the same on every platform, or
 * lockstep desyncs. The first is checked against the platform's libm
 * in units of the last place, so a wrong answer near zero cannot hide
 * behind a loose absolute tolerance. The second is a pinned hash that
 * every platform in CI recomputes.
 */

#include "test_framework.h"
#include "tak_trig.h"
#include <math.h>
#include <float.h>
#include <stdio.h>

/* Pi is not in standard C under a portable name. */
#define TPI  3.14159265358979323846
#define TPI2 1.57079632679489661923
#define TPI4 0.78539816339744830962

/* How many last places apart `got` and the true value are. Measured
 * against the float either side of the answer, so it means the same
 * thing at every magnitude. */
static double ulps_apart(float got, double want) {
    float w = (float)want;
    if (got == w) return 0.0;
    float step = (got > w) ? (nextafterf(w, FLT_MAX) - w)
                           : (w - nextafterf(w, -FLT_MAX));
    if (step == 0.0f) return 0.0;
    double d = (double)got - (double)w;
    if (d < 0) d = -d;
    return d / (double)step;
}

/* The platform's own libm is allowed a last place of its own, and so
 * are we, so two is the line. The figures printed say how close it
 * actually is, and it is nearer half. */
#define ULPS 2.0

static double worst_of(double a, double b) { return a > b ? a : b; }

TEST(sine_and_cosine_match_the_platform_library) {
    double ws = 0.0, wc = 0.0;
    /* Two whole turns either side, in steps far off any round number. */
    for (int i = -2600; i <= 2600; i++) {
        float a = (float)i * 0.0048332f;
        ws = worst_of(ws, ulps_apart(tak_sinf(a), sin((double)a)));
        wc = worst_of(wc, ulps_apart(tak_cosf(a), cos((double)a)));
    }
    printf("[sin %.2f cos %.2f ulp] ", ws, wc);
    ASSERT(ws <= ULPS);
    ASSERT(wc <= ULPS);
}

TEST(sine_and_cosine_hold_up_far_from_zero) {
    /* An angle that has been added to for a long match, out to where
     * the reduction stops being exact. The answer drifts a few last
     * places there and stays tiny in absolute terms, which is what the
     * game can see. */
    const float far_angles[] = { 100.0f, -100.0f, 1000.0f, -1000.0f,
                                 12345.0f, -98765.0f, 8.0e6f, -1.6e7f,
                                 5.7641964e7f, -9.9e7f };
    double worst_abs = 0.0;
    for (int i = 0; i < 10; i++) {
        float a = far_angles[i];
        double es = (double)tak_sinf(a) - sin((double)a);
        double ec = (double)tak_cosf(a) - cos((double)a);
        if (es < 0) es = -es;
        if (ec < 0) ec = -ec;
        worst_abs = worst_of(worst_abs, worst_of(es, ec));
    }
    printf("[far %.2e] ", worst_abs);
    ASSERT(worst_abs < 1.0e-7);
}

TEST(an_angle_past_the_reducible_range_is_refused_not_guessed) {
    /* Nothing in the simulation reaches this. If something ever does,
     * it should be a unit that stops rather than a desync. */
    ASSERT_EQ_INT(1, tak_sinf(1.0e9f) == 0.0f);
    ASSERT_EQ_INT(1, tak_cosf(1.0e9f) == 1.0f);
    ASSERT_EQ_INT(1, tak_sinf(-1.0e9f) == 0.0f);
}

TEST(arctangent_matches_the_platform_library) {
    double worst = 0.0;
    for (int i = -4000; i <= 4000; i++) {
        float x = (float)i * 0.00317f;
        worst = worst_of(worst, ulps_apart(tak_atanf(x), atan((double)x)));
    }
    /* Well past one, where the reciprocal reduction runs, and either
     * side of the fold at tan(pi/8). */
    const float marks[] = { 5.0f, 50.0f, 5000.0f, -5.0f, -50.0f, -5000.0f,
                            0.41421356f, 0.41421357f, 1.0f, 2.4142135f };
    for (int i = 0; i < 10; i++) {
        worst = worst_of(worst, ulps_apart(tak_atanf(marks[i]),
                                           atan((double)marks[i])));
    }
    printf("[atan %.2f ulp] ", worst);
    ASSERT(worst <= ULPS);
}

TEST(two_argument_arctangent_lands_in_the_right_quadrant) {
    double worst = 0.0;
    for (int i = -60; i <= 60; i++) {
        for (int j = -60; j <= 60; j++) {
            if (i == 0 && j == 0) continue;
            float y = (float)i * 1.37f, x = (float)j * 0.91f;
            worst = worst_of(worst, ulps_apart(tak_atan2f(y, x),
                                               atan2((double)y, (double)x)));
        }
    }
    printf("[atan2 %.2f ulp] ", worst);
    ASSERT(worst <= ULPS);
}

/* The axes, both zeros and both infinities. A not-a-number carries a
 * payload the platforms do not agree on, so none may come back. */
TEST(the_edges_of_the_two_argument_arctangent_follow_the_standard) {
    const float zero = 0.0f, nzero = -0.0f;
    const float inf = (float)INFINITY;

    ASSERT_EQ_INT(1, tak_atan2f(zero, 3.0f) == 0.0f);
    ASSERT_EQ_INT(1, signbit(tak_atan2f(nzero, 3.0f)) != 0);
    ASSERT(ulps_apart(tak_atan2f(zero, -3.0f), TPI) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(nzero, -3.0f), -TPI) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(zero, nzero), TPI) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(nzero, nzero), -TPI) <= ULPS);
    ASSERT_EQ_INT(1, tak_atan2f(zero, zero) == 0.0f);

    ASSERT(ulps_apart(tak_atan2f(3.0f, zero), TPI2) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(-3.0f, zero), -TPI2) <= ULPS);

    ASSERT(ulps_apart(tak_atan2f(inf, inf), TPI4) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(inf, -inf), 3 * TPI4) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(-inf, inf), -TPI4) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(-inf, -inf), -3 * TPI4) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(inf, 2.0f), TPI2) <= ULPS);
    ASSERT(ulps_apart(tak_atan2f(2.0f, -inf), TPI) <= ULPS);
    ASSERT_EQ_INT(1, tak_atan2f(2.0f, inf) == 0.0f);

    /* Nothing above, and nothing anywhere, may hand back a NaN. */
    const float probes[] = { 0.0f, -0.0f, 1.0f, -1.0f,
                             (float)INFINITY, -(float)INFINITY };
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 6; j++) {
            float r = tak_atan2f(probes[i], probes[j]);
            ASSERT_EQ_INT(0, isnan(r));
        }
}

TEST(sine_and_cosine_are_exact_at_zero) {
    ASSERT_EQ_INT(1, tak_sinf(0.0f) == 0.0f);
    ASSERT_EQ_INT(1, tak_cosf(0.0f) == 1.0f);
}

TEST(tangent_matches_the_platform_library_away_from_the_pole) {
    double worst = 0.0;
    /* Past a half turn either way, so every quadrant is reached. The
     * one caller is a spray angle and never nears the pole, so the
     * samples step around it. */
    for (int i = -800; i <= 800; i++) {
        float a = (float)i * 0.0041f;
        double t = tan((double)a);
        if (t > 1.0e6 || t < -1.0e6) continue;
        worst = worst_of(worst, ulps_apart(tak_tanf(a), t));
    }
    printf("[tan %.2f ulp] ", worst);
    ASSERT(worst <= ULPS);
}

/* The cross-platform gate. Every platform that builds this must hash
 * the same, and a platform that does not cannot share a room. The
 * value is pinned on purpose: changing the arithmetic has to be a
 * deliberate act, visible in a diff. */
#define TRIG_PROBE_EXPECTED 0xfc18a26bu

TEST(every_platform_hashes_the_same_workload) {
    uint32_t got = tak_trig_probe();
    printf("[%08x] ", (unsigned)got);
    ASSERT_EQ_INT(1, got == TRIG_PROBE_EXPECTED);
}

int main(void) {
    TEST_SUITE("Trig");
    RUN(sine_and_cosine_match_the_platform_library);
    RUN(sine_and_cosine_hold_up_far_from_zero);
    RUN(an_angle_past_the_reducible_range_is_refused_not_guessed);
    RUN(arctangent_matches_the_platform_library);
    RUN(two_argument_arctangent_lands_in_the_right_quadrant);
    RUN(the_edges_of_the_two_argument_arctangent_follow_the_standard);
    RUN(sine_and_cosine_are_exact_at_zero);
    RUN(tangent_matches_the_platform_library_away_from_the_pole);
    RUN(every_platform_hashes_the_same_workload);
    TEST_REPORT();
}

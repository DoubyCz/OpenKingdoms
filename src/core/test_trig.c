/*
 * test_trig.c: the simulation's own trigonometry.
 *
 * Two things have to hold. The answers have to be right, or the game
 * changes. And the bits have to be the same on every platform, or
 * lockstep desyncs. The first is checked against the platform's libm,
 * the second against a pinned hash that every CI platform recomputes.
 */

#include "test_framework.h"
#include "tak_trig.h"
#include <math.h>
#include <float.h>
#include <stdio.h>

/* Two ulps of a float near 1.0. The comparison is absolute, because a
 * heading near zero has no relative scale to speak of, and it is
 * against each platform's own libm, which is itself allowed to be a
 * little off. The printed figures say how close it actually is. */
#define ULP 2.5e-7f

static float err(float got, double want) {
    double d = (double)got - want;
    return (float)(d < 0 ? -d : d);
}

TEST(sine_and_cosine_match_the_platform_library) {
    float worst_sin = 0.0f, worst_cos = 0.0f;
    /* Two whole turns either side, in steps far off any round number. */
    for (int i = -2600; i <= 2600; i++) {
        float a = (float)i * 0.0048332f;
        float es = err(tak_sinf(a), sin((double)a));
        float ec = err(tak_cosf(a), cos((double)a));
        if (es > worst_sin) worst_sin = es;
        if (ec > worst_cos) worst_cos = ec;
    }
    printf("[sin %.2e cos %.2e] ", worst_sin, worst_cos);
    ASSERT(worst_sin < ULP);
    ASSERT(worst_cos < ULP);
}

TEST(sine_and_cosine_hold_up_far_from_zero) {
    /* A pitch or a heading that has been added to for a long match. */
    const float far_angles[] = { 100.0f, -100.0f, 1000.0f, -1000.0f,
                                 12345.0f, -98765.0f };
    for (int i = 0; i < 6; i++) {
        float a = far_angles[i];
        ASSERT(err(tak_sinf(a), sin((double)a)) < 1.0e-6f);
        ASSERT(err(tak_cosf(a), cos((double)a)) < 1.0e-6f);
    }
}

TEST(arctangent_matches_the_platform_library) {
    float worst = 0.0f;
    for (int i = -4000; i <= 4000; i++) {
        float x = (float)i * 0.00317f;
        float e = err(tak_atanf(x), atan((double)x));
        if (e > worst) worst = e;
    }
    /* Well past one, where the reciprocal reduction runs. */
    const float big[] = { 5.0f, 50.0f, 5000.0f, -5.0f, -50.0f, -5000.0f };
    for (int i = 0; i < 6; i++) {
        float e = err(tak_atanf(big[i]), atan((double)big[i]));
        if (e > worst) worst = e;
    }
    printf("[atan %.2e] ", worst);
    ASSERT(worst < ULP);
}

TEST(two_argument_arctangent_lands_in_the_right_quadrant) {
    float worst = 0.0f;
    for (int i = -60; i <= 60; i++) {
        for (int j = -60; j <= 60; j++) {
            if (i == 0 && j == 0) continue;
            float y = (float)i * 1.37f, x = (float)j * 0.91f;
            float e = err(tak_atan2f(y, x), atan2((double)y, (double)x));
            if (e > worst) worst = e;
        }
    }
    printf("[atan2 %.2e] ", worst);
    ASSERT(worst < ULP);
}

TEST(the_axes_and_the_origin_answer_exactly) {
    ASSERT_EQ_INT(1, tak_atan2f(0.0f, 0.0f) == 0.0f);
    ASSERT_EQ_INT(1, tak_atan2f(0.0f, 3.0f) == 0.0f);
    ASSERT(err(tak_atan2f(0.0f, -3.0f), 3.14159265358979) < ULP);
    ASSERT(err(tak_atan2f(3.0f, 0.0f), 1.5707963267949) < ULP);
    ASSERT(err(tak_atan2f(-3.0f, 0.0f), -1.5707963267949) < ULP);
    ASSERT_EQ_INT(1, tak_sinf(0.0f) == 0.0f);
    ASSERT_EQ_INT(1, tak_cosf(0.0f) == 1.0f);
}

TEST(tangent_matches_the_platform_library_away_from_the_pole) {
    float worst = 0.0f;
    /* The one caller is a spray angle, always a small fraction of a
     * turn, so the pole is never approached. */
    for (int i = -300; i <= 300; i++) {
        float a = (float)i * 0.0041f;
        float e = err(tak_tanf(a), tan((double)a));
        if (e > worst) worst = e;
    }
    printf("[tan %.2e] ", worst);
    ASSERT(worst < ULP);
}

/* The cross-platform gate. Every platform that builds this must hash
 * the same, and a platform that does not cannot share a room. The
 * value was taken from the first green run and is pinned on purpose:
 * changing the arithmetic has to be a deliberate act. */
#define TRIG_PROBE_EXPECTED 0xe6cef64du

TEST(every_platform_hashes_the_same_workload) {
    unsigned int got = tak_trig_probe();
    printf("[%08x] ", got);
    ASSERT_EQ_INT(1, got == TRIG_PROBE_EXPECTED);
}

int main(void) {
    TEST_SUITE("Trig");
    RUN(sine_and_cosine_match_the_platform_library);
    RUN(sine_and_cosine_hold_up_far_from_zero);
    RUN(arctangent_matches_the_platform_library);
    RUN(two_argument_arctangent_lands_in_the_right_quadrant);
    RUN(the_axes_and_the_origin_answer_exactly);
    RUN(tangent_matches_the_platform_library_away_from_the_pole);
    RUN(every_platform_hashes_the_same_workload);
    TEST_REPORT();
}

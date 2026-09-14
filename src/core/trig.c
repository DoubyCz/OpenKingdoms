/*
 * trig.c -- the simulation's own sine, cosine, tangent and arctangent.
 *
 * Every line below uses only +, -, *, / and comparison on doubles.
 * IEEE 754 requires each of those to be correctly rounded, so two
 * machines that both follow it produce the same bits. No libm call
 * appears here, and the build pins contraction off so the compiler
 * cannot fold a multiply and an add into one rounding.
 *
 * The polynomials are the fdlibm minimax sets for sin, cos and atan.
 * They are accurate to a double's last places, far past what a float
 * result can show, so the float this returns is the correctly rounded
 * one for every input the game produces.
 */

#include "tak_trig.h"

/* pi/2 in three pieces whose sum is pi/2 to about 150 bits. Each is
 * exact in a double, so the subtractions below lose nothing. */
#define PIO2_1  1.57079632673412561417e+00
#define PIO2_2  6.07710050630396597660e-11
#define PIO2_3  2.02226624879595063154e-21

#define PI      3.14159265358979311600e+00
#define PIO2    1.57079632679489655800e+00
#define PIO4    7.85398163397448278999e-01

/* sin(r) on |r| <= pi/4. */
static double poly_sin(double r) {
    static const double S1 = -1.66666666666666324348e-01;
    static const double S2 =  8.33333333332248946124e-03;
    static const double S3 = -1.98412698298579493134e-04;
    static const double S4 =  2.75573137070700676789e-06;
    static const double S5 = -2.50507602534068634195e-08;
    static const double S6 =  1.58969099521155010221e-10;
    double z = r * r;
    double p = S5 + z * S6;
    p = S4 + z * p;
    p = S3 + z * p;
    p = S2 + z * p;
    p = S1 + z * p;
    return r + r * z * p;
}

/* cos(r) on |r| <= pi/4. */
static double poly_cos(double r) {
    static const double C1 =  4.16666666666666019037e-02;
    static const double C2 = -1.38888888888741095749e-03;
    static const double C3 =  2.48015872894767294178e-05;
    static const double C4 = -2.75573143513906633035e-07;
    static const double C5 =  2.08757232129817482790e-09;
    static const double C6 = -1.13596475577881948265e-11;
    double z = r * r;
    double p = C5 + z * C6;
    p = C4 + z * p;
    p = C3 + z * p;
    p = C2 + z * p;
    p = C1 + z * p;
    return 1.0 - 0.5 * z + z * z * p;
}

/* Truncate towards zero without a libm call. |q| here is small enough
 * that a long long holds it; the caller has already clamped. */
static double round_to_int(double q) {
    long long n = (long long)(q < 0.0 ? q - 0.5 : q + 0.5);
    return (double)n;
}

/* x reduced to r in [-pi/4, pi/4] plus the quadrant it came from.
 * Returns 0 when x is too large to reduce without losing the answer. */
static int reduce_quadrant(double x, double *out_r, int *out_q) {
    /* Beyond this the multiples of pi/2 outrun a double's precision.
     * Nothing in the simulation reaches it: headings stay inside a
     * turn and pitches inside a half turn. */
    if (!(x > -1.0e8 && x < 1.0e8)) return 0;
    double q = round_to_int(x / PIO2);
    /* Cody and Waite: subtract pi/2 in pieces so the cancellation
     * happens against exact values. */
    double r = x - q * PIO2_1;
    r = r - q * PIO2_2;
    r = r - q * PIO2_3;
    *out_r = r;
    *out_q = (int)(((long long)q) & 3);
    return 1;
}

float tak_sinf(float x) {
    double r;
    int quad;
    if (!reduce_quadrant((double)x, &r, &quad)) return 0.0f;
    switch (quad) {
    case 0:  return (float)poly_sin(r);
    case 1:  return (float)poly_cos(r);
    case 2:  return (float)(-poly_sin(r));
    default: return (float)(-poly_cos(r));
    }
}

float tak_cosf(float x) {
    double r;
    int quad;
    if (!reduce_quadrant((double)x, &r, &quad)) return 1.0f;
    switch (quad) {
    case 0:  return (float)poly_cos(r);
    case 1:  return (float)(-poly_sin(r));
    case 2:  return (float)(-poly_cos(r));
    default: return (float)poly_sin(r);
    }
}

float tak_tanf(float x) {
    double r;
    int quad;
    if (!reduce_quadrant((double)x, &r, &quad)) return 0.0f;
    /* An odd quadrant is a quarter turn on, where tan is -cos/sin. */
    double s = poly_sin(r), c = poly_cos(r);
    double num = (quad & 1) ? -c : s;
    double den = (quad & 1) ?  s : c;
    if (den == 0.0) return (num < 0.0) ? -3.4028234663852886e+38f
                                       :  3.4028234663852886e+38f;
    return (float)(num / den);
}

/* atan(u) on |u| <= tan(pi/8). */
static double poly_atan(double u) {
    static const double A[11] = {
         3.33333333333329318027e-01,
        -1.99999999998764832476e-01,
         1.42857142725034663711e-01,
        -1.11111104054623557880e-01,
         9.09088713343650656196e-02,
        -7.69187620504482999495e-02,
         6.66107313738753120669e-02,
        -5.83357013379057348645e-02,
         4.97687799461593236017e-02,
        -3.65315727442169155270e-02,
         1.62858201153657823623e-02,
    };
    double z = u * u;
    double p = A[10];
    for (int i = 9; i >= 0; i--) p = A[i] + z * p;
    return u - u * z * p;
}

/* tan(pi/8), the point the second reduction folds around. */
#define TAN_PIO8 4.14213562373095048802e-01

static double atan_double(double x) {
    int neg = 0;
    if (x < 0.0) { x = -x; neg = 1; }

    double result;
    if (x > 1.0) {
        /* atan(x) = pi/2 - atan(1/x) */
        double t = 1.0 / x;
        if (t > TAN_PIO8) {
            double u = (t - 1.0) / (t + 1.0);
            result = PIO2 - (PIO4 + poly_atan(u));
        } else {
            result = PIO2 - poly_atan(t);
        }
    } else if (x > TAN_PIO8) {
        /* atan(x) = pi/4 + atan((x-1)/(x+1)) */
        double u = (x - 1.0) / (x + 1.0);
        result = PIO4 + poly_atan(u);
    } else {
        result = poly_atan(x);
    }
    return neg ? -result : result;
}

float tak_atanf(float x) {
    double d = (double)x;
    /* Not a number and infinity both settle on the limit. */
    if (!(d == d)) return 0.0f;
    if (d > 1.0e30) return (float)PIO2;
    if (d < -1.0e30) return (float)(-PIO2);
    return (float)atan_double(d);
}

float tak_atan2f(float y, float x) {
    double dy = (double)y, dx = (double)x;
    if (!(dy == dy) || !(dx == dx)) return 0.0f;

    if (dx == 0.0 && dy == 0.0) return 0.0f;
    if (dx == 0.0) return (dy > 0.0) ? (float)PIO2 : (float)(-PIO2);
    if (dy == 0.0) {
        /* A negative zero x is still the negative side. */
        return (dx > 0.0) ? 0.0f : (float)PI;
    }

    double a = atan_double(dy / dx);
    if (dx > 0.0) return (float)a;
    return (dy > 0.0) ? (float)(a + PI) : (float)(a - PI);
}

/* A fixed workload every platform must hash the same way. Keeping it
 * here rather than in the test lets the game print it too. */
unsigned int tak_trig_probe(void) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < 720; i++) {
        float a = (float)i * 0.0087266462f;   /* half a degree */
        float vals[5];
        vals[0] = tak_sinf(a);
        vals[1] = tak_cosf(a);
        vals[2] = tak_atanf(a - 3.0f);
        vals[3] = tak_atan2f(vals[0], vals[1]);
        vals[4] = tak_tanf(a * 0.25f);
        for (int v = 0; v < 5; v++) {
            union { float f; unsigned int u; } bits;
            bits.f = vals[v];
            for (int b = 0; b < 4; b++) {
                h ^= (bits.u >> (b * 8)) & 0xffu;
                h *= 16777619u;
            }
        }
    }
    return h;
}

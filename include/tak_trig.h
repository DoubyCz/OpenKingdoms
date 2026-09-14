#ifndef TAK_TRIG_H
#define TAK_TRIG_H

/* The simulation's own trigonometry.
 *
 * Lockstep needs every machine to reach the same state from the same
 * orders. IEEE 754 pins + - * / and sqrt to one answer, so those agree
 * everywhere, but it says nothing about a sine. Each platform's libm
 * rounds its own way and the difference survives into a different cell
 * after enough ticks. See docs/notes/2026-09-14-float-determinism.md.
 *
 * These are built from pinned double constants and the four pinned
 * operations alone, so the answer is the same on every platform by
 * construction. Anything that writes simulation state calls these.
 * Drawing may call libm.
 *
 * Accuracy is within one ulp of a float, so gameplay is unchanged. */

float tak_sinf(float x);
float tak_cosf(float x);
float tak_tanf(float x);
float tak_atanf(float x);
float tak_atan2f(float y, float x);

/* Bit-pattern hash of a fixed workload, for the cross-platform gate. */
unsigned int tak_trig_probe(void);

#endif /* TAK_TRIG_H */

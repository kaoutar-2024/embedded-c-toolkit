/**
 * pid.h - discrete PID controller.
 *
 * - Output is clamped to [out_min, out_max].
 * - Integral term is clamped to the same range (anti-windup).
 * - Derivative is computed on the measurement, not the error, to avoid a
 *   "derivative kick" when the setpoint changes.
 */
#ifndef PID_H
#define PID_H

#include <stdbool.h>

typedef struct {
    float kp, ki, kd;
    float out_min, out_max;
    float integral;   /* already multiplied by ki */
    float prev_meas;
    bool has_prev;
} pid_ctrl_t;

void pid_init(pid_ctrl_t *p, float kp, float ki, float kd,
              float out_min, float out_max);
void pid_reset(pid_ctrl_t *p);

/** One control step. dt is the time since the last call, in seconds. */
float pid_update(pid_ctrl_t *p, float setpoint, float measurement, float dt);

#endif /* PID_H */

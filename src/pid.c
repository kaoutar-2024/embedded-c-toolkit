#include "pid.h"

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void pid_init(pid_ctrl_t *p, float kp, float ki, float kd,
              float out_min, float out_max)
{
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
    p->out_min = out_min;
    p->out_max = out_max;
    pid_reset(p);
}

void pid_reset(pid_ctrl_t *p)
{
    p->integral = 0.0f;
    p->prev_meas = 0.0f;
    p->has_prev = false;
}

float pid_update(pid_ctrl_t *p, float setpoint, float measurement, float dt)
{
    const float error = setpoint - measurement;

    p->integral = clampf(p->integral + p->ki * error * dt,
                         p->out_min, p->out_max);

    float derivative = 0.0f;
    if (p->has_prev && dt > 0.0f) {
        derivative = -(measurement - p->prev_meas) / dt;
    }
    p->prev_meas = measurement;
    p->has_prev = true;

    const float out = p->kp * error + p->integral + p->kd * derivative;
    return clampf(out, p->out_min, p->out_max);
}

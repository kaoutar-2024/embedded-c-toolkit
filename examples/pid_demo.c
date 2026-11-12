/* Step response of the PID controller on a simulated first-order plant.
 * Prints CSV (time, setpoint, output) that can be plotted with any tool. */
#include <math.h>
#include <stdio.h>

#include "pid.h"

int main(void)
{
    const float dt = 0.01f;   /* 100 Hz control loop */
    const float tau = 0.5f;   /* plant time constant, seconds */
    float x = 0.0f;           /* plant output */

    pid_ctrl_t pid;
    pid_init(&pid, 2.0f, 4.0f, 0.0f, -10.0f, 10.0f);

    float peak = 0.0f, settle_time = -1.0f;
    const float target = 1.0f;

    puts("t,setpoint,output");
    for (int i = 0; i <= 500; i++) {
        const float t = (float)i * dt;
        const float u = pid_update(&pid, target, x, dt);
        x += dt * (-x + u) / tau;

        if (x > peak) peak = x;
        if (fabsf(x - target) > 0.02f) settle_time = t; /* last time outside 2% */

        if (i % 10 == 0) printf("%.2f,%.2f,%.4f\n", t, target, x);
    }
    const float overshoot = fmaxf(0.0f, peak - target) / target * 100.0f;
    fprintf(stderr, "overshoot: %.1f %%  |  2%% settling time: %.2f s\n",
            overshoot, settle_time);
    return 0;
}

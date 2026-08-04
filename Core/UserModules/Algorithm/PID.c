#include "PID.h"

void PID_Init(PID_t *pid,
              float kp,
              float ki,
              float kd,
              float outMin,
              float outMax,
              float integralMin,
              float integralMax)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;

    pid->target = 0.0f;
    pid->actual = 0.0f;

    pid->error0 = 0.0f;
    pid->error1 = 0.0f;

    pid->integral = 0.0f;
    pid->out = 0.0f;

    pid->outMin = outMin;
    pid->outMax = outMax;

    pid->integralMin = integralMin;
    pid->integralMax = integralMax;
}

void PID_SetTarget(PID_t *pid, float target)
{
    pid->target = target;
}

float PID_Calculate(PID_t *pid, float actual)
{
    /* 记录实际值，并更新当前误差和上一次误差。 */
    pid->actual = actual;
    pid->error1 = pid->error0;
    pid->error0 = pid->target - pid->actual;

    /* 累加误差并进行积分限幅，防止积分持续增大。 */
    pid->integral += pid->error0;
    if (pid->integral > pid->integralMax)
    {
        pid->integral = pid->integralMax;
    }
    else if (pid->integral < pid->integralMin)
    {
        pid->integral = pid->integralMin;
    }

    /* 位置式PID：比例项 + 积分项 + 本次与上次误差之差形成的微分项。 */
    pid->out = pid->Kp * pid->error0
             + pid->Ki * pid->integral
             + pid->Kd * (pid->error0 - pid->error1);

    /* 输出限幅，接入M3508时通常将范围设为-16384~16384。 */
    if (pid->out > pid->outMax)
    {
        pid->out = pid->outMax;
    }
    else if (pid->out < pid->outMin)
    {
        pid->out = pid->outMin;
    }

    return pid->out;
}

void PID_Reset(PID_t *pid)
{
    pid->actual = 0.0f;
    pid->error0 = 0.0f;
    pid->error1 = 0.0f;
    pid->integral = 0.0f;
    pid->out = 0.0f;
}

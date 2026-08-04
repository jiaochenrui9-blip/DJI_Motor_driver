#include "dji_m3508.h"

HAL_StatusTypeDef DJI_M3508_Register(DJI_MotorManager_t *manager,
                                     DJI_Motor_t *motor,
                                     uint8_t esc_id)
{
    HAL_StatusTypeDef status;

    status = DJI_MotorManager_Register(manager, motor, esc_id);
    if (status != HAL_OK)
    {
        return status;
    }

    motor->type = DJI_MOTOR_M3508;
    motor->mode = MOTOR_MODE_DISABLE;
    motor->reduction_ratio = DJI_M3508_REDUCTION_RATIO;
    motor->current_limit = DJI_M3508_CURRENT_LIMIT;
    return HAL_OK;
}

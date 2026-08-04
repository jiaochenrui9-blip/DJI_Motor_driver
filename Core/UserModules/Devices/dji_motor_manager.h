#ifndef DJI_MOTOR_MANAGER_H
#define DJI_MOTOR_MANAGER_H

#include "dji_motor.h"
#include "can_bus.h"

#define DJI_MOTOR_CONTROL_ID_1_4   0x200U
#define DJI_MOTOR_CONTROL_ID_5_8   0x1FFU
#define DJI_MOTOR_FEEDBACK_ID_FIRST 0x201U
#define DJI_MOTOR_FEEDBACK_ID_LAST  0x208U

typedef struct
{
    CAN_HandleTypeDef *hcan;
    DJI_Motor_t *motors[DJI_MOTOR_COUNT];
    uint8_t registered_count;
} DJI_MotorManager_t;

HAL_StatusTypeDef DJI_MotorManager_Init(DJI_MotorManager_t *manager,
                                        CAN_HandleTypeDef *hcan);

HAL_StatusTypeDef DJI_MotorManager_Register(DJI_MotorManager_t *manager,
                                            DJI_Motor_t *motor,
                                            uint8_t esc_id);

HAL_StatusTypeDef DJI_MotorManager_Start(DJI_MotorManager_t *manager);

HAL_StatusTypeDef DJI_MotorManager_SendCurrents(DJI_MotorManager_t *manager);

void DJI_MotorManager_RxFifo0Callback(DJI_MotorManager_t *manager,
                                      CAN_HandleTypeDef *hcan);

#endif /* DJI_MOTOR_MANAGER_H */

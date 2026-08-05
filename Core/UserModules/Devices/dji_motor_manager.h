#ifndef DJI_MOTOR_MANAGER_H
#define DJI_MOTOR_MANAGER_H

#include "dji_motor.h"
#include "can_bus.h"

#define DJI_MOTOR_CONTROL_ID_1_4   0x200U
#define DJI_MOTOR_CONTROL_ID_5_8   0x1FFU
#define DJI_MOTOR_FEEDBACK_ID_FIRST 0x201U
#define DJI_MOTOR_FEEDBACK_ID_LAST  0x208U

#define DJI_MOTOR_FRAME_CONTROL     4U
static const uint16_t DJI_Motor_Frame_Control[DJI_MOTOR_FRAME_CONTROL] ={
    0x200,
    0x1FF,
    0x1FE,
    0x2FE
};

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
                                            uint8_t esc_id,
                                            uint16_t tx_id,
                                            uint8_t tx_slot);

HAL_StatusTypeDef DJI_MotorManager_Start(DJI_MotorManager_t *manager);

HAL_StatusTypeDef DJI_MotorManager_SendAll(DJI_MotorManager_t *manager);

void DJI_MotorManager_RxFifo0Callback(DJI_MotorManager_t *manager,
                                      CAN_HandleTypeDef *hcan);

#endif /* DJI_MOTOR_MANAGER_H */

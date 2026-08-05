#include "dji_motor_manager.h"

static uint16_t DJI_MotorManager_ReadU16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

static int16_t DJI_MotorManager_ReadS16(const uint8_t *data)
{
    return (int16_t)DJI_MotorManager_ReadU16(data);
}

HAL_StatusTypeDef DJI_MotorManager_Init(DJI_MotorManager_t *manager,
                                        CAN_HandleTypeDef *hcan)
{
    if ((manager == NULL) || (hcan == NULL))
    {
        return HAL_ERROR;
    }

    *manager = (DJI_MotorManager_t){0};
    manager->hcan = hcan;
    return HAL_OK;
}

HAL_StatusTypeDef DJI_MotorManager_Register(DJI_MotorManager_t *manager,
                                            DJI_Motor_t *motor,
                                            uint8_t esc_id,
                                            uint16_t tx_id,
                                            uint8_t tx_slot)
{
    uint8_t frame;

    if ((manager == NULL) || (manager->hcan == NULL) || (motor == NULL) ||
        (esc_id < 1U) || (esc_id > DJI_MOTOR_COUNT) || (tx_slot >= 4U))
    {
        return HAL_ERROR;
    }

    for (frame = 0U; frame < DJI_MOTOR_FRAME_CONTROL; ++frame)
    {
        if (tx_id == DJI_Motor_Frame_Control[frame])
        {
            break;
        }
    }

    if ((frame == DJI_MOTOR_FRAME_CONTROL) ||
        (manager->registered_count >= DJI_MOTOR_COUNT))
    {
        return HAL_ERROR;
    }

    for (uint8_t index = 0U; index < manager->registered_count; ++index)
    {
        DJI_Motor_t *registered_motor = manager->motors[index];

        if ((registered_motor != NULL) &&
            ((registered_motor->esc_id == esc_id) ||
             ((registered_motor->tx_id == tx_id) &&
              (registered_motor->tx_slot == tx_slot))))
        {
            return HAL_ERROR;
        }
    }

    *motor = (DJI_Motor_t){0};
    motor->esc_id = esc_id;
    motor->tx_id = tx_id;
    motor->tx_slot = tx_slot;
    manager->motors[manager->registered_count] = motor;
    manager->registered_count++;
    return HAL_OK;
}

HAL_StatusTypeDef DJI_MotorManager_Start(DJI_MotorManager_t *manager)
{
    HAL_StatusTypeDef status;
    uint32_t filter_bank;

    if ((manager == NULL) || (manager->hcan == NULL))
    {
        return HAL_ERROR;
    }

    if (manager->hcan->Instance == CAN1)
    {
        filter_bank = 0U;
    }
    else if (manager->hcan->Instance == CAN2)
    {
        filter_bank = 14U;
    }
    else
    {
        return HAL_ERROR;
    }

    status = CAN_Bus_ConfigStdIdMask(manager->hcan, filter_bank, 14U,
                                     0x200U, 0x7F0U);
    if (status != HAL_OK)
    {
        return status;
    }

    return CAN_Bus_StartRxFifo0(manager->hcan);
}

HAL_StatusTypeDef DJI_MotorManager_SendAll(DJI_MotorManager_t *manager)
{
    uint8_t DJI_Frame_Send[DJI_MOTOR_FRAME_CONTROL][8] = {0};
    uint8_t Frame_used[DJI_MOTOR_FRAME_CONTROL] = {0};
    if ((manager == NULL) || (manager->hcan == NULL))
    {
        return HAL_ERROR;
    }

    for (uint8_t index = 0U; index < manager->registered_count; index++)
    {
        DJI_Motor_t *motor = manager->motors[index];
        uint8_t frame;
        uint16_t output_current;

        if (motor == NULL)
        {
            continue;
        }

        if (motor->tx_slot >= 4U)
        {
            return HAL_ERROR;
        }

        for (frame = 0U; frame < DJI_MOTOR_FRAME_CONTROL; frame++)
        {
            if (motor->tx_id != DJI_Motor_Frame_Control[frame])
            {
                continue;
            }

            output_current = (uint16_t)motor->output_current;
            DJI_Frame_Send[frame][motor->tx_slot * 2U] =
                (uint8_t)(output_current >> 8);
            DJI_Frame_Send[frame][motor->tx_slot * 2U + 1U] =
                (uint8_t)output_current;
            Frame_used[frame] = 1U;

            break;
        }

        if (frame == DJI_MOTOR_FRAME_CONTROL)
        {
            return HAL_ERROR;
        }
    }

    for (uint8_t frame = 0U; frame < DJI_MOTOR_FRAME_CONTROL; frame++)
    {
        if (Frame_used[frame] != 0)
        {
            HAL_StatusTypeDef status = CAN_Bus_SendStdData(manager->hcan,DJI_Motor_Frame_Control[frame],
                                          DJI_Frame_Send[frame],8);
            if (status != HAL_OK)
            {
                return status;
            }
        }
    }

    return HAL_OK;
}

void DJI_MotorManager_RxFifo0Callback(DJI_MotorManager_t *manager,
                                      CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef header;
    DJI_Motor_t *motor;
    uint8_t data[8];
    uint8_t motor_id;
    uint8_t index;
    uint16_t angle;
    int32_t encoder_delta;

    if ((manager == NULL) || (hcan == NULL) || (hcan != manager->hcan))
    {
        return;
    }

    if (CAN_Bus_ReadFifo0(hcan, &header, data) != HAL_OK)
    {
        return;
    }

    if ((header.IDE != CAN_ID_STD) ||
        (header.RTR != CAN_RTR_DATA) ||
        (header.DLC != 8U))
    {
        return;
    }

    if (hcan->Instance == CAN1)
    {
        if ((header.StdId < DJI_MOTOR_FEEDBACK_ID_FIRST) ||
            (header.StdId > DJI_MOTOR_FEEDBACK_ID_LAST))
        {
            return;
        }

        motor_id = (uint8_t)(header.StdId - 0x200U);
    }
    else if (hcan->Instance == CAN2)
    {
        if ((header.StdId < 0x205U) || (header.StdId > 0x20BU))
        {
            return;
        }

        motor_id = (uint8_t)(header.StdId - 0x204U);
    }
    else
    {
        return;
    }

    motor = NULL;
    for (index = 0U; index < DJI_MOTOR_COUNT; ++index)
    {
        if ((manager->motors[index] != NULL) &&
            (manager->motors[index]->esc_id == motor_id))
        {
            motor = manager->motors[index];
            break;
        }
    }

    if (motor == NULL)
    {
        return;
    }

    angle = DJI_MotorManager_ReadU16(&data[0]);
    if (motor->feedback_count == 0U)
    {
        motor->last_encoder = (int16_t)angle;
        motor->total_encoder = 0;
    }
    else
    {
        encoder_delta = (int32_t)angle - (int32_t)motor->last_encoder;
        if (encoder_delta > 4096)
        {
            encoder_delta -= 8192;
        }
        else if (encoder_delta < -4096)
        {
            encoder_delta += 8192;
        }

        motor->total_encoder += encoder_delta;
        motor->last_encoder = (int16_t)angle;
    }

    motor->encoder = angle;
    motor->speed_rpm = DJI_MotorManager_ReadS16(&data[2]);
    motor->feedback_current = DJI_MotorManager_ReadS16(&data[4]);
    motor->temperature = (motor->type == DJI_MOTOR_M3508 || motor->type == DJI_MOTOR_GM6020) ? data[6] : 0U;
    motor->last_feedback_tick = HAL_GetTick();
    motor->feedback_count++;
    motor->feedback_updated = 1U;
}

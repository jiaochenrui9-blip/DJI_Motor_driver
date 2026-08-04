#include "dji_motor_manager.h"

static uint16_t DJI_MotorManager_ReadU16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

static int16_t DJI_MotorManager_ReadS16(const uint8_t *data)
{
    return (int16_t)DJI_MotorManager_ReadU16(data);
}

static void DJI_MotorManager_WriteCurrentData(const int16_t current[4],
                                              uint8_t data[8])
{
    for (uint8_t i = 0U; i < 4U; ++i)
    {
        const uint16_t raw = (uint16_t)current[i];
        data[2U * i] = (uint8_t)(raw >> 8);
        data[2U * i + 1U] = (uint8_t)raw;
    }
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
                                            uint8_t esc_id)
{
    uint8_t index;

    if ((manager == NULL) || (manager->hcan == NULL) || (motor == NULL) ||
        (esc_id < 1U) || (esc_id > DJI_MOTOR_COUNT))
    {
        return HAL_ERROR;
    }

    index = esc_id - 1U;
    if (manager->motors[index] != NULL)
    {
        return HAL_ERROR;
    }

    *motor = (DJI_Motor_t){0};
    motor->esc_id = esc_id;
    manager->motors[index] = motor;
    manager->registered_count++;
    return HAL_OK;
}

HAL_StatusTypeDef DJI_MotorManager_Start(DJI_MotorManager_t *manager)
{
    static const uint16_t feedback_ids_1_4[4] = {0x201U, 0x202U, 0x203U, 0x204U};
    static const uint16_t feedback_ids_5_8[4] = {0x205U, 0x206U, 0x207U, 0x208U};
    HAL_StatusTypeDef status;

    if ((manager == NULL) || (manager->hcan == NULL))
    {
        return HAL_ERROR;
    }

    status = CAN_Bus_ConfigStdIdList(manager->hcan, 0U, 14U,
                                     feedback_ids_1_4);
    if (status != HAL_OK)
    {
        return status;
    }

    status = CAN_Bus_ConfigStdIdList(manager->hcan, 1U, 14U,
                                     feedback_ids_5_8);
    if (status != HAL_OK)
    {
        return status;
    }

    return CAN_Bus_StartRxFifo0(manager->hcan);
}

HAL_StatusTypeDef DJI_MotorManager_SendCurrents(DJI_MotorManager_t *manager)
{
    HAL_StatusTypeDef status;
    int16_t current_1_4[4] = {0};
    int16_t current_5_8[4] = {0};
    uint8_t tx_data_1_4[8];
    uint8_t tx_data_5_8[8];

    if ((manager == NULL) || (manager->hcan == NULL))
    {
        return HAL_ERROR;
    }

    for (uint8_t index = 0U; index < DJI_MOTOR_COUNT; ++index)
    {
        DJI_Motor_t *motor = manager->motors[index];
        if (motor == NULL)
        {
            continue;
        }

        if (index < 4U)
        {
            current_1_4[index] = motor->output_current;
        }
        else
        {
            current_5_8[index - 4U] = motor->output_current;
        }
    }

    if (HAL_CAN_GetTxMailboxesFreeLevel(manager->hcan) < 2U)
    {
        return HAL_BUSY;
    }

    DJI_MotorManager_WriteCurrentData(current_1_4, tx_data_1_4);
    DJI_MotorManager_WriteCurrentData(current_5_8, tx_data_5_8);

    status = CAN_Bus_SendStdData(manager->hcan,
                                 DJI_MOTOR_CONTROL_ID_1_4,
                                 tx_data_1_4,
                                 8U);
    if (status != HAL_OK)
    {
        return status;
    }

    return CAN_Bus_SendStdData(manager->hcan,
                               DJI_MOTOR_CONTROL_ID_5_8,
                               tx_data_5_8,
                               8U);
}

void DJI_MotorManager_RxFifo0Callback(DJI_MotorManager_t *manager,
                                      CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef header;
    DJI_Motor_t *motor;
    uint8_t data[8];
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
        (header.DLC != 8U) ||
        (header.StdId < DJI_MOTOR_FEEDBACK_ID_FIRST) ||
        (header.StdId > DJI_MOTOR_FEEDBACK_ID_LAST))
    {
        return;
    }

    index = (uint8_t)(header.StdId - DJI_MOTOR_FEEDBACK_ID_FIRST);
    motor = manager->motors[index];
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
    motor->temperature = (motor->type == DJI_MOTOR_M3508) ? data[6] : 0U;
    motor->last_feedback_tick = HAL_GetTick();
    motor->feedback_count++;
    motor->feedback_updated = 1U;
}

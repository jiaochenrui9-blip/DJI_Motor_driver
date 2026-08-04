#ifndef CAN_BUS_H
#define CAN_BUS_H

#include "main.h"

/* 配置一个可接收4个标准帧ID的16位列表过滤器。 */
HAL_StatusTypeDef CAN_Bus_ConfigStdIdList(CAN_HandleTypeDef *hcan,
                                          uint32_t filter_bank,
                                          uint32_t slave_start_bank,
                                          const uint16_t std_ids[4]);

/* 启动CAN，并开启FIFO0有新消息中断。 */
HAL_StatusTypeDef CAN_Bus_StartRxFifo0(CAN_HandleTypeDef *hcan);

/* 发送一帧11位标准ID的数据帧。 */
HAL_StatusTypeDef CAN_Bus_SendStdData(CAN_HandleTypeDef *hcan,
                                      uint16_t std_id,
                                      const uint8_t *data,
                                      uint8_t length);

/* 从接收FIFO0读取一帧CAN数据。 */
HAL_StatusTypeDef CAN_Bus_ReadFifo0(CAN_HandleTypeDef *hcan,
                                    CAN_RxHeaderTypeDef *header,
                                    uint8_t data[8]);

#endif /* CAN_BUS_H */

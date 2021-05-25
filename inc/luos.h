/******************************************************************************
 * @file luos
 * @brief User functionalities of the Luos library
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef LUOS_H
#define LUOS_H

#include "luos_utils.h"
#include "luos_list.h"
#include "service_structs.h"
#include "routing_table.h"
#include "luos_od.h"
#include "streaming.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/******************************************************************************
 * @struct general_stats_t
 * @brief format all datas to be sent trough msg
 ******************************************************************************/
typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            luos_stats_t node_stat;
            service_stats_t service_stat;
        };
        uint8_t unmap[sizeof(luos_stats_t) + sizeof(service_stats_t)]; /*!< streamable form. */
    };
} general_stats_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Luos_Init(void);
void Luos_Loop(void);
void Luos_ServicesClear(void);
service_t *Luos_CreateService(CONT_CB cont_cb, uint8_t type, const char *alias, revision_t revision);
error_return_t Luos_SendMsg(service_t *service, msg_t *msg);
error_return_t Luos_ReadMsg(service_t *service, msg_t **returned_msg);
error_return_t Luos_ReadFromService(service_t *service, int16_t id, msg_t **returned_msg);
void Luos_SendData(service_t *service, msg_t *msg, void *bin_data, uint16_t size);
error_return_t Luos_ReceiveData(service_t *service, msg_t *msg, void *bin_data);
void Luos_SendStreaming(service_t *service, msg_t *msg, streaming_channel_t *stream);
error_return_t Luos_ReceiveStreaming(service_t *service, msg_t *msg, streaming_channel_t *stream);
void Luos_SendBaudrate(service_t *service, uint32_t baudrate);
void Luos_SetExternId(service_t *service, target_mode_t target_mode, uint16_t target, uint16_t newid);
uint16_t Luos_NbrAvailableMsg(void);
uint32_t Luos_GetSystick(void);
error_return_t Luos_TxComplete(void);
void Luos_Flush(void);

#endif /* LUOS_H */

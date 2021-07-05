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
#include "container_structs.h"
#include "routing_table.h"
#include "luos_od.h"
#include "streaming.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
// Initialise package with a macro
#define LUOS_ADD_PACKAGE(_name) \
    Luos_AddPackage(_name##_Init, _name##_Loop);

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
            container_stats_t container_stat;
        };
        uint8_t unmap[sizeof(luos_stats_t) + sizeof(container_stats_t)]; /*!< streamable form. */
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
void Luos_ContainersClear(void);
container_t *Luos_CreateContainer(CONT_CB cont_cb, uint8_t type, const char *alias, revision_t revision);
error_return_t Luos_SendMsg(container_t *container, msg_t *msg);
error_return_t Luos_ReadMsg(container_t *container, msg_t **returned_msg);
error_return_t Luos_ReadFromContainer(container_t *container, int16_t id, msg_t **returned_msg);
void Luos_SendData(container_t *container, msg_t *msg, void *bin_data, uint16_t size);
error_return_t Luos_ReceiveData(container_t *container, msg_t *msg, void *bin_data);
void Luos_SendStreaming(container_t *container, msg_t *msg, streaming_channel_t *stream);
error_return_t Luos_ReceiveStreaming(container_t *container, msg_t *msg, streaming_channel_t *stream);
void Luos_SendBaudrate(container_t *container, uint32_t baudrate);
void Luos_SetExternId(container_t *container, target_mode_t target_mode, uint16_t target, uint16_t newid);
uint16_t Luos_NbrAvailableMsg(void);
uint32_t Luos_GetSystick(void);
error_return_t Luos_TxComplete(void);
void Luos_Flush(void);
void Luos_AddPackage(void (*Init)(void), void (*Loop)(void));

#endif /* LUOS_H */

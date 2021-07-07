/******************************************************************************
 * @file robus
 * @brief User functionalities of the robus communication protocol
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef _ROBUS_H_
#define _ROBUS_H_

#include <stdint.h>
#include "robus_struct.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Robus_Init(memory_stats_t *memory_stats);
void Robus_Loop(void);
void Robus_ContainerIdInit(void);
ll_container_t *Robus_ContainerCreate(uint16_t type);
void Robus_ContainersClear(void);
error_return_t Robus_SendMsg(ll_container_t *ll_container, msg_t *msg);
node_t *Robus_GetNode(void);
void Robus_Flush(void);

#endif /* _ROBUS_H_ */

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
ll_service_t *Robus_ServiceCreate(uint16_t type);
void Robus_ServicesClear(void);
error_return_t Robus_SendMsg(ll_service_t *ll_service, msg_t *msg);
uint16_t Robus_TopologyDetection(ll_service_t *ll_service);
node_t *Robus_GetNode(void);
void Robus_Flush(void);

#endif /* _ROBUS_H_ */

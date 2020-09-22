/******************************************************************************
 * @file reception
 * @brief reception state machine
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef _RECEPTION_H_
#define _RECEPTION_H_

#include "context.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
// Callbacks reception
void Recep_GetHeader(volatile unsigned char *data);
void Recep_GetData(volatile unsigned char *data);
void Recep_GetCollision(volatile unsigned char *data);

// Callbacks send
void Recep_CatchAck(volatile unsigned char *data);

void Recep_Reset(void);
void Recep_Timeout(void);
void Recep_MsgComplete(msg_t *msg);
uint8_t Recep_ModuleConcerned(header_t *header);

#endif /* _RECEPTION_H_ */

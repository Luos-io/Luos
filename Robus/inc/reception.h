/******************************************************************************
 * @file reception
 * @brief reception state machine
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef _RECEPTION_H_
#define _RECEPTION_H_

#include <robus.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef void (*DATA_CB)(volatile uint8_t *data);

typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            uint8_t identifier : 4;
            uint8_t rx_error : 1;
            uint8_t rx_timeout : 1;
            uint8_t rx_framing_error : 1;
        };
        uint8_t unmap; /*!< Uncmaped form. */
    };
} status_t;

typedef struct
{
    DATA_CB callback;
    status_t status;
} RxCom_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
// Callbacks reception
void Recep_GetHeader(volatile uint8_t *data);
void Recep_GetData(volatile uint8_t *data);
void Recep_GetCollision(volatile uint8_t *data);
void Recep_Drop(volatile uint8_t *data);

// Callbacks send
void Recep_CatchAck(volatile uint8_t *data);

void Recep_Init(void);
void Recep_EndMsg(void);
void Recep_Reset(void);
void Recep_Timeout(void);
void Recep_InterpretMsgProtocol(msg_t *msg);
luos_localhost_t Recep_NodeConcerned(header_t *header);
ll_service_t *Recep_GetConcernedLLService(header_t *header);

#endif /* _RECEPTION_H_ */

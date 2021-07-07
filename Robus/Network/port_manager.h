/******************************************************************************
 * @file portManager
 * @brief portManager state machine.
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef _PORTMANAGER_H_
#define _PORTMANAGER_H_

#include <robus.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
typedef struct
{
    //Port manager
    volatile uint8_t activ;    //last Port where thereis activity
    volatile uint8_t keepLine; //status of the line poked by your node
} PortMng_t;
/*******************************************************************************
 * Function
 ******************************************************************************/
void PortMng_Init(void);
void PortMng_PtpHandler(uint8_t PortNbr);
uint8_t PortMng_PokePort(uint8_t PortNbr);
error_return_t PortMng_PokeNextPort(void);
uint8_t PortMng_PortPokedStatus(void);

#endif /* _PORTMANAGER_H_ */

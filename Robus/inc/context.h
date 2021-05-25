/******************************************************************************
 * @file context
 * @brief definition of context structure environement luos variable
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <robus.h>
#include "config.h"
#include "port_manager.h"
#include "reception.h"
#include "transmission.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef struct
{

    // Variables
    node_t node;    /*!< Node informations. */
    RxCom_t rx;     /*!< Receiver informations. */
    TxCom_t tx;     /*!< Transmitter informations. */
    PortMng_t port; /*!< port informations. */

    //Virtual service management
    ll_service_t ll_service_table[MAX_SERVICE_NUMBER]; /*!< Virtual Service table. */
    uint16_t ll_service_number;                        /*!< Virtual Service number. */

} context_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile extern context_t ctx;
/*******************************************************************************
 * Function
 ******************************************************************************/

#endif /* _CONTEXT_H_ */

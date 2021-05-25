/******************************************************************************
 * @file config
 * @brief config for luos library ans robus protocole
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef _CONFIG_H_
#define _CONFIG_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEFAULTID         0x00
#define PROTOCOL_REVISION 0
#define BROADCAST_VAL     0x0FFF
#define DEFAULTBAUDRATE   1000000

#define TIMEOUT_VAL           2
#define MAX_ALIAS_SIZE        16
#define MAX_DATA_MSG_SIZE     128
#define MAX_MULTICAST_ADDRESS 1

#ifndef NBR_RETRY
#define NBR_RETRY 10
#endif

#ifndef MAX_SERVICE_NUMBER
#define MAX_SERVICE_NUMBER 5
#endif

#ifndef MSG_BUFFER_SIZE
#define MSG_BUFFER_SIZE 3 * sizeof(msg_t)
#endif

#ifndef MAX_MSG_NB
#define MAX_MSG_NB 2 * MAX_SERVICE_NUMBER
#endif

#ifndef NBR_PORT
#define NBR_PORT 2
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/

#endif /* _CONFIG_H_ */

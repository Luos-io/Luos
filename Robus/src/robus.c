/******************************************************************************
 * @file robus
 * @brief User functionalities of the robus communication protocol
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <robus.h>

#include <string.h>
#include "transmission.h"
#include "reception.h"
#include "context.h"
#include "luos_hal.h"
#include "msg_alloc.h"
#include <stdbool.h>

#include "network.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

static error_return_t Robus_MsgHandler(msg_t *input);
/*******************************************************************************
 * Variables
 ******************************************************************************/
// Creation of the robus context. This variable is used in all files of this lib.
volatile context_t ctx;
uint32_t baudrate; /*!< System current baudrate. */

/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief Initialisation of the Robus communication protocole
 * @param None
 * @return None
 ******************************************************************************/
void Robus_Init(memory_stats_t *memory_stats)
{
    // Init the number of created  virtual container.
    ctx.ll_container_number = 0;
    // Set default container id. This id is a void id used if no container is created.
    ctx.node.node_id = DEFAULTID;
    // By default node are not certified.
    ctx.node.certified = false;
    // no transmission lock
    ctx.tx.lock = false;
    // Init collision state
    ctx.tx.collision = false;
    // Init Tx status
    ctx.tx.status = TX_DISABLE;
    // Save luos baudrate
    baudrate = DEFAULTBAUDRATE;

    // Init reception
    Recep_Init();

    // Clear message allocation buffer table
    MsgAlloc_Init(memory_stats);

    // Init hal
    LuosHAL_Init();

    // init detection structure
    Robus_ContainerIdInit();
    Network_PortInit();

    // Initialize the robus container status
    ctx.rx.status.unmap      = 0;
    ctx.rx.status.identifier = 0xF;
}
/******************************************************************************
 * @brief Loop of the Robus communication protocole
 * @param None
 * @return None
 ******************************************************************************/
void Robus_Loop(void)
{
    // Execute message allocation tasks
    MsgAlloc_loop();
    // Interpreat received messages and create luos task for it.
    msg_t *msg = NULL;
    while (MsgAlloc_PullMsgToInterpret(&msg) == SUCCEED)
    {
        // Check if this message is a protocol one
        if (Robus_MsgHandler(msg) == FAILED)
        {
            // If not create luos tasks.
            Recep_InterpretMsgProtocol(msg);
        }
    }

    // network management
    Network_Loop();
}

/******************************************************************************
 * @brief Set Container ID to default ID
 * @param None
 * @return None
 ******************************************************************************/
void Robus_ContainerIdInit(void)
{
    // Reinit ll_container id
    for (uint8_t i = 0; i < ctx.ll_container_number; i++)
    {
        ctx.ll_container_table[i].id = DEFAULTID;
    }
}
/******************************************************************************
 * @brief crete a container in route table
 * @param type of container create
 * @return None
 ******************************************************************************/
ll_container_t *Robus_ContainerCreate(uint16_t type)
{
    // Set the container type
    ctx.ll_container_table[ctx.ll_container_number].type = type;
    // Initialise the container id, TODO the ID could be stored in EEprom, the default ID could be set in factory...
    ctx.ll_container_table[ctx.ll_container_number].id = DEFAULTID;
    // Initialize dead container detection
    ctx.ll_container_table[ctx.ll_container_number].dead_container_spotted = 0;
    // Clear stats
    ctx.ll_container_table[ctx.ll_container_number].ll_stat.max_retry = 0;
    // Return the freshly initialized ll_container pointer.
    return (ll_container_t *)&ctx.ll_container_table[ctx.ll_container_number++];
}
/******************************************************************************
 * @brief clear container list in route table
 * @param None
 * @return None
 ******************************************************************************/
void Robus_ContainersClear(void)
{
    // Clear ll_container table
    memset((void *)ctx.ll_container_table, 0, sizeof(ll_container_t) * MAX_CONTAINER_NUMBER);
    // Reset the number of created containers
    ctx.ll_container_number = 0;
}
/******************************************************************************
 * @brief Send Msg to a container
 * @param container to send
 * @param msg to send
 * @return none
 ******************************************************************************/
error_return_t Robus_SendMsg(ll_container_t *ll_container, msg_t *msg)
{
    uint8_t ack        = 0;
    uint16_t data_size = 0;
    uint16_t crc_val   = 0xFFFF;
    // ********** Prepare the message ********************
    // Set protocol revision and source ID on the message
    msg->header.protocol = PROTOCOL_REVISION;
    if (ll_container->id != 0)
    {
        msg->header.source = ll_container->id;
    }
    else
    {
        msg->header.source = ctx.node.node_id;
    }

    // Compute the full message size based on the header size info.
    if (msg->header.size > MAX_DATA_MSG_SIZE)
    {
        data_size = MAX_DATA_MSG_SIZE;
    }
    else
    {
        data_size = msg->header.size;
    }
    // Add the CRC to the total size of the message
    uint16_t full_size = sizeof(header_t) + data_size + CRC_SIZE;

    // compute the CRC
    for (uint16_t i = 0; i < full_size - 2; i++)
    {
        uint16_t dbyte = msg->stream[i];
        crc_val ^= dbyte << 8;
        for (uint8_t j = 0; j < 8; ++j)
        {
            uint16_t mix = crc_val & 0x8000;
            crc_val      = (crc_val << 1);
            if (mix)
                crc_val = crc_val ^ 0x0007;
        }
    }

    // Check the localhost situation
    uint8_t localhost = Recep_NodeConcerned(&msg->header);
    // Check if ACK needed
    if (((msg->header.target_mode == IDACK) || (msg->header.target_mode == NODEIDACK)) && (localhost && (msg->header.target != DEFAULTID)))
    {
        // This is a localhost message and we need to transmit a ack. Add it at the end of the data to transmit
        ack = ctx.rx.status.unmap;
        full_size++;
    }

    // ********** Allocate the message ********************
    if (MsgAlloc_SetTxTask(ll_container, (uint8_t *)msg->stream, crc_val, full_size, localhost, ack) == FAILED)
    {
        return FAILED;
    }
    // **********Try to send the message********************
    Transmit_Process();

    return SUCCEED;
}

/******************************************************************************
 * @brief check if received messages are protocols one and manage it if it is.
 * @param msg pointer to the reeived message
 * @return error_return_t SUCCEED if the message have been consumed.
 ******************************************************************************/
static error_return_t Robus_MsgHandler(msg_t *input)
{
    uint32_t baudrate;

    switch (input->header.cmd)
    {
        case WRITE_NODE_ID:
        case CONNECT_MEMBER:
        case ACCEPT_CONNECTION:
        case DISCOVER_NEIGHBORS:
        case TOPOLOGY_INFORMATION:
            Network_MsgHandler(input);
            return SUCCEED;
            break;
        case RESET_DETECTION:
            return SUCCEED;
            break;
        case SET_BAUDRATE:
            // We have to wait the end of transmission of all the messages we have to transmit
            while (MsgAlloc_TxAllComplete() == FAILED)
                ;
            memcpy(&baudrate, input->data, sizeof(uint32_t));
            LuosHAL_ComInit(baudrate);
            return SUCCEED;
            break;
        default:
            return FAILED;
            break;
    }
    return FAILED;
}
/******************************************************************************
 * @brief get node structure
 * @param None
 * @return Node pointer
 ******************************************************************************/
node_t *Robus_GetNode(void)
{
    return (node_t *)&ctx.node;
}
/******************************************************************************
 * @brief Flush the entire msg buffer
 * @param None
 * @return None
 ******************************************************************************/
void Robus_Flush(void)
{
    while (ctx.tx.lock != false)
        ;
    LuosHAL_SetIrqState(false);
    MsgAlloc_Init(NULL);
    LuosHAL_SetIrqState(true);
}

/******************************************************************************
 * @brief get container structure
 * @param None
 * @return Node pointer
 ******************************************************************************/
ll_container_t *Robus_GetContainerFromID(uint16_t container_id)
{
    return (ll_container_t *)&ctx.ll_container_table[container_id];
}

/******************************************************************************
 * @brief get container number
 * @param None
 * @return container number
 ******************************************************************************/
uint16_t Robus_GetContainerNumber(void)
{
    return ctx.ll_container_number;
}

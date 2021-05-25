/******************************************************************************
 * @file transmission
 * @brief transmission management.
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

/******************************* Description of the TX process ************************************************************************
 * 
 *                                           |                             Luos_send
 *                                           |                                 |
 *                                           |                                 |
 *                       TX_status=NOK  +----v-----+  TX_status=DISABLE  +-----v-----+
 *                           +----------|  Timeout |-----------+         |    Msg    |         +-----------+     +-----------+
 *                           |          +----------+           |         |Preparation|         | Collision | x4  |  Prepare  |      ^
 *                           |               |TX_status=OK     |         +-----------+         | Reception |---->|  ACK RX   |------+
 *                           |               |                 |               |               +-----------+     +-----------+
 *      +-----------+   +----v-----+    +----v-----+    +------v----+    +-----v-----+               |
 * ^    | TX_status |   |  Delay   |    |   rm     |    | TX_status |    |    Tx     |     ^         |Collision
 * +----| =DISABLE  |<--| compute  |    | TX task  |    | =DISABLE  |    |Allocation |-----+         |
 *      +-----------+   +----------+    +----------+    +-----------+    +-----------+tx_lock  +-----v-----+
 *                           |               |             |                   |               |  Stop TX  |
 *                           |               |             |         +---------+-------+       +-----------+
 *                           |retry=10       +-------------+-------->|Transmit |       |             |
 *                           |               |                       |Process  |       |       +-----v-----+
 *                      +----v-----+         |                       |   +-----v-----+ |       | Pass data |
 *                      | Exclude  |         |                       |   |    Get    | |   ^   |   to RX   |
 *                      |service |---------+                       |   | TX_tasks  |-+---+   +-----------+
 *                      +----------+                                 |   +-----------+ No task       |
 *                                                                   |         |       |       +-----v-----+
 *                                                                   |   +-----v-----+ |       | TX_status |      ^
 *                                                                   |   |Send + set | |       |   =NOK    |------+
 *                                                                   |   | TX_status | |       +-----------+
 *                                                                   |   +-----------+ |
 *                                                                   |         |       |
 *                                      +----------+                 |   +-----v-----+ |
 *                      +----------+    |TX_status |     ^           |   |   Enable  | |   ^
 *                  --->|   RX[1]  |--->| =DISABLE |-----+           |   | Collision |-+---+
 *                      +----------+    +----------+                 |   +-----------+ |
 *                                                                   +-----------------+
 * 
 ************************************************************************************************************************************/

#include <transmission.h>

#include "luos_hal.h"
#include <string.h>
#include <stdbool.h>
#include "context.h"
#include "reception.h"
#include "msg_alloc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t nbrRetry = 0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static uint8_t Transmit_GetLockStatus(void);

/******************************************************************************
 * @brief Transmit an ACK
 * @param None
 * @return None
 ******************************************************************************/
void Transmit_SendAck(void)
{
    // Info : We don't consider this transmission as a complete message transmission but as a complete message reception.
    LuosHAL_SetRxState(false);
    // Transmit Ack data
    LuosHAL_ComTransmit((unsigned char *)&ctx.rx.status.unmap, 1);
    // Reset Ack status
    ctx.rx.status.unmap = 0x0F;
}
/******************************************************************************
 * @brief transmission process
 * @param pointer data to send
 * @param size of data to send
 * @return None
 ******************************************************************************/
void Transmit_Process()
{
    uint8_t *data = 0;
    uint16_t size;
    uint8_t localhost;
    ll_service_t *ll_service_pt;
    if ((MsgAlloc_GetTxTask(&ll_service_pt, &data, &size, &localhost) == SUCCEED) && (Transmit_GetLockStatus() == false))
    {
        // We have something to send
        // Check if we already try to send it multiple times and save it on stats if it is
        if ((*ll_service_pt->ll_stat.max_retry < nbrRetry) || (nbrRetry >= NBR_RETRY))
        {
            *ll_service_pt->ll_stat.max_retry = nbrRetry;
            if (*ll_service_pt->ll_stat.max_retry >= NBR_RETRY)
            {
                // We failed to transmit this message. We can't allow it, there is a issue on this target.
                // If it was an ACK issue, save the target as dead service into the sending ll_service
                if (ctx.tx.collision)
                {
                    ll_service_pt->dead_service_spotted = (uint16_t)(((msg_t *)data)->header.target);
                }
                nbrRetry         = 0;
                ctx.tx.collision = false;
                // Remove all transmist messages of this specific target
                MsgAlloc_PullServiceFromTxTask((uint16_t)(((msg_t *)data)->header.target));
                // Try to get a tx_task for another service
                if (MsgAlloc_GetTxTask(&ll_service_pt, &data, &size, &localhost) == FAILED)
                {
                    // Nothing to transmit anymore, just exit.
                    return;
                }
            }
        }
        // Check if we will need an ACK for this message and compute the transmit status we will need to manage it
        transmitStatus_t initial_transmit_status = TX_OK;
        if (((((msg_t *)data)->header.target_mode == IDACK) || (((msg_t *)data)->header.target_mode == NODEIDACK)) && (!localhost || (((msg_t *)data)->header.target == DEFAULTID)))
        {
            // We will need to validate the good reception of the ack.
            // Switch the tx status as TX_NOK allowing to detect a default at the next Timeout if no ACK have been received.
            initial_transmit_status = TX_NOK;
        }
        // Now we can try to send something
        if (Transmit_GetLockStatus() == false)
        {
            // We are free to transmit
            // We will prepare to transmit something enable tx status with precomputed value if we need ACK
            ctx.tx.status = initial_transmit_status;
            // Lock the bus
            ctx.tx.lock = true;
            LuosHAL_SetRxDetecPin(false);
            // Switch reception in collision detection mode
            LuosHAL_SetIrqState(false);
            ctx.rx.callback = Recep_GetCollision;
            LuosHAL_SetIrqState(true);
            ctx.tx.data = data;
            // Transmit data
            LuosHAL_ComTransmit(data, size);
        }
    }
}
/******************************************************************************
 * @brief Send ID to others service on network
 * @param None
 * @return lock status
 ******************************************************************************/
static uint8_t Transmit_GetLockStatus(void)
{
    if (ctx.tx.lock != true)
    {
        ctx.tx.lock |= LuosHAL_GetTxLockState();
    }
    return ctx.tx.lock;
}
/******************************************************************************
 * @brief finish transmit and try to launch a new one
 * @param None
 * @return None
 ******************************************************************************/
void Transmit_End(void)
{
    if (ctx.tx.status == TX_OK)
    {
        // A tx_task have been sucessfully transmitted
        nbrRetry         = 0;
        ctx.tx.collision = false;
        // Remove the task
        MsgAlloc_PullMsgFromTxTask();
    }
    else if (ctx.tx.status == TX_NOK)
    {
        // A tx_task failed
        nbrRetry++;
        // compute a delay before retry
        LuosHAL_ResetTimeout(20 * nbrRetry * (ctx.node.node_id + 1));
        // Lock the trasmission to be sure no one can send something from this node.
        ctx.tx.lock   = true;
        ctx.tx.status = TX_DISABLE;
        return;
    }
    // Try to send something if we need to.
    Transmit_Process();
}

/******************************************************************************
 * @file Legacy routing algorithm
 * @brief functions to make possible network detection
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "stamp.h"
#include "stdbool.h"
#include "robus.h"
#include "luos_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MAX_CONNECTION_RETRY 5

typedef enum
{
    POWER,
    CONNECT,
    RUN
} stamp_state_t;
stamp_state_t stamp_state = POWER;

// member specific variables
bool accept_connection_rcv = false;
uint32_t tick_start        = 0;
bool connect_retry         = false;
uint32_t connect_period    = 10;
uint8_t nb_retry           = 0;

// leader specific variables

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
static inline bool Stamp_Connect(void);

/******************************************************************************
 * @brief Stamp machine state
 * @param ll_container pointer to the detecting ll_container
 * @return The number of detected node.
 ******************************************************************************/
void Stamp_SetState(stamp_state_t state)
{
    stamp_state = state;
}

/******************************************************************************
 * @brief Stamp member machine state
 * @param None
 * @return None
 ******************************************************************************/
void Stamp_MemberLoop()
{
    switch (stamp_state)
    {
        case POWER:
            Stamp_SetState(CONNECT);
            break;
        case CONNECT:
            // have we received an ACCEPT_CONNECTION message ?
            if (accept_connection_rcv == true)
            {
                Stamp_SetState(RUN);
            }
            else
            {
                // try to connect
                Stamp_Connect();
            }
            break;
        case RUN:
            Stamp_MemberRun();
            break;
        default:
            Stamp_SetState(CONNECT);
            break;
    }
}

/******************************************************************************
 * @brief Stamp member machine state
 * @param None
 * @return None
 ******************************************************************************/
bool Stamp_Connect(void)
{
    msg_t connect_msg;

    while (nb_retry < MAX_CONNECTION_RETRY)
    {
        if (!connect_retry)
        {
            connect_retry  = true;
            connect_period = connect_period * 2;
            tick_start     = LuosHAL_GetSystick();
        }
        else
        {
            // send CONNECT_MEMBER message if timeout is reached
            if (LuosHAL_GetSystick() - tick_start > connect_period)
            {
                connect_msg.header.cmd         = CONNECT_MEMBER;
                connect_msg.header.size        = 0;
                connect_msg.header.target      = GROUP_LEADER_ID;
                connect_msg.header.target_mode = NODEIDACK;

                ll_container_t *node_container = Robus_GetContainer(0);
                Robus_SendMsg(node_container, &connect_msg);
            }
        }
    }
}

/******************************************************************************
 * @brief Stamp member message handler
 * @param None
 * @return None
 ******************************************************************************/
void Stamp_MemberMsgHandler(msg_t *msg)
{
    switch (msg->header.cmd)
    {
        case ACCEPT_CONNECTION:
            accept_connection_rcv = true;
            break;
        default:
            break;
    }
}

/******************************************************************************
 * @brief Stamp Leader machine state
 * @param None
 * @return None
 ******************************************************************************/
void Stamp_LeaderLoop()
{

    switch (stamp_state)
    {
        case POWER:
            Stamp_SetState(RUN);
            break;
        case RUN:
            Stamp_LeaderRun();
            break;
        default:
            break;
    }
}

/******************************************************************************
 * @brief Stamp leader message handler
 * @param None
 * @return None
 ******************************************************************************/
void Stamp_LeaderMsgHandler(msg_t *input)
{
}
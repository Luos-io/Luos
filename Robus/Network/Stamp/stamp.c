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
#define MAX_CONNECTION_RETRY 8

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
uint32_t connect_period    = 0;
uint8_t nb_retry           = 0;
volatile static uint16_t node_id;

bool test_flag = true;

// leader specific variables
bool member_connect_rcv  = false;
uint16_t current_node_id = FIRST_GROUP_MEMBER_ID;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
// member functions
static inline void Stamp_Connect(void);

// leader functions
static inline void Stamp_LeaderAccept(void);

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
void Stamp_MemberLoop(void)
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
                Stamp_Connect();
                // try to connect
            }
            break;
        case RUN:
            //Stamp_MemberRun();
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
void Stamp_Connect(void)
{
    msg_t connect_msg;

    if (nb_retry < MAX_CONNECTION_RETRY)
    {
        if (!connect_retry)
        {
            connect_retry = true;
            tick_start    = LuosHAL_GetSystick();

            if (nb_retry == 1)
            {
                connect_period = 10;
            }
        }
        else
        {
            // send CONNECT_MEMBER message if timeout is reached
            if (LuosHAL_GetSystick() - tick_start > connect_period)
            {
                connect_period = connect_period * 2;
                connect_retry  = false;
                nb_retry += 1;

                connect_msg.header.cmd         = CONNECT_MEMBER;
                connect_msg.header.size        = 0;
                connect_msg.header.target      = GROUP_LEADER_ID;
                connect_msg.header.target_mode = NODEID;

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
    node_t *node = Robus_GetNode();
    switch (msg->header.cmd)
    {
        case ACCEPT_CONNECTION:
            accept_connection_rcv = true;
            node->node_id         = msg->data[0];
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
void Stamp_LeaderLoop(void)
{
    node_t *node = Robus_GetNode();
    switch (stamp_state)
    {
        case POWER:
            // set member node id
            node->node_id = GROUP_LEADER_ID;
            // go to run state
            Stamp_SetState(RUN);
            break;
        case RUN:
            if (member_connect_rcv)
            {
                Stamp_LeaderAccept();
                // reset for next connection
                member_connect_rcv = false;
            }
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
void Stamp_LeaderAccept(void)
{
    msg_t connect_msg;
    connect_msg.header.cmd         = ACCEPT_CONNECTION;
    connect_msg.header.size        = sizeof(uint16_t);
    connect_msg.header.target      = 0;
    connect_msg.header.target_mode = NODEID;

    // set member ID
    connect_msg.data[0] = FIRST_GROUP_MEMBER_ID;

    ll_container_t *node_container = Robus_GetContainer(0);
    Robus_SendMsg(node_container, &connect_msg);
}

/******************************************************************************
 * @brief Stamp leader message handler
 * @param None
 * @return None
 ******************************************************************************/
void Stamp_LeaderMsgHandler(msg_t *msg)
{
    switch (msg->header.cmd)
    {
        case CONNECT_MEMBER:
            member_connect_rcv = true;
            break;
        default:
            break;
    }
}
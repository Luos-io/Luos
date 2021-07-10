/******************************************************************************
 * @file Stamp routing algorithm
 * @brief functions to make possible network detection
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef STAMP_H
#define STAMP_H

#include "robus_struct.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define GROUP_LEADER_ID 1

typedef enum
{
    CONNECT_MEMBER,
    ACCEPT_CONNECTION,
    DISCOVER_NEIGHBORS,
    TOPOLOGY_INFORMATION
} stamp_msg_t;

typedef enum
{
    GROUP_LEADER,
    GROUP_MEMBER
} stamp_role_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Stamp_MemberMsgHandler(msg_t *input);
void Stamp_Memberloop();

void Stamp_LeaderMsgHandler(msg_t *input);
void Stamp_Leaderloop();

#endif
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
void Stamp_MemberLoop(void);

void Stamp_LeaderMsgHandler(msg_t *input);
void Stamp_LeaderLoop(void);

#endif
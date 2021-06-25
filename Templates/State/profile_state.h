/******************************************************************************
 * @file Profile state
 * @brief state object managing a true false API
 * WARING : This h file should be only included by profile_*.c codes
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef PROFILE_STATE_H
#define PROFILE_STATE_H

#include "profile.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef struct
{
    bool state;
} profile_state_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Luos_LinkProfile(profile_t *profile, profile_state_t *profile_state, CONT_CB callback, const char *alias, revision_t revision);

#endif /* PROFILE_STATE_H_ */

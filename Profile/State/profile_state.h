/******************************************************************************
 * @file Profile state
 * @brief state object managing a true false API
 * WARING : This h file should be only included by profile_*.c codes
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef PROFILE_STATE_H
#define PROFILE_STATE_H

#include "profile_core.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define NB_CMD 1

// state profile data
typedef struct
{
    uint8_t access;
    bool value;
} state_cmd_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Luos_AddCommandToProfile(profile_cmd_t cmd_array[NB_CMD], state_cmd_t *state_cmd);
void Luos_LinkStateProfile(profile_core_t *profile, profile_cmd_t cmd_array[NB_CMD], CONT_CB callback);

#endif /* PROFILE_STATE_H_ */

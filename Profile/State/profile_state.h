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

#define CREATE_STATE_PROFILE(state_profile, cmd, state_cmd, name, rev) \
    Luos_AddCommandToProfile(cmd, &state_cmd);                         \
    Luos_LinkStateProfile(&state_profile, cmd, 0);                     \
    Luos_LaunchProfile(&state_profile, name, rev);

// state profile data
typedef struct
{
    uint8_t access;
    bool value;
} state_data_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Luos_AddCommandToProfile(profile_cmd_t *profile_cmd, state_data_t *state_data);
void Luos_LinkStateProfile(profile_core_t *profile, profile_cmd_t *profile_cmd, CONT_CB callback);

#endif /* PROFILE_STATE_H_ */
/******************************************************************************
 * @file Profile 
 * @brief container profile
 * WARING : This h file should be only included by user code or profile_*.h codes
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef PROFILE_CORE_H
#define PROFILE_CORE_H

#include <stdbool.h>
#include "luos.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

typedef struct
{
    uint32_t size;
    uint8_t *data;
} profile_data_t;

typedef struct
{
    luos_type_t type;
    luos_cmd_t cmd;
    access_t access;
    profile_data_t profile_data;
    CONT_CB profile_callback;
} profile_core_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
container_t *Luos_LaunchProfile(profile_core_t *profile, const char *alias, revision_t revision);

#endif /* PROFILE_CORE_H_ */

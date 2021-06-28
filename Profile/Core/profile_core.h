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
    void *data; // general pointer to handle all type of data structures
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

// start a service which has been already linked with a profile
container_t *Luos_LaunchProfile(profile_core_t *profile, const char *alias, revision_t revision);

// send message to the specified service
void Luos_SendProfile(const char *dest, container_t *src, luos_cmd_t cmd, const void *data, uint32_t size);

#endif /* PROFILE_CORE_H_ */

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
    void (*Init)(void *);
    void (*Handler)(container_t *, msg_t *);
    CONT_CB Callback;
} profile_ops_t;

typedef struct
{
    luos_type_t type;
    void *profile_data;
    profile_ops_t profile_ops;
} profile_core_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
profile_core_t *Luos_GetProfileFromContainer(container_t *container);
container_t *Luos_LaunchProfile(profile_core_t *profile, const char *alias, revision_t revision);
void Luos_SendProfile(char *dest, char *src, luos_cmd_t cmd, const void *data, uint32_t size);

#endif /* PROFILE_CORE_H_ */

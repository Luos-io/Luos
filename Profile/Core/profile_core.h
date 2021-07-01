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

#define ADD_CMD(profile_cmd, cmd_type, size, handler) \
    profile_cmd.cmd         = cmd_type;               \
    profile_cmd.cmd_size    = size;                   \
    profile_cmd.cmd_handler = handler;

typedef struct
{
    luos_cmd_t cmd;
    uint32_t cmd_size;
    void *cmd_handler; // general pointer to handle all type of data structures
} profile_cmd_t;

typedef struct
{
    void (*Init)(void);
    void (*Handler)(container_t *, msg_t *);
    CONT_CB Callback;
} profile_ops_t;

typedef struct
{
    luos_type_t type;
    profile_cmd_t *profile_cmd;
    profile_ops_t profile_ops;
} profile_core_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/

// start a service which has been already linked with a profile
container_t *Luos_LaunchProfile(profile_core_t *profile, const char *alias, revision_t revision);

// send message to the specified container
void Luos_SendProfile(char *dest, char *src, luos_cmd_t cmd, const void *data, uint32_t size);

#endif /* PROFILE_CORE_H_ */

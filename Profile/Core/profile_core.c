#include "profile_core.h"

/******************************************************************************
 * @brief get command from profile
 * @param profile pointer 
 * @return profile_cmd pointer
 ******************************************************************************/
profile_cmd_t *Luos_GetCmdFromProfile(profile_core_t *profile, uint8_t cmd_number)
{
    return &profile->profile_cmd[cmd_number];
}

/******************************************************************************
 * @brief get profile_context from container
 * @param Container 
 * @return profile_context pointer
 ******************************************************************************/
profile_core_t *Luos_GetProfileFromContainer(container_t *container)
{
    return (profile_core_t *)container->profile_context;
}

/******************************************************************************
 * @brief get profile_context from container
 * @param Container 
 * @return profile_context pointer
 ******************************************************************************/
void Luos_LinkProfile(profile_core_t *profile, luos_type_t type, profile_cmd_t *profile_cmd, profile_ops_t *profile_ops)
{
    // set general profile handler type
    profile->type = type;

    // set profile handler / callback functions
    profile->profile_ops.Init     = profile_ops->Init;
    profile->profile_ops.Handler  = profile_ops->Handler;
    profile->profile_ops.Callback = profile_ops->Callback;

    // link general profile handler to the command array
    profile->profile_cmd = profile_cmd;
}

/******************************************************************************
 * @brief Msg Handler call backed by Luos when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Luos_ProfileHandler(container_t *container, msg_t *msg)
{
    // get profile context out of the container
    profile_core_t *profile = Luos_GetProfileFromContainer(container);

    // auto-update profile data
    profile->profile_ops.Handler(container, msg);

    // call the profile callback if needed
    if (profile->profile_ops.Callback != 0)
    {
        profile->profile_ops.Callback(container, msg);
    }
}

/******************************************************************************
 * @brief Container creation following the template
 * @param profile_core_t the profile handler to launch
 * @param alias for the container string (15 caracters max).
 * @param revision FW for the container (tab[MajorVersion,MinorVersion,Patch])
 * @return None
 ******************************************************************************/
container_t *Luos_LaunchProfile(profile_core_t *profile, const char *alias, revision_t revision)
{
    // create container
    container_t *container = Luos_CreateContainer(Luos_ProfileHandler, profile->type, alias, revision);

    // link profile to container
    container->profile_context = (void *)profile;

    // call the profile init if needed
    if (profile->profile_ops.Init != 0)
    {
        profile->profile_ops.Init();
    }

    return container;
}

/******************************************************************************
 * @brief send data to the specified container
 * @param alias of the destination
 * @param alias of the source
 * @param cmd to send
 * @param data pointer to send
 * @param size of the data to send
 * @return None
 ******************************************************************************/
void Luos_SendProfile(char *dest, char *src, luos_cmd_t cmd, const void *data, uint32_t size)
{
    // find container_t pointer from alias
    container_t *container_src = Luos_GetContainerFromAlias(src);

    // get the container id of the destination from the routing table
    uint8_t id_dest = RoutingTB_IDFromAlias(dest);

    // send the message from the source to the dest
    msg_t msg;
    msg.header.target      = id_dest;
    msg.header.cmd         = cmd;
    msg.header.target_mode = IDACK;
    msg.header.size        = size;
    memcpy(msg.data, data, size);
    Luos_SendMsg(container_src, &msg);
}
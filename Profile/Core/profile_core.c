#include "profile_core.h"

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
 * @brief Msg Handler call backed by Luos when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Luos_ProfileHandler(container_t *container, msg_t *msg)
{
    profile_core_t *profile = Luos_GetProfileFromContainer(container);

    // auto-update profile data
    profile->profile_ops.Handler(container, msg);

    // call the profile callback when the container receive a message
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
    // call the profile init function if needed
    if (profile->profile_ops.Init != 0)
    {
        profile->profile_ops.Init((HANDLER *)profile);
    }

    container_t *container     = Luos_CreateContainer(Luos_ProfileHandler, profile->type, alias, revision);
    container->profile_context = (HANDLER *)profile;

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
    container_t *container_src = Luos_GetContainerFromAlias(src);

    uint8_t id_dest = RoutingTB_IDFromAlias(dest);

    msg_t msg;
    msg.header.target      = id_dest;
    msg.header.cmd         = cmd;
    msg.header.target_mode = IDACK;
    msg.header.size        = size;
    memcpy(msg.data, data, size);
    Luos_SendMsg(container_src, &msg);
}
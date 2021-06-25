#include "profile.h"

/******************************************************************************
 * @brief function converting Luos messages into data and reverse.
 * @param container the target container
 * @param msg the received message
 * @param state_struct the data struct to update
 * @return None
 ******************************************************************************/
void Profile_Handler(container_t *container, msg_t *msg, profile_t *profile)
{
    // if someone sends us general ASK_PUB_CMD then publish data
    if ((msg->header.cmd == ASK_PUB_CMD) && ((profile->access == READ_WRITE_ACCESS) || (profile->access == READ_ONLY_ACCESS)))
    {
        // fill the message infos
        msg_t pub_msg;
        pub_msg.header.cmd         = IO_STATE;
        pub_msg.header.target_mode = msg->header.target_mode;
        pub_msg.header.target      = msg->header.source;

        // fill with profile data
        pub_msg.header.size = profile->profile_data.size;
        memcpy(&pub_msg.data, profile->profile_data.data, profile->profile_data.size);

        // send message
        Luos_SendMsg(container, &pub_msg);
    }
    // if someone sends us specific profile command, copy to the profile data
    if ((msg->header.cmd == profile->cmd) && ((profile->access == READ_WRITE_ACCESS) || (profile->access == WRITE_ONLY_ACCESS)))
    {
        // save received data in profile data
        memcpy(profile->profile_data.data, &msg->data, profile->profile_data.size);
    }
}

/******************************************************************************
 * @brief Msg Handler call backed by Luos when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Profile_MsgHandler(container_t *container, msg_t *msg)
{
    // get profile context out of the container
    profile_t *profile = (profile_t *)container->profile_context;

    // update profile data
    Profile_Handler(container, msg, profile);

    // call the profile callback if needed
    if (profile->profile_callback != 0)
    {
        profile->profile_callback(container, msg);
    }
}

/******************************************************************************
 * @brief Container creation following the template
 * @param cont_cb is an optional user callback called on every massage for this container
 * @param state_struct template object pointer
 * @param alias for the container string (15 caracters max).
 * @param revision FW for the container (tab[MajorVersion,MinorVersion,Patch])
 * @return None
 ******************************************************************************/
container_t *Luos_LaunchProfile(profile_t *profile, const char *alias, revision_t revision)
{
    container_t *container     = Luos_CreateContainer(Profile_MsgHandler, profile->type, alias, revision);
    container->profile_context = (void *)profile;
    return container;
}

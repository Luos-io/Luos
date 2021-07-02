#include "profile_state.h"

/******************************************************************************
 * @brief function converting Luos messages into data and reverse.
 * @param container the target container
 * @param msg the received message
 * @return None
 ******************************************************************************/
void Luos_StateHandler(container_t *container, msg_t *msg)
{
    profile_core_t *profile        = Luos_GetProfileFromContainer(container);
    profile_state_t *profile_state = (profile_state_t *)profile->profile_data;

    if ((msg->header.cmd == ASK_PUB_CMD) && ((profile_state->access == READ_WRITE_ACCESS) || (profile_state->access == READ_ONLY_ACCESS)))
    {
        // fill the message infos
        msg_t pub_msg;
        pub_msg.header.cmd         = IO_STATE;
        pub_msg.header.target_mode = msg->header.target_mode;
        pub_msg.header.target      = msg->header.source;
        pub_msg.header.size        = sizeof(bool);
        memcpy(&pub_msg.data, &profile_state->state, sizeof(bool));
        Luos_SendMsg(container, &pub_msg);
    }
    if ((msg->header.cmd == IO_STATE) && ((profile_state->access == READ_WRITE_ACCESS) || (profile_state->access == WRITE_ONLY_ACCESS)))
    {
        memcpy(&profile_state->state, &msg->data, sizeof(bool));
    }
}

/******************************************************************************
 * @brief Lik state profile to the general profile handler
 * @param profile handler, 
 * @param profile_state handler, 
 * @param callback used by the profile
 * @return None
 ******************************************************************************/
void Luos_LinkStateProfile(profile_core_t *profile, profile_state_t *profile_state, CONT_CB callback)
{
    // set general profile handler type
    profile->type = STATE_TYPE;

    // link general profile handler to the profile data structure
    profile->profile_data = (HANDLER *)profile_state;

    // set profile handler / callback functions
    profile->profile_ops.Init     = 0;
    profile->profile_ops.Handler  = Luos_StateHandler;
    profile->profile_ops.Callback = callback;
}
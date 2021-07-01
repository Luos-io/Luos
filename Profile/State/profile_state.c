#include "profile_state.h"

/******************************************************************************
 * @brief Profile Initialisation function
 * @param None
 * @return None
 ******************************************************************************/
void Luos_StateInit(void)
{
}

/******************************************************************************
 * @brief function converting Luos messages into data and reverse.
 * @param container the target container
 * @param msg the received message
 * @return None
 ******************************************************************************/
void Luos_StateHandler(container_t *container, msg_t *msg)
{
    // get profile context out of the container
    profile_core_t *profile = (profile_core_t *)container->profile_context;

    // get profiles_cmd structures from profile handler
    profile_cmd_t *state_cmd = &profile->profile_cmd[0];
    state_data_t *state_data = (state_data_t *)state_cmd->cmd_handler;

    // if someone sends us general ASK_PUB_CMD then publish data
    if ((msg->header.cmd == ASK_PUB_CMD) && ((state_data->access == READ_WRITE_ACCESS) || (state_data->access == READ_ONLY_ACCESS)))
    {
        // fill the message infos
        msg_t pub_msg;
        pub_msg.header.cmd         = state_cmd->cmd;
        pub_msg.header.target_mode = msg->header.target_mode;
        pub_msg.header.target      = msg->header.source;

        // fill with profile data
        pub_msg.header.size = state_cmd->cmd_size;
        memcpy(&pub_msg.data, state_data, state_cmd->cmd_size);

        // send message
        Luos_SendMsg(container, &pub_msg);
    }
    // if someone sends us state command, copy to the profile data
    if ((msg->header.cmd == state_cmd->cmd) && ((state_data->access == READ_WRITE_ACCESS) || (state_data->access == WRITE_ONLY_ACCESS)))
    {
        // save received data in profile data
        memcpy(state_data, &msg->data, state_cmd->cmd_size);
    }
}

/******************************************************************************
 * @brief Lik state profile to the general profile handler
 * @param cmd array used by the profile, 
 * @param state_cmd structure used by state profile 
 * @return None
 ******************************************************************************/
void Luos_AddCommandToProfile(profile_cmd_t *profile_cmd, state_data_t *state_data)
{
    ADD_CMD(profile_cmd[0], IO_STATE, sizeof(state_data_t), (void *)state_data);
}

/******************************************************************************
 * @brief Lik state profile to the general profile handler
 * @param profile handler, 
 * @param cmd array used by the profile, 
 * @param callback used by the profile
 * @return None
 ******************************************************************************/
void Luos_LinkStateProfile(profile_core_t *profile, profile_cmd_t *profile_cmd, CONT_CB callback)
{
    // set general profile handler type
    profile->type = STATE_TYPE;

    // set profile handler / callback functions
    profile->profile_ops.Init     = Luos_StateInit;
    profile->profile_ops.Handler  = Luos_StateHandler;
    profile->profile_ops.Callback = callback;

    // link general profile handler to the command array
    profile->profile_cmd = profile_cmd;
}
#include "profile_state.h"

/******************************************************************************
 * @brief function converting Luos messages into data and reverse.
 * @param container the target container
 * @param msg the received message
 * @return None
 ******************************************************************************/
void Luos_StateHandler(container_t *container, msg_t *msg)
{
    profile_core_t *profile = Luos_GetProfileFromContainer(container);

    profile_cmd_t *state_cmd = Luos_GetCmdFromProfile(profile, 0);
    state_data_t *state_data = (state_data_t *)state_cmd->cmd_handler;

    if ((msg->header.cmd == ASK_PUB_CMD) && ((state_data->access == READ_WRITE_ACCESS) || (state_data->access == READ_ONLY_ACCESS)))
    {
        // fill the message infos
        msg_t pub_msg;
        pub_msg.header.cmd         = state_cmd->cmd;
        pub_msg.header.target_mode = msg->header.target_mode;
        pub_msg.header.target      = msg->header.source;
        pub_msg.header.size        = state_cmd->cmd_size;
        memcpy(&pub_msg.data, state_data, state_cmd->cmd_size);
        Luos_SendMsg(container, &pub_msg);
    }
    if ((msg->header.cmd == state_cmd->cmd) && ((state_data->access == READ_WRITE_ACCESS) || (state_data->access == WRITE_ONLY_ACCESS)))
    {
        memcpy(state_data, &msg->data, state_cmd->cmd_size);
    }
}

/******************************************************************************
 * @brief Lik state profile to the general profile handler
 * @param cmd array used by the profile, 
 * @param state_data structure used by state profile 
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
    profile_ops_t state_ops = {
        .Init     = 0,
        .Handler  = Luos_StateHandler,
        .Callback = callback};

    Luos_LinkProfile(profile, STATE_TYPE, profile_cmd, &state_ops);
}
#include "profile_state.h"

/******************************************************************************
 * @brief Lik state profile to the general profile handler
 * @param profile_t *profile, 
 * @param profile_state_t profile_state, 
 * @param const char *alias, 
 * @param revision_t revision
 * @return None
 ******************************************************************************/
void Luos_LinkProfile(profile_t *profile, profile_state_t *profile_state, CONT_CB callback)
{
    profile->type              = STATE_TYPE;
    profile->cmd               = IO_STATE;
    profile->access            = READ_WRITE_ACCESS;
    profile->profile_data.size = sizeof(profile_state_t);
    profile->profile_data.data = (uint8_t *)profile_state;
    profile->profile_callback  = callback;
}
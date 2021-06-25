#include "profile_state.h"

/******************************************************************************
 * @brief Lik state profile to the general profile handler
 * @param profile_t *profile, 
 * @param profile_state_t profile_state, 
 * @param const char *alias, 
 * @param revision_t revision
 * @return None
 ******************************************************************************/
void Luos_LinkProfile(profile_t *profile, profile_state_t *profile_state, CONT_CB callback, const char *alias, revision_t revision)
{
    int i = 0;

    profile->type = STATE_TYPE;
    profile->cmd  = IO_STATE;
    for (i = 0; i < MAX_ALIAS_SIZE - 1; i++)
    {
        profile->alias[i] = alias[i];
        if (profile->alias[i] == '\0')
            break;
    }
    profile->alias[i] = '\0';
    memcpy(&profile->revision, &revision, sizeof(revision_t));
    profile->profile_data.size = sizeof(profile_state_t);
    profile->profile_data.data = (uint8_t *)profile_state;
    if (callback != 0)
    {
        profile->profile_callback = callback;
    }
}
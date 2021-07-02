#include "profile_voltage.h"

/******************************************************************************
 * @brief function converting Luos messages into data and reverse.
 * @param container the target container
 * @param msg the received message
 * @return None
 ******************************************************************************/
void Luos_voltageHandler(container_t *container, msg_t *msg)
{
    profile_core_t *profile            = Luos_GetProfileFromContainer(container);
    profile_voltage_t *voltage_profile = (profile_voltage_t *)profile->profile_data;

    if ((msg->header.cmd == ASK_PUB_CMD) && ((voltage_profile->access == READ_WRITE_ACCESS) || (voltage_profile->access == READ_ONLY_ACCESS)))
    {
        // fill the message infos
        msg_t pub_msg;
        pub_msg.header.target_mode = msg->header.target_mode;
        pub_msg.header.target      = msg->header.source;
        if (voltage_profile->control.rec)
        {
            LUOS_ASSERT(voltage_profile->signal.data_ptr != 0);
            // send back a record stream
            pub_msg.header.cmd = VOLTAGE;
            Luos_SendStreaming(container, &pub_msg, &voltage_profile->signal);
        }
        else
        {
            ElectricOD_VoltageToMsg(&voltage_profile->voltage, &pub_msg);
            Luos_SendMsg(container, &pub_msg);
        }
    }
    if ((msg->header.cmd == VOLTAGE) && ((voltage_profile->access == READ_WRITE_ACCESS) || (voltage_profile->access == WRITE_ONLY_ACCESS)))
    {
        if (msg->header.size == sizeof(voltage_t))
        {
            // Get the voltage
            ElectricOD_VoltageFromMsg(&voltage_profile->voltage, msg);
        }
        else
        {
            LUOS_ASSERT(voltage_profile->signal.data_ptr != 0);
            // this is a signal, save it into ring buffer.
            Luos_ReceiveStreaming(container, msg, &voltage_profile->signal);
            // values will be converted one by one during trajectory management.
        }
    }
    if (msg->header.cmd == TIME)
    {
        // save time in ms
        TimeOD_TimeFromMsg((time_luos_t *)&voltage_profile->sampling_period, msg);
        return;
    }
}

/******************************************************************************
 * @brief Link voltage profile to the general profile handler
 * @param profile handler, 
 * @param profile_voltage handler, 
 * @param callback used by the profile
 * @return None
 ******************************************************************************/
void Luos_LinkVoltageProfile(profile_core_t *profile, profile_voltage_t *profile_voltage, CONT_CB callback)
{
    // set general profile handler type
    profile->type = VOLTAGE_TYPE;

    // link general profile handler to the profile data structure
    profile->profile_data = (void *)profile_voltage;

    // set profile handler / callback functions
    profile->profile_ops.Init     = 0;
    profile->profile_ops.Handler  = Luos_voltageHandler;
    profile->profile_ops.Callback = callback;
}
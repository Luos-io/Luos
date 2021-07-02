#include "profile_motor.h"
#include "math.h"

/******************************************************************************
 * @brief function converting Luos messages into data and reverse.
 * @param container the target container
 * @param msg the received message
 * @return None
 ******************************************************************************/
void Luos_MotorHandler(container_t *container, msg_t *msg)
{
    profile_core_t *profile        = Luos_GetProfileFromContainer(container);
    profile_motor_t *profile_motor = (profile_motor_t *)profile->profile_data;

    switch (msg->header.cmd)
    {
        case ASK_PUB_CMD:
        {
            msg_t pub_msg;
            pub_msg.header.target_mode = msg->header.target_mode;
            pub_msg.header.target      = msg->header.source;
            if (profile_motor->mode.current)
            {
                ElectricOD_CurrentToMsg(&profile_motor->current, &pub_msg);
                Luos_SendMsg(container, &pub_msg);
            }
            if (profile_motor->mode.temperature)
            {
                TemperatureOD_TemperatureToMsg(&profile_motor->temperature, &pub_msg);
                Luos_SendMsg(container, &pub_msg);
            }
        }
        break;
        case RATIO:
        {
            // get the motor power ratio
            RatioOD_RatioFromMsg(&profile_motor->power, msg);
            if (profile_motor->power > 100.0)
                profile_motor->power = 100.0;
            if (profile_motor->power < -100.0)
                profile_motor->power = -100.0;
        }
        break;
        case RATIO_LIMIT:
        {
            // set the motor power ratio limit
            RatioOD_RatioFromMsg(&profile_motor->limit_ratio, msg);
            profile_motor->limit_ratio = fabs(profile_motor->limit_ratio);
            if (profile_motor->limit_ratio > 100.0)
                profile_motor->limit_ratio = 100.0;
        }
        break;
        case CURRENT_LIMIT:
        {
            // set the motor current limit
            ElectricOD_CurrentFromMsg((current_t *)&profile_motor->limit_current, msg);
            profile_motor->limit_current = fabs(profile_motor->limit_current);
        }
        break;
        case TEMPERATURE_LIMIT:
        {
            // set the motor power ratio limit
            TemperatureOD_TemperatureFromMsg(&profile_motor->limit_temperature, msg);
            profile_motor->limit_temperature = fabs(profile_motor->limit_temperature);
        }
        break;
        case PARAMETERS:
        {
            // check the message size
            // fill the message infos
            memcpy((void *)&profile_motor->mode, msg->data, sizeof(motor_mode_t));
        }
        break;
    }
}

/******************************************************************************
 * @brief Link motor profile to the general profile handler
 * @param profile handler, 
 * @param profile_motor handler, 
 * @param callback used by the profile
 * @return None
 ******************************************************************************/
void Luos_LinkMotorProfile(profile_core_t *profile, profile_motor_t *profile_motor, CONT_CB callback)
{
    // set general profile handler type
    profile->type = MOTOR_TYPE;

    // link general profile handler to the profile data structure
    profile->profile_data = (HANDLER *)profile_motor;

    // set profile handler / callback functions
    profile->profile_ops.Init     = 0;
    profile->profile_ops.Handler  = Luos_MotorHandler;
    profile->profile_ops.Callback = callback;
}
/******************************************************************************
 * @file Profile voltage
 * @brief voltage object
 * WARING : This h file should be only included by profile_*.c codes
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef PROFILE_VOLTAGE_H_
#define PROFILE_VOLTAGE_H_

#include "profile_core.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef struct
{
    access_t access;
    voltage_t voltage;

    //streaming
    control_t control;
    streaming_channel_t signal;
    time_luos_t sampling_period;
} profile_voltage_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Luos_LinkVoltageProfile(profile_core_t *profile, profile_voltage_t *profile_voltage, CONT_CB callback);

#endif /* PROFILE_VOLTAGE_H_ */
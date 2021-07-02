/******************************************************************************
 * @file Profile servo motor
 * @brief servo motor profile
 * WARING : This h file should be only included by profile_*.c codes
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef PROFILE_SERVO_MOTOR_H_
#define PROFILE_SERVO_MOTOR_H_

#include "profile_motor.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*
 * motor mode structure
 */
typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            // drive modes
            uint16_t mode_compliant : 1;
            uint16_t mode_power : 1;
            uint16_t mode_torque : 1;
            uint16_t mode_angular_speed : 1;
            uint16_t mode_angular_position : 1;
            uint16_t mode_linear_speed : 1;
            uint16_t mode_linear_position : 1;

            // measures modes
            uint16_t angular_position : 1;
            uint16_t angular_speed : 1;
            uint16_t linear_position : 1;
            uint16_t linear_speed : 1;
            uint16_t current : 1;
            uint16_t temperature : 1;
        };
        uint8_t unmap[2];
    };
} servo_motor_mode_t;
/*
 * motor data structure
 */
typedef struct
{
    // H bridge and motor object
    profile_motor_t motor;

    // drive
    servo_motor_mode_t mode;
    angular_position_t target_angular_position;
    angular_speed_t target_angular_speed;

    // limits
    angular_position_t limit_angular_position[2];
    angular_speed_t limit_angular_speed[2];

    // measures
    angular_position_t angular_position;
    angular_speed_t angular_speed;
    linear_position_t linear_position;
    linear_speed_t linear_speed;

    //configs
    float motor_reduction;
    float resolution;
    linear_position_t wheel_diameter;
    asserv_pid_t position_pid;
    asserv_pid_t speed_pid;

    //streaming
    control_t control;
    streaming_channel_t trajectory;
    streaming_channel_t measurement;
    time_luos_t sampling_period;
} profile_servo_motor_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Luos_LinkServoMotorProfile(profile_core_t *profile, profile_servo_motor_t *profile_servo_motor, CONT_CB callback);

#endif /* PROFILE_SERVO_MOTOR_H_ */
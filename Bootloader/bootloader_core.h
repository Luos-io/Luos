/******************************************************************************
 * @file Bootloader
 * @brief Bootloader functionnalities for luos framework
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "robus_struct.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define BOOTLOADER_MODE  0x00
#define APPLICATION_MODE 0x01

#define BOOTLOADER_RCV_COMMAND   0x01
#define BOOTLOADER_SND_COMMAND   0x10
#define BOOTLOADER_ERROR_COMMAND 0x20
typedef enum
{
    BOOTLOADER_IDLE,
    BOOTLOADER_START = BOOTLOADER_RCV_COMMAND,
    BOOTLOADER_STOP,
    BOOTLOADER_READY,
    BOOTLOADER_ERASE,
    BOOTLOADER_BIN_CHUNK,
    BOOTLOADER_BIN_END,
    BOOTLOADER_CRC_TEST,
    BOOTLOADER_READY_RESP = BOOTLOADER_SND_COMMAND,
    BOOTLOADER_BIN_HEADER_RESP,
    BOOTLOADER_ERASE_RESP,
    BOOTLOADER_BIN_CHUNK_RESP,
    BOOTLOADER_BIN_END_RESP,
    BOOTLOADER_CRC_RESP,
    BOOTLOADER_ERROR_SIZE = BOOTLOADER_ERROR_COMMAND,
} bootloader_cmd_t;

typedef enum
{
    BOOTLOADER_START_STATE,
    BOOTLOADER_READY_STATE,
    BOOTLOADER_STOP_STATE,
    BOOTLOADER_ERASE_STATE,
    BOOTLOADER_BIN_CHUNK_STATE,
    BOOTLOADER_CRC_TEST_STATE,
} bootloader_state_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief Main function used by the bootloader app
 ******************************************************************************/
void LuosBootloader_Run(void);

/******************************************************************************
 * @brief function used by Luos to send message to the bootloader
 ******************************************************************************/
void LuosBootloader_MsgHandler(msg_t *);

#endif /* BOOTLOADER_H */
/******************************************************************************
 * @file Bootloader
 * @brief Bootloader functionnalities for luos framework
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "bootloader_core.h"
#include "luos_hal.h"
#include "luos.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifdef BOOTLOADER_CONFIG
#define MAX_FRAME_SIZE (MAX_DATA_MSG_SIZE - 1)
#define BUFFER_SIZE    0x800 // 2kB buffer to store received data
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
static bootloader_cmd_t bootloader_cmd;

#ifdef BOOTLOADER_CONFIG
static bootloader_state_t bootloader_state = BOOTLOADER_START_STATE;
// variables use to save binary data in flash
static uint8_t bootloader_data[MAX_FRAME_SIZE];
static uint16_t bootloader_data_size = 0;

uint32_t flash_addr = APP_ADDRESS;
uint8_t data_buff[(uint16_t)BUFFER_SIZE];
uint16_t data_index     = 0;
uint16_t residual_space = (uint16_t)BUFFER_SIZE;
uint32_t nb_bytes       = 0;
#endif

/*******************************************************************************
 * Function
 ******************************************************************************/
#ifdef BOOTLOADER_CONFIG
static uint8_t LuosBootloader_GetMode(void);
static void LuosBootloader_DeInit(void);
static void LuosBootloader_JumpToApp(void);
static void LuosBootloader_SetNodeID(void);
static inline uint8_t LuosBootloader_IsEnoughSpace(uint32_t);
static inline void LuosBootloader_EraseMemory(void);
static inline void LuosBootloader_ProcessData(void);
static inline void LuosBootloader_SaveLastData(void);
static void LuosBootloader_SendResponse(bootloader_cmd_t);
static void LuosBootloader_SendCrc(bootloader_cmd_t, uint8_t, uint8_t);
static void LuosBootloader_SetState(bootloader_state_t);
static void LuosBootloader_Task(void);
#else
static void LuosBootloader_SaveNodeID(void);
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief read the boot mode in flash memory
 * @param None
 * @return boot_mode
 ******************************************************************************/
uint8_t LuosBootloader_GetMode(void)
{
    return LuosHAL_GetMode();
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief DeInit peripherals in bootloader
 * @param None
 * @return None
 ******************************************************************************/
void LuosBootloader_DeInit(void)
{
    LuosHAL_DeInit();
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief launch application from the bootloader
 * @param None
 * @return None
 ******************************************************************************/
void LuosBootloader_JumpToApp(void)
{
    LuosHAL_JumpToApp(APP_ADDRESS);
}
#endif

/******************************************************************************
 * @brief Save node id in flash
 * @param None
 * @return None
 ******************************************************************************/
void LuosBootloader_SaveNodeID(void)
{
    node_t *node     = Robus_GetNode();
    uint16_t node_id = node->node_id;

    LuosHAL_SaveNodeID(node_id);
}

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief Set node id with data saved in flash
 * @param None
 * @return None
 ******************************************************************************/
void LuosBootloader_SetNodeID(void)
{
    uint16_t node_id = LuosHAL_GetNodeID();
    node_t *node     = Robus_GetNode();

    node->node_id = node_id;
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief Set node id with data saved in flash
 * @param None
 * @return None
 ******************************************************************************/
uint8_t LuosBootloader_IsEnoughSpace(uint32_t binary_size)
{
    uint32_t free_space = FLASH_END - APP_ADDRESS;
    if (free_space > binary_size)
    {
        return 0x01;
    }
    else
    {
        return 0x00;
    }
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief process binary data received from the gate
 * @param None 
 * @return None
 ******************************************************************************/
void LuosBootloader_EraseMemory(void)
{
    LuosHAL_EraseMemory(APP_ADDRESS, nb_bytes);
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief process binary data received from the gate
 * @param None 
 * @return None
 ******************************************************************************/
void LuosBootloader_ProcessData(void)
{
    if (residual_space >= bootloader_data_size)
    {
        // there is enough space in the current page
        // fill the current page with data
        memcpy(&data_buff[data_index], bootloader_data, bootloader_data_size);

        // update data_index and residual_space
        data_index += bootloader_data_size;
        residual_space -= bootloader_data_size;
    }
    else
    {
        // complete the current page buffer
        memcpy(&data_buff[data_index], bootloader_data, residual_space);

        // save the completed page in flash memory
        LuosHAL_ProgramFlash(flash_addr, (uint16_t)BUFFER_SIZE, data_buff);

        // prepare next page buffer
        flash_addr += BUFFER_SIZE;
        data_index = 0;
        memset(data_buff, 0xFF, (uint16_t)BUFFER_SIZE);

        // copy the remaining data in the new page buffer
        memcpy(&data_buff[data_index], &bootloader_data[residual_space], bootloader_data_size - residual_space);

        // update data_index and residual_space
        data_index     = bootloader_data_size - residual_space;
        residual_space = (uint16_t)BUFFER_SIZE - data_index;
    }
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief Save the current page when BIN_END command is received 
 * @param None 
 * @return None
 ******************************************************************************/
void LuosBootloader_SaveLastData(void)
{
    LuosHAL_ProgramFlash(flash_addr, (uint16_t)BUFFER_SIZE, data_buff);
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief compute crc 8 for each data
 * @param data pointer, data len 
 * @return crc
 ******************************************************************************/
void crc8(const uint8_t *data, uint8_t *crc, uint16_t polynome)
{
    uint16_t dbyte = 0;
    uint16_t mix   = 0;

    // zero padding data
    dbyte = (uint16_t)*data << 8;
    // add initial value
    dbyte ^= (uint16_t)*crc << 8;
    // right shift polynomial key
    polynome = polynome << 8;
    // shift and xor if necessary
    for (uint8_t j = 0; j < 8; ++j)
    {
        mix   = dbyte & 0x8000;
        dbyte = (dbyte << 1);
        if (mix)
            dbyte ^= polynome;
    }

    *crc = (uint8_t)(dbyte >> 8);
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief compute crc for the whole binary
 * @param data pointer, data len 
 * @return crc
 ******************************************************************************/
uint8_t compute_crc(void)
{
    uint8_t crc   = 0x00; // initial crc value
    uint8_t data  = 0x00;
    uint16_t poly = 0x0007;

    uint32_t data_counter  = 0;
    uint8_t data_index     = 0;
    uint32_t *data_address = (uint32_t *)APP_ADDRESS;
    uint32_t data_flash    = 0;

    while (data_counter < nb_bytes)
    {
        // read 1 word in flash memory
        data_flash = *data_address;
        // read 4 bytes in flash memory
        for (data_index = 0; data_index < 4; data_index++)
        {
            if (data_counter < nb_bytes)
            {
                data = (uint8_t)(data_flash >> (8 * data_index));
                // compute crc
                crc8(&data, &crc, poly);
                data_counter += 1;
            }
        }
        // update data address
        data_address += 1;
    }

    return crc;
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief Send response to the gate
 * @param None
 * @return None
 ******************************************************************************/
void LuosBootloader_SendCrc(bootloader_cmd_t response, uint8_t data, uint8_t size)
{
    msg_t ready_msg;
    ready_msg.header.cmd         = BOOTLOADER_RESP;
    ready_msg.header.target_mode = NODEIDACK;
    ready_msg.header.target      = 1; // always send to the gate wich launched the detection
    ready_msg.header.size        = 2 * sizeof(uint8_t);
    ready_msg.data[0]            = response;
    ready_msg.data[1]            = data;

    Luos_SendMsg(0, &ready_msg);
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief Send response to the gate
 * @param None
 * @return None
 ******************************************************************************/
void LuosBootloader_SendResponse(bootloader_cmd_t response)
{
    msg_t ready_msg;
    ready_msg.header.cmd         = BOOTLOADER_RESP;
    ready_msg.header.target_mode = NODEIDACK;
    ready_msg.header.target      = 1; // always send to the gate wich launched the detection
    ready_msg.header.size        = sizeof(uint8_t);
    ready_msg.data[0]            = response;
    Luos_SendMsg(0, &ready_msg);
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief set state of the bootloader
 * @param state 
 * @return None
 ******************************************************************************/
void LuosBootloader_SetState(bootloader_state_t state)
{
    bootloader_state = state;
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief Bootloader machine state
 * @param None
 * @return None
 ******************************************************************************/
void LuosBootloader_Task(void)
{
    switch (bootloader_state)
    {
        case BOOTLOADER_START_STATE:
            // set ID node saved in flash
            LuosBootloader_SetNodeID();
            // go to ready state
            LuosBootloader_SetState(BOOTLOADER_READY_STATE);
            break;

        case BOOTLOADER_READY_STATE:
            // if STOP_CMD, restart the node
            if (bootloader_cmd == BOOTLOADER_STOP)
            {
                LuosBootloader_SetState(BOOTLOADER_STOP_STATE);
            }
            // if READY_CMD, continue BOOTLOADER process
            if (bootloader_cmd == BOOTLOADER_READY)
            {
                // save binary length
                memcpy(&nb_bytes, &bootloader_data, sizeof(uint32_t));
                // check free space in flash
                if (LuosBootloader_IsEnoughSpace(nb_bytes))
                {
                    // send READY response
                    LuosBootloader_SendResponse(BOOTLOADER_READY_RESP);
                    // go to HEADER state
                    LuosBootloader_SetState(BOOTLOADER_ERASE_STATE);
                }
                else
                {
                    // send READY response
                    LuosBootloader_SendResponse(BOOTLOADER_ERROR_SIZE);
                }
            }
            break;

        case BOOTLOADER_ERASE_STATE:

            if (bootloader_cmd == BOOTLOADER_ERASE)
            {
                // erase flash memory
                LuosBootloader_EraseMemory();
                // send READY response
                LuosBootloader_SendResponse(BOOTLOADER_ERASE_RESP);
                // go to HEADER state
                LuosBootloader_SetState(BOOTLOADER_BIN_CHUNK_STATE);
            }

            break;

        case BOOTLOADER_BIN_CHUNK_STATE:

            if (bootloader_cmd == BOOTLOADER_BIN_CHUNK)
            {
                // reset bootloader cmd to avoid looping in this portion of the code
                bootloader_cmd = BOOTLOADER_IDLE;
                // handle binary data
                LuosBootloader_ProcessData();
                // send ack to the Host
                LuosBootloader_SendResponse(BOOTLOADER_BIN_CHUNK_RESP);
            }

            if (bootloader_cmd == BOOTLOADER_BIN_END)
            {
                // save the current page in flash memory
                LuosBootloader_SaveLastData();

                // send ack to the Host
                LuosBootloader_SendResponse(BOOTLOADER_BIN_END_RESP);

                // go to BIN_END state
                LuosBootloader_SetState(BOOTLOADER_CRC_TEST_STATE);
            }
            break;

        case BOOTLOADER_CRC_TEST_STATE:

            if (bootloader_cmd == BOOTLOADER_CRC_TEST)
            {
                uint8_t crc = compute_crc();
                // send ack to the Host
                LuosBootloader_SendCrc(BOOTLOADER_CRC_RESP, crc, sizeof(uint16_t));

                // go to READY state
                LuosBootloader_SetState(BOOTLOADER_READY_STATE);
            }
            break;

        case BOOTLOADER_STOP_STATE:
            // save boot_mode in flash
            LuosHAL_SetMode(APPLICATION_MODE);
            // wait for the command to be send to all nodes
            LuosHAL_Delay(1000);
            // reboot the node
            LuosHAL_Reboot();
            break;

        default:
            break;
    }
}
#endif

#ifdef BOOTLOADER_CONFIG
/******************************************************************************
 * @brief bootloader app
 * @param None
 * @return None
 ******************************************************************************/
void LuosBootloader_Run(void)
{
    switch (LuosBootloader_GetMode())
    {
        case APPLICATION_MODE:
            // boot the application programmed in dedicated flash partition
            LuosBootloader_DeInit();
            LuosBootloader_JumpToApp();
            break;

        case BOOTLOADER_MODE:
        default:
            while (1)
            {
                LUOS_RUN()
                LuosBootloader_Task();
            }
            break;
    }
}
#endif

/******************************************************************************
 * @brief Message handler called from luos library
 * @param data received from luos network
 * @return None
 ******************************************************************************/
void LuosBootloader_MsgHandler(msg_t *input)
{
    bootloader_cmd = input->data[0];

    switch (bootloader_cmd)
    {
#ifndef BOOTLOADER_CONFIG
        case BOOTLOADER_START:
            // We're in the app,
            // set bootloader mode, save node ID and reboot
            LuosHAL_SetMode(BOOTLOADER_MODE);
            LuosBootloader_SaveNodeID();
            LuosHAL_Reboot();
            break;
#else
        case BOOTLOADER_STOP:
        case BOOTLOADER_READY:
        case BOOTLOADER_BIN_CHUNK:
        case BOOTLOADER_BIN_END:
        case BOOTLOADER_CRC_TEST:
            // we're in the bootloader,
            // process cmd and data
            bootloader_data_size = input->header.size - sizeof(char);
            memcpy(bootloader_data, &(input->data[1]), bootloader_data_size);
            break;
#endif
        default:
            break;
    }
}

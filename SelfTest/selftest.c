/******************************************************************************
 * @file luos selftest
 * @brief function to test luos library
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "selftest.h"
#include "luos_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

typedef enum
{
    KO = -1,
    OK = 1
} result_t;

volatile uint8_t rx_flag  = 0;
volatile uint8_t ptp_flag = 0;

static inline void selftest_init(void);
static inline result_t selftest_com(void);
static inline result_t selftest_ptp(void);

/******************************************************************************
 * @brief set ptp selftest flag
 * @param None
 * @return None
 ******************************************************************************/
void selftest_SetPtpFlag(void)
{
    ptp_flag = 1;
}

/******************************************************************************
 * @brief set rx selftest flag
 * @param None
 * @return None
 ******************************************************************************/
void selftest_SetRxFlag(void)
{
    rx_flag = 1;
}

/******************************************************************************
 * @brief Initialization of the luos library
 * @param None
 * @return None
 ******************************************************************************/
void selftest_init(void)
{
    Luos_Init();
}

/******************************************************************************
 * @brief Test of the Tx / Rx lines
 * @param None
 * @return None
 ******************************************************************************/
result_t selftest_com(void)
{
    msg_t msg;
    msg.header.target      = 1;
    msg.header.target_mode = NODEID;
    msg.header.cmd         = IO_STATE;
    msg.header.size        = 5 * sizeof(uint8_t);
    msg.data[0]            = 0xAA;
    msg.data[1]            = 0x55;
    msg.data[2]            = 0xAA;
    msg.data[3]            = 0x55;
    msg.data[4]            = 0xAA;

    Luos_SendMsg(0, &msg);

    uint32_t tickstart = LuosHAL_GetSystick();
    while (!rx_flag)
    {
        if ((LuosHAL_GetSystick() - tickstart) > 3000)
        {
            return KO;
        }
    }

    return OK;
}

/******************************************************************************
 * @brief Test of the PTP lines
 * @param None
 * @return None
 ******************************************************************************/
result_t selftest_ptp(void)
{
    LuosHAL_SetPTPDefaultState(0);
    if (LuosHAL_GetPTPState(1))
        return KO;

    LuosHAL_PushPTP(0);
    if (!LuosHAL_GetPTPState(1))
        return KO;

    if (!ptp_flag)
    {
        return KO;
    }

    return OK;
}

/******************************************************************************
 * @brief Test of the PTP lines
 * @param None
 * @return None
 ******************************************************************************/
void selftest_run(void)
{
    result_t result = OK;

    selftest_init();

    if (result == OK)
    {
        result = selftest_com();
    }

    if (result == OK)
    {
        result = selftest_ptp();
    }
}
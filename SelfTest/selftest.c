/******************************************************************************
 * @file luos selftest
 * @brief function to test luos library
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "selftest.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

typedef enum
{
    KO = -1,
    OK = 1
} result_t;

static inline result_t selftest_init(void);
static inline result_t selftest_com(void);
static inline result_t selftest_ptp(void);

/******************************************************************************
 * @brief Initialization of the luos library
 * @param None
 * @return None
 ******************************************************************************/
result_t selftest_init(void)
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
    msg.header.target      = 0;
    msg.header.target_mode = NODEIDACK;
    msg.header.cmd         = IO_STATE;
    msg.header.size        = 5 * sizeof(uint8_t);
    msg.data[0]            = 0xAA;
    msg.data[1]            = 0x55;
    msg.data[2]            = 0xAA;
    msg.data[3]            = 0x55;
    msg.data[4]            = 0xAA;
    Luos_SendMsg(0, msg);
}

/******************************************************************************
 * @brief Test of the PTP lines
 * @param None
 * @return None
 ******************************************************************************/
result_t selftest_ptp(void)
{
}

/******************************************************************************
 * @brief Test of the PTP lines
 * @param None
 * @return None
 ******************************************************************************/
void selftest_run(void)
{
    selftest_init();

    selftest_com();
    selftest_ptp();
}
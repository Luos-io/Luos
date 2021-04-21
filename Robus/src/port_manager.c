/******************************************************************************
 * @file detection
 * @brief detection state machine.
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <stdbool.h>
#include "port_manager.h"
#include "transmission.h"
#include "context.h"
#include "luos_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef enum
{
    POKE,
    RELEASE
} PortState_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/
PortState_t Port_ExpectedState = POKE;
/*******************************************************************************
 * Function
 ******************************************************************************/

static void PortMng_Reset(void);
/******************************************************************************
 * @brief init the portManager state machine
 * @param None
 * @return None
 ******************************************************************************/
void PortMng_Init(void)
{
    PortMng_Reset();
    // Reinit ll_container id
    for (uint8_t i = 0; i < ctx.ll_container_number; i++)
    {
#ifdef SNIFFER_H //do not change the sniffer's id
        if (ctx.ll_container_table[i].id != 0xFFFF)
            ctx.ll_container_table[i].id = DEFAULTID;
#else
        ctx.ll_container_table[i].id = DEFAULTID;
#endif /* SNIFFER_H */
    }
    // Reinit port table
    for (uint8_t port = 0; port < NBR_PORT; port++)
    {
        ctx.node.port_table[port] = 0;
    }
}
/******************************************************************************
 * @brief PTP interrupt handler
 * @param port id
 * @return None
 ******************************************************************************/
void PortMng_PtpHandler(uint8_t PortNbr)
{
    if (Port_ExpectedState == RELEASE)
    {
        Port_ExpectedState = POKE;
        ctx.port.keepLine = false;
        // Check if every line have been poked and poke it if not
        for (uint8_t port = 0; port < NBR_PORT; port++)
        {
            if (ctx.node.port_table[port] == 0)
            {
                return;
            }
        }
        PortMng_Reset();
    }
#ifndef SNIFFER_H   //in case of a sniffer we do not notify our presence
    else if (Port_ExpectedState == POKE)
    {
        // we receive a poke, pull the line to notify your presence
        LuosHAL_PushPTP(PortNbr);
        ctx.port.activ = PortNbr;
    }
#endif  /* SNIFFER_H */
}
/******************************************************************************
 * @brief Poke
 * @param port id
 * @return None
 ******************************************************************************/
uint8_t PortMng_PokePort(uint8_t PortNbr)
{
    // push the ptp line
    LuosHAL_PushPTP(PortNbr);
    // wait a little just to be sure everyone can read it
    uint32_t start_tick = LuosHAL_GetSystick();
    while (LuosHAL_GetSystick() - start_tick < 2)
        ;
    // release the ptp line
    LuosHAL_SetPTPDefaultState(PortNbr);
    while (LuosHAL_GetSystick() - start_tick < 3)
        ;
    // Save port as empty by default
    ctx.node.port_table[PortNbr] = 0xFFFF;
    // read the line state
    if (LuosHAL_GetPTPState(PortNbr))
    {
        // Someone reply, reverse the detection to wake up on line release
        LuosHAL_SetPTPReverseState(PortNbr);
        Port_ExpectedState = RELEASE;
        // Port poked by node
        ctx.port.activ = PortNbr;
        ctx.port.keepLine = true;
        return 1;
    }
    // Nobodies reply to our poke
    return 0;
}
/******************************************************************************
 * @brief detect the next module by poke ptp line
 * @param None
 * @return true if a port have been poke else false
 ******************************************************************************/
error_return_t PortMng_PokeNextPort(void)
{
    for (uint8_t port = 0; port < NBR_PORT; port++)
    {
        if (ctx.node.port_table[port] == 0)
        {
            // this port have not been poked
            if (PortMng_PokePort(port))
            {
                return SUCCEED;
            }
            else
            {
                // nobody is here
                ctx.node.port_table[port] = 0xFFFF;
            }
        }
    }
    PortMng_Reset();
    return FAILED;
}
/******************************************************************************
 * @brief reinit the detection state machine
 * @param None
 * @return None
 ******************************************************************************/
void PortMng_Reset(void)
{
    ctx.port.keepLine = false;
    ctx.port.activ = NBR_PORT;
    Port_ExpectedState = POKE;
    // if it is finished reset all lines
    for (uint8_t port = 0; port < NBR_PORT; port++)
    {
        LuosHAL_SetPTPDefaultState(port);
    }
}

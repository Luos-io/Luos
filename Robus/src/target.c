/******************************************************************************
 * @file target
 * @brief multicast protocole description
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "target.h"
#include "stdbool.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief lookink for a bank of target in multicast mode
 * @param service in multicast
 * @param multicast bank
 * @return Error
 ******************************************************************************/
uint8_t Trgt_MulticastTargetBank(ll_service_t *ll_service, uint16_t val)
{
    unsigned char i;
    for (i = 0; i < ll_service->max_multicast_target; i++)
    {
        if (ll_service->multicast_target_bank[i] == val)
            return true;
    }
    return false;
}
/******************************************************************************
 * @brief add a target to the bank
 * @param service in multicast
 * @param target to add
 * @return Error
 ******************************************************************************/
void Trgt_AddMulticastTarget(ll_service_t *ll_service, uint16_t target)
{
    ll_service->multicast_target_bank[ll_service->max_multicast_target++] = target;
}

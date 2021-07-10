/******************************************************************************
 * @file Legacy routing algorithm
 * @brief functions to make possible network detection
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "legacy.h"
#include "robus_struct.h"

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
 * @brief Initiliaze network port
 * @param None
 * @return None
 ******************************************************************************/
void Network_PortInit(void)
{
    Legacy_PortInit();
}

/******************************************************************************
 * @brief Start a topology detection procedure
 * @param ll_container pointer to the detecting ll_container
 * @return The number of detected node.
 ******************************************************************************/
uint16_t Network_TopologyDetection(ll_container_t *ll_container)
{
    return Legacy_TopologyDetection(ll_container);
}
/******************************************************************************
 * @brief run the procedure allowing to detect the next nodes on the next port
 * @param ll_container pointer to the detecting ll_container
 * @return None.
 ******************************************************************************/
void Network_MsgHandler(msg_t *input)
{
    return Legacy_MsgHandler(input);
}
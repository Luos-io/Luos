/******************************************************************************
 * @file Legacy routing algorithm
 * @brief functions to make possible network detection
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "legacy.h"
#include "stamp.h"
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
#ifdef LEGACY_DETECTION
    Legacy_PortInit();
#endif

#ifdef STAMP_DETECTION

#endif
}

/******************************************************************************
 * @brief Start a topology detection procedure
 * @param ll_container pointer to the detecting ll_container
 * @return The number of detected node.
 ******************************************************************************/
uint16_t Network_TopologyDetection(ll_container_t *ll_container)
{
#ifdef LEGACY_DETECTION
    return Legacy_TopologyDetection(ll_container);
#endif

#ifdef STAMP_DETECTION
    return 0;
#endif
}

/******************************************************************************
 * @brief run the procedure allowing to detect the next nodes on the next port
 * @param ll_container pointer to the detecting ll_container
 * @return None.
 ******************************************************************************/
void Network_MsgHandler(msg_t *input)
{
#ifdef LEGACY_DETECTION
    Legacy_MsgHandler(input);
#endif

#ifdef STAMP_DETECTION
    Stamp_MemberMsgHandler(input);
#endif
}

/******************************************************************************
 * @brief run the procedure allowing to detect the next nodes on the next port
 * @param ll_container pointer to the detecting ll_container
 * @return None.
 ******************************************************************************/
void Network_Loop(void)
{
#ifdef STAMP_DETECTION
    Stamp_MemberLoop();
#endif
}
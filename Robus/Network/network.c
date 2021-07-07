/******************************************************************************
 * @file routing algorithm
 * @brief functions to make possible network detection
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "network.h"
#include "msg_alloc.h"
#include <stdbool.h>
#include "luos_utils.h"
#include <string.h>
#include "luos_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            uint16_t prev_nodeid;
            uint16_t nodeid;
        };
        uint8_t unmap[sizeof(uint16_t) * 2];
    };
} node_bootstrap_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint16_t last_node = 0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static error_return_t Network_ResetNetworkDetection(ll_container_t *ll_container);
static error_return_t Network_DetectNextNodes(ll_container_t *ll_container);

/******************************************************************************
 * @brief Start a topology detection procedure
 * @param ll_container pointer to the detecting ll_container
 * @return The number of detected node.
 ******************************************************************************/
uint16_t Network_TopologyDetection(ll_container_t *ll_container)
{
    uint8_t redetect_nb = 0;
    bool detect_enabled = true;

    while (detect_enabled)
    {
        detect_enabled = false;

        // Reset all detection state of containers on the network
        Network_ResetNetworkDetection(ll_container);

        // setup local node
        ctx.node.node_id = 1;
        last_node        = 1;

        // setup sending ll_container
        ll_container->id = 1;

        if (Network_DetectNextNodes(ll_container) == FAILED)
        {
            // check the number of retry we made
            LUOS_ASSERT((redetect_nb <= 4));
            // Detection fail, restart it
            redetect_nb++;
            detect_enabled = true;
        }
    }

    return last_node;
}
/******************************************************************************
 * @brief reset all module port states
 * @param ll_container pointer to the detecting ll_container
 * @return The number of detected node.
 ******************************************************************************/
error_return_t Network_ResetNetworkDetection(ll_container_t *ll_container)
{
    msg_t msg;
    uint8_t try_nbr = 0;

    msg.header.target      = BROADCAST_VAL;
    msg.header.target_mode = BROADCAST;
    msg.header.cmd         = RESET_DETECTION;
    msg.header.size        = 0;

    do
    {
        //msg send not blocking
        Robus_SendMsg(ll_container, &msg);
        //need to wait until tx msg before clear msg alloc
        while (MsgAlloc_TxAllComplete() != SUCCEED)
            ;

        MsgAlloc_Init(NULL);

        // wait for some 2ms to be sure all previous messages are received and treated
        uint32_t start_tick = LuosHAL_GetSystick();
        while (LuosHAL_GetSystick() - start_tick < 2)
            ;
        try_nbr++;
    } while ((MsgAlloc_IsEmpty() != SUCCEED) || (try_nbr > 5));

    ctx.node.node_id = 0;
    Robus_ContainerIdInit();
    PortMng_Init();
    if (try_nbr < 5)
    {
        return SUCCEED;
    }

    return FAILED;
}
/******************************************************************************
 * @brief run the procedure allowing to detect the next nodes on the next port
 * @param ll_container pointer to the detecting ll_container
 * @return None.
 ******************************************************************************/
error_return_t Network_DetectNextNodes(ll_container_t *ll_container)
{
    // Lets try to poke other nodes
    while (PortMng_PokeNextPort() == SUCCEED)
    {
        // There is someone here
        // Clear spotted dead container detection
        ll_container->dead_container_spotted = 0;
        // Ask an ID  to the detector container.
        msg_t msg;
        msg.header.target_mode = IDACK;
        msg.header.target      = 1;
        msg.header.cmd         = WRITE_NODE_ID;
        msg.header.size        = 0;
        Robus_SendMsg(ll_container, &msg);
        // Wait the end of transmission
        while (MsgAlloc_TxAllComplete() == FAILED)
            ;
        // Check if there is a failure on transmission
        if (ll_container->dead_container_spotted != 0)
        {
            // Message transmission failure
            // Consider this port unconnected
            ctx.node.port_table[ctx.port.activ] = 0xFFFF;
            ctx.port.activ                      = NBR_PORT;
            ctx.port.keepLine                   = false;
            continue;
        }

        // when Robus loop will receive the reply it will store and manage the new node_id and send it to the next node.
        // We just have to wait the end of the treatment of the entire branch
        uint32_t start_tick = LuosHAL_GetSystick();
        while (ctx.port.keepLine)
        {
            Robus_Loop();
            if (LuosHAL_GetSystick() - start_tick > 1000)
            {
                // topology detection is too long, we should abort it and restart
                return FAILED;
            }
        }
    }
    return SUCCEED;
}

/******************************************************************************
 * @brief run the procedure allowing to detect the next nodes on the next port
 * @param ll_container pointer to the detecting ll_container
 * @return None.
 ******************************************************************************/
void Network_MsgHandler(msg_t *input)
{
    msg_t output_msg;
    node_bootstrap_t node_bootstrap;
    ll_container_t *ll_container = Recep_GetConcernedLLContainer(&input->header);
    // Depending on the size of the received data we have to do different things
    switch (input->header.size)
    {
        case 0:
            // Someone asking us a new node id (we are the detecting module)
            // Increase the number of node_nb and send it back
            last_node++;
            output_msg.header.cmd         = WRITE_NODE_ID;
            output_msg.header.size        = sizeof(uint16_t);
            output_msg.header.target      = input->header.source;
            output_msg.header.target_mode = NODEIDACK;
            memcpy(output_msg.data, (void *)&last_node, sizeof(uint16_t));
            Robus_SendMsg(ll_container, &output_msg);
            break;
        case 2:
            // This is a node id for the next node.
            // This is a reply to our request to generate the next node id.
            // This node_id is the one after the currently poked branch.
            // We need to save this ID as a connection on a port
            memcpy((void *)&ctx.node.port_table[ctx.port.activ], (void *)&input->data[0], sizeof(uint16_t));
            // Now we can send it to the next node
            memcpy((void *)&node_bootstrap.nodeid, (void *)&input->data[0], sizeof(uint16_t));
            node_bootstrap.prev_nodeid    = ctx.node.node_id;
            output_msg.header.cmd         = WRITE_NODE_ID;
            output_msg.header.size        = sizeof(node_bootstrap_t);
            output_msg.header.target      = 0;
            output_msg.header.target_mode = NODEIDACK;
            memcpy((void *)&output_msg.data[0], (void *)&node_bootstrap.unmap[0], sizeof(node_bootstrap_t));
            Robus_SendMsg(ll_container, &output_msg);
            break;
        case sizeof(node_bootstrap_t):
            if (ctx.node.node_id != 0)
            {
                ctx.node.node_id = 0;
                MsgAlloc_Init(NULL);
            }
            // This is a node bootstrap information.
            memcpy((void *)&node_bootstrap.unmap[0], (void *)&input->data[0], sizeof(node_bootstrap_t));
            ctx.node.node_id                    = node_bootstrap.nodeid;
            ctx.node.port_table[ctx.port.activ] = node_bootstrap.prev_nodeid;
            // Continue the topology detection on our other ports.
            Network_DetectNextNodes(ll_container);
        default:
            break;
    }
}
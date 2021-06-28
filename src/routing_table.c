/******************************************************************************
 * @file routingTable
 * @brief routing table descrption function
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <routing_table.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "luos_hal.h"
#include "context.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ALIAS_SIZE 15
/*******************************************************************************
 * Variables
 ******************************************************************************/
routing_table_t routing_table[MAX_RTB_ENTRY];
volatile uint16_t last_container           = 0;
volatile uint16_t last_routing_table_entry = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void RoutingTB_AddNumToAlias(char *alias, uint8_t num);
static uint16_t RoutingTB_BigestID(void);
static uint16_t RoutingTB_BigestNodeID(void);
static bool RoutingTB_WaitRoutingTable(container_t *container, msg_t *intro_msg);

static void RoutingTB_Generate(container_t *container, uint16_t nb_node);
static void RoutingTB_Share(container_t *container, uint16_t nb_node);

// ************************ routing_table search tools ***************************

/******************************************************************************
 * @brief  Return an id from alias
 * @param pointer to alias
 * @return ID or Error
 ******************************************************************************/
uint16_t RoutingTB_IDFromAlias(char *alias)
{
    if (*alias != -1)
    {
        for (int i = 0; i <= last_routing_table_entry; i++)
        {
            if (routing_table[i].mode == CONTAINER)
            {
                if (strcmp(routing_table[i].alias, alias) == 0)
                {
                    return routing_table[i].id;
                }
            }
        }
    }
    return 0xFFFF;
}
/******************************************************************************
 * @brief  Return an id from type
 * @param type of container look at
 * @return ID or Error
 ******************************************************************************/
uint16_t RoutingTB_IDFromType(luos_type_t type)
{
    for (int i = 0; i <= last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == CONTAINER)
        {
            if (type == routing_table[i].type)
            {
                return routing_table[i].id;
            }
        }
    }
    return 0xFFFF;
}
/******************************************************************************
 * @brief  Return a Nodeid from container id
 * @param id of container
 * @return NODEID or Error
 ******************************************************************************/
uint16_t RoutingTB_NodeIDFromID(uint16_t id)
{
    for (uint16_t i = RoutingTB_GetContainerID(id); i >= 0; i--)
    {
        if (routing_table[i].mode == NODE)
        {
            return routing_table[i].node_id;
        }
    }
    return 0;
}
/******************************************************************************
 * @brief  Return an id from container
 * @param container look at
 * @return ID or Error
 ******************************************************************************/
uint16_t RoutingTB_IDFromContainer(container_t *container)
{
    // make sure route table is clean before sharing id
    if (last_routing_table_entry == 0)
    {
        return 0;
    }
    return (uint16_t)container->ll_container->id;
}
/******************************************************************************
 * @brief  Return container Alias from ID
 * @param id container look at
 * @return pointer to string or Error
 ******************************************************************************/
char *RoutingTB_AliasFromId(uint16_t id)
{
    for (int i = 0; i <= last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == CONTAINER)
        {
            if (id == routing_table[i].id)
            {
                return routing_table[i].alias;
            }
        }
    }
    return (char *)0;
}
/******************************************************************************
 * @brief  Return container type from ID
 * @param id container look at
 * @return pointer container or Error
 ******************************************************************************/
luos_type_t RoutingTB_TypeFromID(uint16_t id)
{
    for (uint16_t i = 0; i <= last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == CONTAINER)
        {
            if (routing_table[i].id == id)
            {
                return routing_table[i].type;
            }
        }
    }
    return -1;
}
/******************************************************************************
 * @brief  Return container type from alias
 * @param alias container look at
 * @return pointer to container or Error
 ******************************************************************************/
luos_type_t RoutingTB_TypeFromAlias(char *alias)
{
    uint16_t id = RoutingTB_IDFromAlias(alias);
    return RoutingTB_TypeFromID(id);
}
/******************************************************************************
 * @brief  Create a string from a container type
 * @param type of container look at
 * @return pointer to string or Error
 ******************************************************************************/
char *RoutingTB_StringFromType(luos_type_t type)
{
    switch (type)
    {
        case STATE_MOD:
            return "State";
            break;
        case COLOR_MOD:
            return "Color";
            break;
        case SERVO_MOD:
            return "Servo";
            break;
        case ANGLE_MOD:
            return "Angle";
            break;
        case DISTANCE_MOD:
            return "DistanceSensor";
            break;
        case GATE_MOD:
            return "Gate";
            break;
        case DYNAMIXEL_MOD:
            return "DynamixelMotor";
            break;
        case STEPPER_MOD:
            return "Stepper";
            break;
        case DCMOTOR_MOD:
            return "DCMotor";
            break;
            break;
        case IMU_MOD:
            return "Imu";
            break;
        case LIGHT_MOD:
            return "LightSensor";
            break;
        case CONTROLLER_MOTOR_MOD:
            return "ControllerMotor";
        case VOID_MOD:
            return "Void";
            break;
        case LOAD_MOD:
            return "Load";
            break;
        case VOLTAGE_MOD:
            return "Voltage";
            break;
        default:
            return "Unknown";
            break;
    }
}
/******************************************************************************
 * @brief  check if the container is a sensor or not
 * @param container look at
 * @return Error
 ******************************************************************************/
uint8_t RoutingTB_ContainerIsSensor(luos_type_t type)
{
    if ((type == ANGLE_MOD)
        || (type == STATE_MOD)
        || (type == DYNAMIXEL_MOD)
        || (type == DISTANCE_MOD)
        || (type == IMU_MOD)
        || (type == LOAD_MOD)
        || (type == CONTROLLER_MOTOR_MOD)
        || (type == VOLTAGE_MOD)
        || (type == LIGHT_MOD))
    {
        return 1;
    }
    return 0;
}
/******************************************************************************
 * @brief  return bigest container ID in list
 * @param None
 * @return ID
 ******************************************************************************/
static uint16_t RoutingTB_BigestID(void)
{
    uint16_t max_id = 0;
    for (uint16_t i = 0; i < last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == CONTAINER)
        {
            if (routing_table[i].id > max_id)
            {
                max_id = routing_table[i].id;
            }
        }
    }
    return max_id;
}
/******************************************************************************
 * @brief  return bigest node ID in list
 * @param None
 * @return ID
 ******************************************************************************/
static uint16_t RoutingTB_BigestNodeID(void)
{
    uint16_t max_id = 0;
    for (uint16_t i = 0; i < last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == NODE)
        {
            if (routing_table[i].node_id > max_id)
            {
                max_id = routing_table[i].node_id;
            }
        }
    }
    return max_id;
}

/******************************************************************************
 * @brief  get number of a node on network
 * @param None
 * @return number of node
 ******************************************************************************/
uint16_t RoutingTB_GetNodeNB(void)
{
    uint16_t node_nb = 0;
    for (uint16_t i = 0; i <= last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == NODE)
        {
            node_nb++;
        }
    }
    return node_nb - 1;
}
/******************************************************************************
 * @brief  get ID of node on network
 * @param pointer to index of Node
 * @return node_id
 ******************************************************************************/
uint16_t RoutingTB_GetNodeID(uint16_t index)
{
    return routing_table[index].node_id;
}

/******************************************************************************
 * @brief  get number of a container on the network
 * @param None
 * @return container number
 ******************************************************************************/
uint16_t RoutingTB_GetContainerNB(void)
{
    uint16_t container_nb = 0;
    for (uint16_t i = 0; i <= last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == CONTAINER)
        {
            container_nb++;
        }
    }
    return container_nb - 1;
}
/******************************************************************************
 * @brief  get ID of container on the network
 * @param routing table index
 * @return ID
 ******************************************************************************/
uint16_t RoutingTB_GetContainerID(uint16_t index)
{
    return routing_table[index].id;
}

/******************************************************************************
 * @brief  get mode of a routing table entry
 * @param routing table index
 * @return mode
 ******************************************************************************/
entry_mode_t RoutingTB_GetMode(uint16_t index)
{
    return routing_table[index].mode;
}

// ********************* routing_table management tools ************************

/******************************************************************************
 * @brief compute entry number
 * @param None
 * @return None
 ******************************************************************************/
void RoutingTB_ComputeRoutingTableEntryNB(void)
{
    for (uint16_t i = 0; i < MAX_RTB_ENTRY; i++)
    {
        if (routing_table[i].mode == CONTAINER)
        {
            last_container = routing_table[i].id;
        }
        if (routing_table[i].mode == CLEAR)
        {
            last_routing_table_entry = i;
            return;
        }
    }
    // Routing table space is full.
    last_routing_table_entry = MAX_RTB_ENTRY - 1;
}
/******************************************************************************
 * @brief manage container name increment to never have same alias
 * @param alias to change
 * @param nb to add
 * @return None
 ******************************************************************************/
static void RoutingTB_AddNumToAlias(char *alias, uint8_t num)
{
    uint8_t intsize = 1;
    if (num > 9)
    {
        // The string size of num is 2
        intsize = 2;
    }
    if (num > 99) //only 2 digit are alowed when add alias number
    {
        // This is probably a mistake, put an error into the alias
        memset(alias, 0, ALIAS_SIZE);
        memcpy(alias, "error", strlen("error"));
        return;
    }
    // Change size to fit into 15 characters
    if (strlen(alias) > (ALIAS_SIZE - intsize))
    {
        alias[(ALIAS_SIZE - intsize)] = '\0';
    }
    else
    {
        alias[strlen(alias)] = '\0';
    }
    // Add a number at the end of the alias
    sprintf(alias, "%s%d", alias, num);
}
/******************************************************************************
 * @brief time out to receive en route table from
 * @param container receive
 * @param intro msg in route table
 * @return None
 ******************************************************************************/
static bool RoutingTB_WaitRoutingTable(container_t *container, msg_t *intro_msg)
{
    const uint8_t timeout    = 15; // timeout in ms
    const uint16_t entry_bkp = last_routing_table_entry;
    Luos_SendMsg(container, intro_msg);
    uint32_t timestamp = LuosHAL_GetSystick();
    while ((LuosHAL_GetSystick() - timestamp) < timeout)
    {
        // If this request is for a container in this board allow him to respond.
        Luos_Loop();
        if (entry_bkp != last_routing_table_entry)
        {
            return true;
        }
    }
    return false;
}
/******************************************************************************
 * @brief Generate Complete route table with local route table receive
 * @param container in node
 * @param node number on network
 * @return None
 ******************************************************************************/
static void RoutingTB_Generate(container_t *container, uint16_t nb_node)
{
    // Asks for introduction for every found node (even the one detecting).
    uint16_t try_nb       = 0;
    uint16_t last_node_id = RoutingTB_BigestNodeID();
    uint16_t last_cont_id = 0;
    msg_t intro_msg;
    while ((last_node_id < nb_node) && (try_nb < nb_node))
    {
        try_nb++;
        intro_msg.header.cmd         = RTB_CMD;
        intro_msg.header.target_mode = NODEIDACK;
        // Target next unknown node
        intro_msg.header.target = last_node_id + 1;
        // set the first container id it can use
        intro_msg.header.size = 2;
        last_cont_id          = RoutingTB_BigestID() + 1;
        memcpy(intro_msg.data, &last_cont_id, sizeof(uint16_t));
        // Ask to introduce and wait for a reply
        if (!RoutingTB_WaitRoutingTable(container, &intro_msg))
        {
            // We don't get the answer
            nb_node = last_node_id;
            break;
        }
        last_node_id = RoutingTB_BigestNodeID();
    }
    // Check Alias duplication.
    uint16_t nb_mod = RoutingTB_BigestID();
    for (uint16_t id = 1; id <= nb_mod; id++)
    {
        uint16_t found_id = RoutingTB_IDFromAlias(RoutingTB_AliasFromId(id));
        if ((found_id != id) & (found_id != -1))
        {
            // The found_id don't match with the actual ID of the container because the alias already exist
            // Find the new alias to give him
            uint8_t annotation              = 1;
            char base_alias[MAX_ALIAS_SIZE] = {0};
            memcpy(base_alias, RoutingTB_AliasFromId(id), MAX_ALIAS_SIZE);
            // Add a number after alias in routing table
            RoutingTB_AddNumToAlias(RoutingTB_AliasFromId(id), annotation++);
            // check another time if this alias is already used
            while (RoutingTB_IDFromAlias(RoutingTB_AliasFromId(id)) != id)
            {
                // This alias is already used.
                // Remove the number previously setuped by overwriting it with the base_alias
                memcpy(RoutingTB_AliasFromId(id), base_alias, MAX_ALIAS_SIZE);
                RoutingTB_AddNumToAlias(RoutingTB_AliasFromId(id), annotation++);
            }
        }
    }
}
/******************************************************************************
 * @brief Send the complete route table to each node on the network
 * @param container who send
 * @param node number on network
 * @return None
 ******************************************************************************/
static void RoutingTB_Share(container_t *container, uint16_t nb_node)
{
    // send route table to each nodes. Routing tables are commonly usable for each containers of a node.
    msg_t intro_msg;
    intro_msg.header.cmd         = RTB_CMD;
    intro_msg.header.target_mode = NODEIDACK;

    for (uint16_t i = 2; i <= nb_node; i++) //don't send to ourself
    {
        intro_msg.header.target = i;
        Luos_SendData(container, &intro_msg, routing_table, (last_routing_table_entry * sizeof(routing_table_t)));
    }
}

/******************************************************************************
 * @brief Detect all containers and create a route table with it.
 * If multiple containers have the same name it will be changed with a number in it
 * Automatically at the end this function create a list of sensors id
 * @param container who send
 * @return None
 ******************************************************************************/
void RoutingTB_DetectContainers(container_t *container)
{
    // Starts the topology detection.
    uint16_t nb_node = Robus_TopologyDetection(container->ll_container);
    // clear the routing table.
    RoutingTB_Erase();
    // Generate the routing_table
    RoutingTB_Generate(container, nb_node);
    // We have a complete routing table now share it with others.
    RoutingTB_Share(container, nb_node);
}
/******************************************************************************
 * @brief entry in routable node with associate container
 * @param route table
 * @param node structure
 * @return None
 ******************************************************************************/
void RoutingTB_ConvertNodeToRoutingTable(routing_table_t *entry, node_t *node)
{
    // Check if the NBR_PORT config is too high to fit into routing table.
    LUOS_ASSERT(sizeof(node_t) <= (sizeof(routing_table_t) - 1));
    memset(entry, 0, sizeof(routing_table_t));
    entry->mode = NODE;
    memcpy(entry->unmap_data, node->unmap, sizeof(node_t));
}
/******************************************************************************
 * @brief entry in routable container associate to a node
 * @param route table
 * @param container in node
 * @return None
 ******************************************************************************/
void RoutingTB_ConvertContainerToRoutingTable(routing_table_t *entry, container_t *container)
{
    entry->type = container->ll_container->type;
    entry->id   = container->ll_container->id;
    entry->mode = CONTAINER;
    for (uint8_t i = 0; i < MAX_ALIAS_SIZE; i++)
    {
        entry->alias[i] = container->alias[i];
    }
}
/******************************************************************************
 * @brief remove an entire node
 * @param route table
 * @return None
 ******************************************************************************/
void RoutingTB_RemoveNode(uint16_t nodeid)
{
    // instead of removing a node just remove all the container in it to make it unusable
    // We could add a param (CONTROL for example) to declare the node as STOP
    // find the node
    for (uint16_t i = 0; i < last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == NODE)
        {
            if (routing_table[i].node_id == nodeid)
            {
                i++;
                // We find our node remove all containers
                while (routing_table[i].mode == CONTAINER)
                {
                    RoutingTB_RemoveOnRoutingTable(RoutingTB_GetContainerID(i));
                }
                return;
            }
        }
    }
}
/******************************************************************************
 * @brief remove an entry from routing_table
 * @param id of container
 * @return None
 ******************************************************************************/
void RoutingTB_RemoveOnRoutingTable(uint16_t id)
{
    // find the container
    for (uint16_t i = 0; i < last_routing_table_entry; i++)
    {
        if ((routing_table[i].mode == CONTAINER) && (routing_table[i].id == id))
        {
            LUOS_ASSERT(i < last_routing_table_entry);
            memcpy(&routing_table[i], &routing_table[i + 1], sizeof(routing_table_t) * (last_routing_table_entry - (i + 1)));
            last_routing_table_entry--;
            memset(&routing_table[last_routing_table_entry], 0, sizeof(routing_table_t));
            return;
        }
    }
}
/******************************************************************************
 * @brief eras erouting_table
 * @param None
 * @return None
 ******************************************************************************/
void RoutingTB_Erase(void)
{
    memset(routing_table, 0, sizeof(routing_table));
    last_container           = 0;
    last_routing_table_entry = 0;
}
/******************************************************************************
 * @brief get routing_table
 * @param None
 * @return route table
 ******************************************************************************/
routing_table_t *RoutingTB_Get(void)
{
    return routing_table;
}
/******************************************************************************
 * @brief return the last ID registered into the routing_table
 * @param None
 * @return last container ID
 ******************************************************************************/
uint16_t RoutingTB_GetLastContainer(void)
{
    return (uint16_t)last_container;
}
/******************************************************************************
 * @brief return the last ID registered into the routing_table
 * @param index of container
 * @return Last entry
 ******************************************************************************/
uint16_t RoutingTB_GetLastEntry(void)
{
    return (uint16_t)last_routing_table_entry;
}

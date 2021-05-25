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
volatile uint16_t last_service             = 0;
volatile uint16_t last_routing_table_entry = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void RoutingTB_AddNumToAlias(char *alias, uint8_t num);
static uint16_t RoutingTB_BigestID(void);
static uint16_t RoutingTB_BigestNodeID(void);
static bool RoutingTB_WaitRoutingTable(service_t *service, msg_t *intro_msg);

static void RoutingTB_Generate(service_t *service, uint16_t nb_node);
static void RoutingTB_Share(service_t *service, uint16_t nb_node);

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
            if (routing_table[i].mode == SERVICE)
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
 * @param type of service look at
 * @return ID or Error
 ******************************************************************************/
uint16_t RoutingTB_IDFromType(luos_type_t type)
{
    for (int i = 0; i <= last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == SERVICE)
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
 * @brief  Return an id from service
 * @param service look at
 * @return ID or Error
 ******************************************************************************/
uint16_t RoutingTB_IDFromService(service_t *service)
{
    // make sure route table is clean before sharing id
    if (last_routing_table_entry == 0)
    {
        return 0;
    }
    return (uint16_t)service->ll_service->id;
}
/******************************************************************************
 * @brief  Return service Alias from ID
 * @param id service look at
 * @return pointer to string or Error
 ******************************************************************************/
char *RoutingTB_AliasFromId(uint16_t id)
{
    for (int i = 0; i <= last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == SERVICE)
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
 * @brief  Return service type from ID
 * @param id service look at
 * @return pointer service or Error
 ******************************************************************************/
luos_type_t RoutingTB_TypeFromID(uint16_t id)
{
    for (uint16_t i = 0; i <= last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == SERVICE)
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
 * @brief  Return service type from alias
 * @param alias service look at
 * @return pointer to service or Error
 ******************************************************************************/
luos_type_t RoutingTB_TypeFromAlias(char *alias)
{
    uint16_t id = RoutingTB_IDFromAlias(alias);
    return RoutingTB_TypeFromID(id);
}
/******************************************************************************
 * @brief  Create a string from a service type
 * @param type of service look at
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
 * @brief  check if the service is a sensor or not
 * @param service look at
 * @return Error
 ******************************************************************************/
uint8_t RoutingTB_ServiceIsSensor(luos_type_t type)
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
 * @brief  return bigest service ID in list
 * @param None
 * @return ID
 ******************************************************************************/
static uint16_t RoutingTB_BigestID(void)
{
    uint16_t max_id = 0;
    for (uint16_t i = 0; i < last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == SERVICE)
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
 * @return Error
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
 * @return None
 ******************************************************************************/
uint16_t RoutingTB_GetNodeID(uint16_t index)
{
    return routing_table[index + 1].id;
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
        if (routing_table[i].mode == SERVICE)
        {
            last_service = routing_table[i].id;
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
 * @brief manage service name increment to never have same alias
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
 * @param service receive
 * @param intro msg in route table
 * @return None
 ******************************************************************************/
static bool RoutingTB_WaitRoutingTable(service_t *service, msg_t *intro_msg)
{
    const uint8_t timeout    = 15; // timeout in ms
    const uint16_t entry_bkp = last_routing_table_entry;
    Luos_SendMsg(service, intro_msg);
    uint32_t timestamp = LuosHAL_GetSystick();
    while ((LuosHAL_GetSystick() - timestamp) < timeout)
    {
        // If this request is for a service in this board allow him to respond.
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
 * @param service in node
 * @param node number on network
 * @return None
 ******************************************************************************/
static void RoutingTB_Generate(service_t *service, uint16_t nb_node)
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
        // set the first service id it can use
        intro_msg.header.size = 2;
        last_cont_id          = RoutingTB_BigestID() + 1;
        memcpy(intro_msg.data, &last_cont_id, sizeof(uint16_t));
        // Ask to introduce and wait for a reply
        if (!RoutingTB_WaitRoutingTable(service, &intro_msg))
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
            // The found_id don't match with the actual ID of the service because the alias already exist
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
 * @param service who send
 * @param node number on network
 * @return None
 ******************************************************************************/
static void RoutingTB_Share(service_t *service, uint16_t nb_node)
{
    // send route table to each nodes. Routing tables are commonly usable for each services of a node.
    msg_t intro_msg;
    intro_msg.header.cmd         = RTB_CMD;
    intro_msg.header.target_mode = NODEIDACK;

    for (uint16_t i = 2; i <= nb_node; i++) //don't send to ourself
    {
        intro_msg.header.target = i;
        Luos_SendData(service, &intro_msg, routing_table, (last_routing_table_entry * sizeof(routing_table_t)));
    }
}

/******************************************************************************
 * @brief Detect all services and create a route table with it.
 * If multiple services have the same name it will be changed with a number in it
 * Automatically at the end this function create a list of sensors id
 * @param service who send
 * @return None
 ******************************************************************************/
void RoutingTB_DetectServices(service_t *service)
{
    // Starts the topology detection.
    uint16_t nb_node = Robus_TopologyDetection(service->ll_service);
    // clear the routing table.
    RoutingTB_Erase();
    // Generate the routing_table
    RoutingTB_Generate(service, nb_node);
    // We have a complete routing table now share it with others.
    RoutingTB_Share(service, nb_node);
}
/******************************************************************************
 * @brief entry in routable node with associate service
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
 * @brief entry in routable service associate to a node
 * @param route table
 * @param service in node
 * @return None
 ******************************************************************************/
void RoutingTB_ConvertServiceToRoutingTable(routing_table_t *entry, service_t *service)
{
    entry->type = service->ll_service->type;
    entry->id   = service->ll_service->id;
    entry->mode = SERVICE;
    for (uint8_t i = 0; i < MAX_ALIAS_SIZE; i++)
    {
        entry->alias[i] = service->alias[i];
    }
}
/******************************************************************************
 * @brief remove an entire node
 * @param route table
 * @return None
 ******************************************************************************/
void RoutingTB_RemoveNode(uint16_t nodeid)
{
    // instead of removing a node just remove all the service in it to make it unusable
    // We could add a param (CONTROL for example) to declare the node as STOP
    // find the node
    for (uint16_t i = 0; i < last_routing_table_entry; i++)
    {
        if (routing_table[i].mode == NODE)
        {
            if (routing_table[i].node_id == nodeid)
            {
                i++;
                // We find our node remove all services
                while (routing_table[i].mode == SERVICE)
                {
                    RoutingTB_RemoveOnRoutingTable(i);
                }
                return;
            }
        }
    }
}
/******************************************************************************
 * @brief remove an entry from routing_table
 * @param index of service
 * @return None
 ******************************************************************************/
void RoutingTB_RemoveOnRoutingTable(uint16_t index)
{
    LUOS_ASSERT(index < last_routing_table_entry);
    memcpy(&routing_table[index], &routing_table[index + 1], sizeof(routing_table_t) * (last_routing_table_entry - (index + 1)));
    last_routing_table_entry--;
    memset(&routing_table[last_routing_table_entry], 0, sizeof(routing_table_t));
}
/******************************************************************************
 * @brief eras erouting_table
 * @param None
 * @return None
 ******************************************************************************/
void RoutingTB_Erase(void)
{
    memset(routing_table, 0, sizeof(routing_table));
    last_service             = 0;
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
 * @return last service ID
 ******************************************************************************/
uint16_t RoutingTB_GetLastService(void)
{
    return (uint16_t)last_service;
}
/******************************************************************************
 * @brief return the last ID registered into the routing_table
 * @param index of service
 * @return Last entry
 ******************************************************************************/
uint16_t RoutingTB_GetLastEntry(void)
{
    return (uint16_t)last_routing_table_entry;
}

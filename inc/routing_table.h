/******************************************************************************
 * @file routingTable
 * @brief routing table descrption function
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef TABLE
#define TABLE

#include "luos.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MAX_RTB_ENTRY 40

typedef enum
{
    CLEAR,     // No content
    CONTAINER, // Contain a container informations
    NODE,      // Contain a node informations
} entry_mode_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/

/* This structure is used to receive or send messages between containers in slave
 * and master mode.
 * please refer to the documentation
 */
typedef struct __attribute__((__packed__))
{
    entry_mode_t mode;
    union
    {
        struct __attribute__((__packed__))
        {                               // CONTAINER mode entry
            uint16_t id;                // Container ID.
            uint16_t type;              // Container type.
            char alias[MAX_ALIAS_SIZE]; // Container alias.
        };
        struct __attribute__((__packed__))
        { // NODE mode entry
            // Watch out this structure have a lot similarities with the node_t struct.
            // It is similar to allow copy of a node_t struct directly in this one.
            // But you there is potentially a port_table size difference so
            // Do not replace it with node_t struct.
            struct __attribute__((__packed__))
            {
                uint16_t node_id : 12;  // Node id
                uint16_t certified : 4; // True if the node have a certificate
            };
            uint16_t port_table[(MAX_ALIAS_SIZE + 2 + 2 - 2) / 2]; // Node link table
        };
        uint8_t unmap_data[MAX_ALIAS_SIZE + 2 + 2];
    };
} routing_table_t;

/*******************************************************************************
 * Function
 ******************************************************************************/
// ********************* routing_table search tools ************************
uint16_t RoutingTB_IDFromAlias(char *alias);
uint16_t RoutingTB_IDFromType(luos_type_t type);
uint16_t RoutingTB_IDFromContainer(container_t *container);
char *RoutingTB_AliasFromId(uint16_t id);
luos_type_t RoutingTB_TypeFromID(uint16_t id);
luos_type_t RoutingTB_TypeFromAlias(char *alias);
char *RoutingTB_StringFromType(luos_type_t type);
uint16_t RoutingTB_NodeIDFromID(uint16_t id);
uint8_t RoutingTB_ContainerIsSensor(luos_type_t type);
uint16_t RoutingTB_GetNodeNB(void);
uint16_t RoutingTB_GetNodeID(uint16_t index);
uint16_t RoutingTB_GetContainerNB(void);
uint16_t RoutingTB_GetContainerID(uint16_t index);
entry_mode_t RoutingTB_GetMode(uint16_t index);

// ********************* routing_table management tools ************************
void RoutingTB_ComputeRoutingTableEntryNB(void);
void RoutingTB_DetectContainers(container_t *container);
void RoutingTB_ConvertNodeToRoutingTable(routing_table_t *entry, node_t *node);
void RoutingTB_ConvertContainerToRoutingTable(routing_table_t *entry, container_t *container);
void RoutingTB_RemoveNode(uint16_t nodeid);
void RoutingTB_RemoveOnRoutingTable(uint16_t id);
void RoutingTB_Erase(void);
routing_table_t *RoutingTB_Get(void);
uint16_t RoutingTB_GetLastContainer(void);
uint16_t *RoutingTB_GetLastNode(void);
uint16_t RoutingTB_GetLastEntry(void);

#endif /* TABLE */

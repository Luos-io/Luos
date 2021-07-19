/******************************************************************************
 * @file luos
 * @brief User functionalities of the Luos library
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "luos.h"
#include <stdio.h>
#include <stdbool.h>
#include "msg_alloc.h"
#include "robus.h"
#include "luos_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef enum
{
    NODE_INIT,
    NODE_RUN
} node_state_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
revision_t luos_version = {.Major = 1, .Minor = 3, .Build = 0};
package_t package_table[MAX_CONTAINER_NUMBER];
uint16_t package_number = 0;
container_t container_table[MAX_CONTAINER_NUMBER];
uint16_t container_number = 0;
volatile routing_table_t *routing_table_pt;
node_state_t node_state = NODE_INIT;

luos_stats_t luos_stats;
general_stats_t general_stats;
/*******************************************************************************
 * Function
 ******************************************************************************/
static error_return_t Luos_MsgHandler(container_t *container, msg_t *input);
static container_t *Luos_GetContainer(ll_container_t *ll_container);
static uint16_t Luos_GetContainerIndex(container_t *container);
static void Luos_TransmitLocalRoutingTable(container_t *container, msg_t *routeTB_msg);
static void Luos_AutoUpdateManager(void);
static error_return_t Luos_SaveAlias(container_t *container, uint8_t *alias);
static void Luos_WriteAlias(uint16_t local_id, uint8_t *alias);
static error_return_t Luos_ReadAlias(uint16_t local_id, uint8_t *alias);
static error_return_t Luos_IsALuosCmd(container_t *container, uint8_t cmd, uint16_t size);
static inline void Luos_EmptyNode(void);
static inline void Luos_PackageInit(void);
static inline void Luos_PackageLoop(void);
static inline void Luos_SetState(node_state_t state);

/******************************************************************************
 * @brief Luos init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Luos_Init(void)
{
    container_number = 0;
    memset(&luos_stats.unmap[0], 0, sizeof(luos_stats_t));
    Robus_Init(&luos_stats.memory);
}
/******************************************************************************
 * @brief Luos Loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Luos_Loop(void)
{
    static uint32_t last_loop_date;
    uint16_t remaining_msg_number       = 0;
    ll_container_t *oldest_ll_container = NULL;
    msg_t *returned_msg                 = NULL;

    // check loop call time stat
    if ((LuosHAL_GetSystick() - last_loop_date) > luos_stats.max_loop_time_ms)
    {
        luos_stats.max_loop_time_ms = LuosHAL_GetSystick() - last_loop_date;
    }
    Robus_Loop();
    // look at all received messages
    while (MsgAlloc_LookAtLuosTask(remaining_msg_number, &oldest_ll_container) != FAILED)
    {
        // There is a message available find the container linked to it
        container_t *container = Luos_GetContainer(oldest_ll_container);
        // check if this is a Luos Command
        uint8_t cmd   = 0;
        uint16_t size = 0;
        // There is a possibility to receive in IT a reset_detection so check task before doing any treatement
        if ((MsgAlloc_GetLuosTaskCmd(remaining_msg_number, &cmd) != SUCCEED) || (MsgAlloc_GetLuosTaskSize(remaining_msg_number, &size) != SUCCEED))
        {
            break;
        }
        //check if this msg cmd should be consumed by Luos_MsgHandler
        if (Luos_IsALuosCmd(container, cmd, size) == SUCCEED)
        {
            if (MsgAlloc_PullMsgFromLuosTask(remaining_msg_number, &returned_msg) == SUCCEED)
            {
                // be sure the content of this message need to be managed by Luos and do it if it is.
                if (Luos_MsgHandler((container_t *)container, returned_msg) == SUCCEED)
                {
                    // Luos CMD are generic for all containers and have to be executed only once
                    // Clear all luos tasks related to this message (in case of multicast message)
                    MsgAlloc_ClearMsgFromLuosTasks(returned_msg);
                }
                else
                {
                    // Here we should not have polling modules.
                    LUOS_ASSERT(container->cont_cb != 0);
                    // This message is for the user, pass it to the user.
                    container->cont_cb(container, returned_msg);
                }
            }
        }
        else
        {
            // This message is for a container
            // check if this continer have a callback?
            if (container->cont_cb != 0)
            {
                // This container have a callback pull the message
                if (MsgAlloc_PullMsgFromLuosTask(remaining_msg_number, &returned_msg) == SUCCEED)
                {
                    // This message is for the user, pass it to the user.
                    container->cont_cb(container, returned_msg);
                }
            }
            else
            {
                remaining_msg_number++;
            }
        }
    }
    // finish msg used
    MsgAlloc_UsedMsgEnd();
    // manage timed auto update
    Luos_AutoUpdateManager();
    // save loop date
    last_loop_date = LuosHAL_GetSystick();
}
/******************************************************************************
 * @brief Check if this command concern luos
 * @param cmd The command value
 * @return SUCCEED if the command if for Luos else Fail
 ******************************************************************************/
static error_return_t Luos_IsALuosCmd(container_t *container, uint8_t cmd, uint16_t size)
{
    switch (cmd)
    {
        case WRITE_NODE_ID:
        case RESET_DETECTION:
        case SET_BAUDRATE:
            // ERROR
            LUOS_ASSERT(0);
            break;
        case ASSERT:
            if (container->cont_cb != 0)
            {
                return SUCCEED;
            }
            else
            {
                return FAILED;
            }
            break;
        case RTB_CMD:
        case WRITE_ALIAS:
        case UPDATE_PUB:
            return SUCCEED;
            break;

        case REVISION:
        case LUOS_REVISION:
        case NODE_UUID:
        case LUOS_STATISTICS:
            if (size == 0)
            {
                return SUCCEED;
            }
            break;
        default:
            return FAILED;
            break;
    }
    return FAILED;
}
/******************************************************************************
 * @brief handling msg for Luos library
 * @param container
 * @param input msg
 * @param output msg
 * @return None
 ******************************************************************************/
static error_return_t Luos_MsgHandler(container_t *container, msg_t *input)
{
    error_return_t consume = FAILED;
    msg_t output_msg;
    routing_table_t *route_tab = &RoutingTB_Get()[RoutingTB_GetLastEntry()];
    time_luos_t time;
    uint16_t base_id = 0;

    switch (input->header.cmd)
    {
        case ASSERT:
            // a module assert remove all modules of the asserted node in routing table
            RoutingTB_RemoveNode(input->header.source);
            // This assert information could be usefull for containers, do not remove it.
            consume = FAILED;
            break;
        case RTB_CMD:
            // Depending on the size of this message we have to make different operations
            // If size is 0 someone ask to get local_route table back
            // If size is 2 someone ask us to generate a local route table based on the given container ID then send local route table back.
            // If size is bigger than 2 this is a complete routing table comming. We have to save it.
            switch (input->header.size)
            {
                case 2:
                    // generate local ID
                    RoutingTB_Erase();
                    memcpy(&base_id, &input->data[0], sizeof(uint16_t));
                    if (base_id == 1)
                    {
                        // set container Id based on received data except for the detector one.
                        base_id   = 2;
                        int index = 0;
                        for (uint16_t i = 0; i < container_number; i++)
                        {
                            if (container_table[i].ll_container->id != 1)
                            {
                                container_table[i].ll_container->id = base_id + index;
                                index++;
                            }
                        }
                    }
                    else
                    {
                        // set container Id based on received data
                        for (uint16_t i = 0; i < container_number; i++)
                        {
                            container_table[i].ll_container->id = base_id + i;
                        }
                    }
                case 0:
                    // send back a local routing table
                    output_msg.header.cmd         = RTB_CMD;
                    output_msg.header.target_mode = IDACK;
                    output_msg.header.target      = input->header.source;
                    Luos_TransmitLocalRoutingTable(container, &output_msg);
                    break;
                default:
                    // Check routing table overflow
                    LUOS_ASSERT(((uint32_t)route_tab + input->header.size) <= ((uint32_t)RoutingTB_Get() + (sizeof(routing_table_t) * MAX_RTB_ENTRY)));
                    if (Luos_ReceiveData(container, input, (void *)route_tab) == SUCCEED)
                    {
                        // route table section reception complete
                        RoutingTB_ComputeRoutingTableEntryNB();
                    }
                    break;
            }
            consume = SUCCEED;
            break;
        case REVISION:
            if (input->header.size == 0)
            {
                msg_t output;
                output.header.cmd         = REVISION;
                output.header.target_mode = ID;
                memcpy(output.data, container->revision.unmap, sizeof(revision_t));
                output.header.size   = sizeof(revision_t);
                output.header.target = input->header.source;
                Luos_SendMsg(container, &output);
                consume = SUCCEED;
            }
            break;
        case LUOS_REVISION:
            if (input->header.size == 0)
            {
                msg_t output;
                output.header.cmd         = LUOS_REVISION;
                output.header.target_mode = ID;
                memcpy(output.data, &luos_version.unmap, sizeof(revision_t));
                output.header.size   = sizeof(revision_t);
                output.header.target = input->header.source;
                Luos_SendMsg(container, &output);
                consume = SUCCEED;
            }
            break;
        case NODE_UUID:
            if (input->header.size == 0)
            {
                msg_t output;
                output.header.cmd         = NODE_UUID;
                output.header.target_mode = ID;
                output.header.size        = sizeof(luos_uuid_t);
                output.header.target      = input->header.source;
                luos_uuid_t uuid;
                uuid.uuid[0] = LUOS_UUID[0];
                uuid.uuid[1] = LUOS_UUID[1];
                uuid.uuid[2] = LUOS_UUID[2];
                memcpy(output.data, &uuid.unmap, sizeof(luos_uuid_t));
                Luos_SendMsg(container, &output);
                consume = SUCCEED;
            }
            break;
        case LUOS_STATISTICS:
            if (input->header.size == 0)
            {
                msg_t output;
                output.header.cmd         = LUOS_STATISTICS;
                output.header.target_mode = ID;
                output.header.size        = sizeof(general_stats_t);
                output.header.target      = input->header.source;
                memcpy(&general_stats.node_stat, &luos_stats.unmap, sizeof(luos_stats_t));
                memcpy(&general_stats.container_stat, container->statistics.unmap, sizeof(container_stats_t));
                memcpy(output.data, &general_stats.unmap, sizeof(general_stats_t));
                Luos_SendMsg(container, &output);
                consume = SUCCEED;
            }
            break;
        case WRITE_ALIAS:
            // Make a clean copy with full \0 at the end.
            memset(container->alias, '\0', MAX_ALIAS_SIZE);
            if (input->header.size > 16)
            {
                input->header.size = 16;
            }
            // check if there is no forbiden character
            uint8_t wrong = false;
            for (uint8_t i = 0; i < MAX_ALIAS_SIZE; i++)
            {
                if (input->data[i] == '\r')
                {
                    wrong = true;
                    break;
                }
            }
            if ((((input->data[0] >= 'A') & (input->data[0] <= 'Z')) | ((input->data[0] >= 'a') & (input->data[0] <= 'z'))) & (input->header.size != 0) & (wrong == false))
            {
                memcpy(container->alias, input->data, input->header.size);
                Luos_SaveAlias(container, container->alias);
            }
            else
            {
                // This is a wrong alias or an erase instruction, get back to default one
                Luos_SaveAlias(container, '\0');
                memcpy(container->alias, container->default_alias, MAX_ALIAS_SIZE);
            }
            consume = SUCCEED;
            break;
        case UPDATE_PUB:
            // this container need to be auto updated
            TimeOD_TimeFromMsg(&time, input);
            container->auto_refresh.target      = input->header.source;
            container->auto_refresh.time_ms     = (uint16_t)TimeOD_TimeTo_ms(time);
            container->auto_refresh.last_update = LuosHAL_GetSystick();
            consume                             = SUCCEED;
            break;
        default:
            break;
    }
    return consume;
}
/******************************************************************************
 * @brief get pointer to a container in route table
 * @param ll_container
 * @return container from list
 ******************************************************************************/
static container_t *Luos_GetContainer(ll_container_t *ll_container)
{
    for (uint16_t i = 0; i < container_number; i++)
    {
        if (ll_container == container_table[i].ll_container)
        {
            return &container_table[i];
        }
    }
    return 0;
}
/******************************************************************************
 * @brief get this index of the container
 * @param container
 * @return container from list
 ******************************************************************************/
static uint16_t Luos_GetContainerIndex(container_t *container)
{
    for (uint16_t i = 0; i < container_number; i++)
    {
        if (container == &container_table[i])
        {
            return i;
        }
    }
    return 0xFFFF;
}
/******************************************************************************
 * @brief transmit local to network
 * @param none
 * @return none
 ******************************************************************************/
static void Luos_TransmitLocalRoutingTable(container_t *container, msg_t *routeTB_msg)
{
    uint16_t entry_nb = 0;
    routing_table_t local_routing_table[container_number + 1];

    //start by saving node entry
    RoutingTB_ConvertNodeToRoutingTable(&local_routing_table[entry_nb], Robus_GetNode());
    entry_nb++;
    // save containers entry
    for (uint16_t i = 0; i < container_number; i++)
    {
        RoutingTB_ConvertContainerToRoutingTable((routing_table_t *)&local_routing_table[entry_nb++], &container_table[i]);
    }
    Luos_SendData(container, routeTB_msg, (void *)local_routing_table, (entry_nb * sizeof(routing_table_t)));
}
/******************************************************************************
 * @brief auto update publication for container
 * @param none
 * @return none
 ******************************************************************************/
static void Luos_AutoUpdateManager(void)
{
    // check all containers timed_update_t contexts
    for (uint16_t i = 0; i < container_number; i++)
    {
        // check if containers have an actual ID. If not, we are in detection mode and should reset the auto refresh
        if (container_table[i].ll_container->id == DEFAULTID)
        {
            // this container have not been detected or is in detection mode. remove auto_refresh parameters
            container_table[i].auto_refresh.target      = 0;
            container_table[i].auto_refresh.time_ms     = 0;
            container_table[i].auto_refresh.last_update = 0;
        }
        else
        {
            // check if there is a timed update setted and if it's time to update it.
            if (container_table[i].auto_refresh.time_ms)
            {
                if ((LuosHAL_GetSystick() - container_table[i].auto_refresh.last_update) >= container_table[i].auto_refresh.time_ms)
                {
                    // This container need to send an update
                    // Create a fake message for it from the container asking for update
                    msg_t updt_msg;
                    updt_msg.header.target      = container_table[i].ll_container->id;
                    updt_msg.header.source      = container_table[i].auto_refresh.target;
                    updt_msg.header.target_mode = IDACK;
                    updt_msg.header.cmd         = ASK_PUB_CMD;
                    updt_msg.header.size        = 0;
                    if ((container_table[i].cont_cb != 0))
                    {
                        container_table[i].cont_cb(&container_table[i], &updt_msg);
                    }
                    else
                    {
                        //store container and msg pointer
                        // todo this can't work for now because this message is not permanent.
                        //mngr_set(&container_table[i], &updt_msg);
                    }
                    container_table[i].auto_refresh.last_update = LuosHAL_GetSystick();
                }
            }
        }
    }
}
/******************************************************************************
 * @brief clear list of container
 * @param none
 * @return none
 ******************************************************************************/
void Luos_ContainersClear(void)
{
    container_number = 0;
    Robus_ContainersClear();
}
/******************************************************************************
 * @brief API to Create a container
 * @param callback msg handler for the container
 * @param type of container corresponding to object dictionnary
 * @param alias for the container string (15 caracters max).
 * @param version FW for the container (tab[MajorVersion,MinorVersion,Patch])
 * @return container object pointer.
 ******************************************************************************/
container_t *Luos_CreateContainer(CONT_CB cont_cb, uint8_t type, const char *alias, revision_t revision)
{
    uint8_t i               = 0;
    container_t *container  = &container_table[container_number];
    container->ll_container = Robus_ContainerCreate(type);

    // Link the container to his callback
    container->cont_cb = cont_cb;
    // Save default alias
    for (i = 0; i < MAX_ALIAS_SIZE - 1; i++)
    {
        container->default_alias[i] = alias[i];
        if (container->default_alias[i] == '\0')
            break;
    }
    container->default_alias[i] = '\0';
    // Initialise the container alias to 0
    memset((void *)container->alias, 0, sizeof(container->alias));
    if (Luos_ReadAlias(container_number, (uint8_t *)container->alias) == FAILED)
    {
        // if no alias saved keep the default one
        for (i = 0; i < MAX_ALIAS_SIZE - 1; i++)
        {
            container->alias[i] = alias[i];
            if (container->alias[i] == '\0')
                break;
        }
        container->alias[i] = '\0';
    }

    //Initialise the container revision to 0
    memset((void *)container->revision.unmap, 0, sizeof(revision_t));
    // Save firmware version
    for (i = 0; i < sizeof(revision_t); i++)
    {
        container->revision.unmap[i] = revision.unmap[i];
    }

    //initiate container statistics
    container->node_statistics                 = &luos_stats;
    container->ll_container->ll_stat.max_retry = &container->statistics.max_retry;

    container_number++;
    return container;
}
/******************************************************************************
 * @brief Send msg through network
 * @param Container who send
 * @param Message to send
 * @return None
 ******************************************************************************/
error_return_t Luos_SendMsg(container_t *container, msg_t *msg)
{
    if (container == 0)
    {
        // There is no container specified here, take the first one
        container = &container_table[0];
    }
    if (Robus_SendMsg(container->ll_container, msg) == FAILED)
    {
        return FAILED;
    }

    return SUCCEED;
}
/******************************************************************************
 * @brief read last msg from buffer for a container
 * @param container who receive the message we are looking for
 * @param returned_msg oldest message of the container
 * @return FAILED if no message available
 ******************************************************************************/
error_return_t Luos_ReadMsg(container_t *container, msg_t **returned_msg)
{
    error_return_t error = SUCCEED;
    while (error == SUCCEED)
    {
        error = MsgAlloc_PullMsg(container->ll_container, returned_msg);
        // check if the content of this message need to be managed by Luos and do it if it is.
        if (error == SUCCEED)
        {
            if (Luos_MsgHandler(container, *returned_msg) == FAILED)
            {
                // This message is for the user, pass it to the user.
                return SUCCEED;
            }
        }
        MsgAlloc_ClearMsgFromLuosTasks(*returned_msg);
    }
    return FAILED;
}
/******************************************************************************
 * @brief read last msg from buffer from a specific id container
 * @param container who receive the message we are looking for
 * @param id who sent the message we are looking for
 * @param returned_msg oldest message of the container
 * @return FAILED if no message available
 ******************************************************************************/
error_return_t Luos_ReadFromContainer(container_t *container, short id, msg_t **returned_msg)
{
    uint16_t remaining_msg_number       = 0;
    ll_container_t *oldest_ll_container = NULL;
    error_return_t error                = SUCCEED;
    while (MsgAlloc_LookAtLuosTask(remaining_msg_number, &oldest_ll_container) != FAILED)
    {
        // Check if this message is for us
        if (oldest_ll_container == container->ll_container)
        {
            // Check the source id
            uint16_t source = 0;
#ifdef LUOS_ASSERTION
            LUOS_ASSERT(MsgAlloc_GetLuosTaskSourceId(remaining_msg_number, &source) == SUCCEED);
#else
            MsgAlloc_GetLuosTaskSourceId(remaining_msg_number, &source);
#endif
            if (source == id)
            {
                // Source id of this message match, get it and treat it.
                error = MsgAlloc_PullMsgFromLuosTask(remaining_msg_number, returned_msg);
                // check if the content of this message need to be managed by Luos and do it if it is.
                if ((Luos_MsgHandler(container, *returned_msg) == FAILED) & (error == SUCCEED))
                {
                    // This message is for the user, pass it to the user.
                    return SUCCEED;
                }
                MsgAlloc_ClearMsgFromLuosTasks(*returned_msg);
            }
            else
            {
                remaining_msg_number++;
            }
        }
        else
        {
            remaining_msg_number++;
        }
    }
    return FAILED;
}
/******************************************************************************
 * @brief Send large among of data and formating to send into multiple msg
 * @param Container who send
 * @param Message to send
 * @param Pointer to the message data table
 * @param Size of the data to transmit
 * @return None
 ******************************************************************************/
void Luos_SendData(container_t *container, msg_t *msg, void *bin_data, uint16_t size)
{
    // Compute number of message needed to send this data
    uint16_t msg_number = 1;
    uint16_t sent_size  = 0;
    if (size > MAX_DATA_MSG_SIZE)
    {
        msg_number = (size / MAX_DATA_MSG_SIZE);
        msg_number += (msg_number * MAX_DATA_MSG_SIZE < size);
    }

    // Send messages one by one
    for (uint16_t chunk = 0; chunk < msg_number; chunk++)
    {
        // Compute chunk size
        uint16_t chunk_size = 0;
        if ((size - sent_size) > MAX_DATA_MSG_SIZE)
        {
            chunk_size = MAX_DATA_MSG_SIZE;
        }
        else
        {
            chunk_size = size - sent_size;
        }

        // Copy data into message
        memcpy(msg->data, (uint8_t *)bin_data + sent_size, chunk_size);
        msg->header.size = size - sent_size;

        // Send message
        uint32_t tickstart = Luos_GetSystick();
        while (Luos_SendMsg(container, msg) == FAILED)
        {
            // No more memory space available
            Luos_Loop();
            // 500 here represent 500ms of timeout after start trying to load our data in memory.
            LUOS_ASSERT(((volatile uint32_t)Luos_GetSystick() - tickstart) < 500);
        }

        // Save current state
        sent_size = sent_size + chunk_size;
    }
}
/******************************************************************************
 * @brief receive a multi msg data
 * @param Container who receive
 * @param Message chunk received
 * @param pointer to data
 * @return error
 ******************************************************************************/
error_return_t Luos_ReceiveData(container_t *container, msg_t *msg, void *bin_data)
{
    // Manage buffer session (one per container)
    static uint32_t data_size[MAX_CONTAINER_NUMBER]       = {0};
    static uint32_t total_data_size[MAX_CONTAINER_NUMBER] = {0};
    static uint16_t last_msg_size                         = 0;
    uint16_t id                                           = Luos_GetContainerIndex(container);
    // check good container index
    if (id == 0xFFFF)
    {
        return FAILED;
    }

    //store total size of a msg
    if (total_data_size[id] == 0)
    {
        total_data_size[id] = msg->header.size;
    }

    LUOS_ASSERT(msg->header.size <= total_data_size[id]);

    // check message integrity
    if ((last_msg_size > 0) && (last_msg_size - MAX_DATA_MSG_SIZE > msg->header.size))
    {
        // we miss a message (a part of the data),
        // reset session and return an error.
        data_size[id] = 0;
        last_msg_size = 0;
        return FAILED;
    }

    // Get chunk size
    uint16_t chunk_size = 0;
    if (msg->header.size > MAX_DATA_MSG_SIZE)
    {
        chunk_size = MAX_DATA_MSG_SIZE;
    }
    else
    {
        chunk_size = msg->header.size;
    }

    // Copy data into buffer
    memcpy(bin_data + data_size[id], msg->data, chunk_size);

    // Save buffer session
    data_size[id] = data_size[id] + chunk_size;
    last_msg_size = msg->header.size;

    //check
    LUOS_ASSERT(data_size[id] <= total_data_size[id]);

    // Check end of data
    if (msg->header.size <= MAX_DATA_MSG_SIZE)
    {
        // Data collection finished, reset buffer session state
        data_size[id]       = 0;
        last_msg_size       = 0;
        total_data_size[id] = 0;
        return SUCCEED;
    }
    return FAILED;
}
/******************************************************************************
 * @brief Send datas of a streaming channel
 * @param Container who send
 * @param Message to send
 * @param streaming channel pointer
 * @return None
 ******************************************************************************/
void Luos_SendStreaming(container_t *container, msg_t *msg, streaming_channel_t *stream)
{
    // Compute number of message needed to send available datas on ring buffer
    int msg_number              = 1;
    int data_size               = Stream_GetAvailableSampleNB(stream);
    const int max_data_msg_size = (MAX_DATA_MSG_SIZE / stream->data_size);
    if (data_size > max_data_msg_size)
    {
        msg_number = (data_size / max_data_msg_size);
        msg_number += ((msg_number * max_data_msg_size) < data_size);
    }

    // Send messages one by one
    for (volatile uint16_t chunk = 0; chunk < msg_number; chunk++)
    {
        // compute chunk size
        uint16_t chunk_size = 0;
        if (data_size > max_data_msg_size)
        {
            chunk_size = max_data_msg_size;
        }
        else
        {
            chunk_size = data_size;
        }

        // Copy data into message
        Stream_GetSample(stream, msg->data, chunk_size);
        msg->header.size = data_size;

        // Send message
        while (Luos_SendMsg(container, msg) == FAILED)
        {
            Luos_Loop();
        }

        // check end of data
        if (data_size > max_data_msg_size)
        {
            data_size -= max_data_msg_size;
        }
        else
        {
            data_size = 0;
        }
    }
}
/******************************************************************************
 * @brief Receive a streaming channel datas
 * @param Container who send
 * @param Message to send
 * @param streaming channel pointer
 * @return error
 ******************************************************************************/
error_return_t Luos_ReceiveStreaming(container_t *container, msg_t *msg, streaming_channel_t *stream)
{
    // Get chunk size
    unsigned short chunk_size = 0;
    if (msg->header.size > MAX_DATA_MSG_SIZE)
        chunk_size = MAX_DATA_MSG_SIZE;
    else
        chunk_size = msg->header.size;

    // Copy data into buffer
    Stream_PutSample(stream, msg->data, (chunk_size / stream->data_size));

    // Check end of data
    if ((msg->header.size <= MAX_DATA_MSG_SIZE))
    {
        // Chunk collection finished
        return SUCCEED;
    }
    return FAILED;
}
/******************************************************************************
 * @brief store alias name container in flash
 * @param container to store
 * @param alias to store
 * @return error
 ******************************************************************************/
static error_return_t Luos_SaveAlias(container_t *container, uint8_t *alias)
{
    // Get container index
    uint16_t i = (uint16_t)(Luos_GetContainerIndex(container));

    if ((i >= 0) && (i != 0xFFFF))
    {
        Luos_WriteAlias(i, alias);
        return SUCCEED;
    }
    return FAILED;
}
/******************************************************************************
 * @brief write alias name container from flash
 * @param position in the route table
 * @param alias to store
 * @return error
 ******************************************************************************/
static void Luos_WriteAlias(uint16_t local_id, uint8_t *alias)
{
    uint32_t addr = ADDRESS_ALIASES_FLASH + (local_id * (MAX_ALIAS_SIZE + 1));
    LuosHAL_FlashWriteLuosMemoryInfo(addr, 16, (uint8_t *)alias);
}
/******************************************************************************
 * @brief read alias from flash
 * @param position in the route table
 * @param alias to store
 * @return error
 ******************************************************************************/
static error_return_t Luos_ReadAlias(uint16_t local_id, uint8_t *alias)
{
    uint32_t addr = ADDRESS_ALIASES_FLASH + (local_id * (MAX_ALIAS_SIZE + 1));
    LuosHAL_FlashReadLuosMemoryInfo(addr, 16, (uint8_t *)alias);
    // Check name integrity
    if ((((alias[0] < 'A') | (alias[0] > 'Z')) & ((alias[0] < 'a') | (alias[0] > 'z'))) | (alias[0] == '\0'))
    {
        return FAILED;
    }
    else
    {
        return SUCCEED;
    }
}
/******************************************************************************
 * @brief send network bauderate
 * @param container sending request
 * @param baudrate
 * @return None
 ******************************************************************************/
void Luos_SendBaudrate(container_t *container, uint32_t baudrate)
{
    msg_t msg;
    memcpy(msg.data, &baudrate, sizeof(uint32_t));
    msg.header.target_mode = BROADCAST;
    msg.header.target      = BROADCAST_VAL;
    msg.header.cmd         = SET_BAUDRATE;
    msg.header.size        = sizeof(uint32_t);
    Robus_SendMsg(container->ll_container, &msg);
}
/******************************************************************************
 * @brief set id of a container trough the network
 * @param container sending request
 * @param target_mode
 * @param target
 * @param newid : The new Id of container(s)
 * @return None
 ******************************************************************************/
void Luos_SetExternId(container_t *container, target_mode_t target_mode, uint16_t target, uint16_t newid)
{
    msg_t msg;
    msg.header.target      = target;
    msg.header.target_mode = target_mode;
    msg.header.cmd         = WRITE_NODE_ID;
    msg.header.size        = 2;
    msg.data[1]            = newid;
    msg.data[0]            = (newid << 8);
    Robus_SendMsg(container->ll_container, &msg);
}
/******************************************************************************
 * @brief return the number of messages available
 * @param None
 * @return the number of messages
 ******************************************************************************/
uint16_t Luos_NbrAvailableMsg(void)
{
    return MsgAlloc_LuosTasksNbr();
}
/******************************************************************************
 * @brief Get tick number
 * @param None
 * @return tick
 ******************************************************************************/
uint32_t Luos_GetSystick(void)
{
    return LuosHAL_GetSystick();
}
/******************************************************************************
 * @brief return True if all message are complete
 * @param None
 * @return error
 ******************************************************************************/
error_return_t Luos_TxComplete(void)
{
    return MsgAlloc_TxAllComplete();
}
/******************************************************************************
 * @brief Flush the entire Luos msg buffer
 * @param None
 * @return None
 ******************************************************************************/
void Luos_Flush(void)
{
    Robus_Flush();
}

/******************************************************************************
 * @brief register a new package
 * @param package to register
 * @return None
 ******************************************************************************/
void Luos_AddPackage(void (*Init)(void), void (*Loop)(void))
{
    package_table[package_number].Init = Init;
    package_table[package_number].Loop = Loop;

    package_number += 1;
}

/******************************************************************************
 * @brief Create a service to signal empty node
 * @param None
 * @return None
 ******************************************************************************/
void Luos_EmptyNode(void)
{
    Luos_CreateContainer(0, VOID_MOD, "empty_node", luos_version);
}

/******************************************************************************
 * @brief Run each package Init()
 * @param None
 * @return None
 ******************************************************************************/
void Luos_PackageInit(void)
{
    uint16_t package_index = 0;
    if (package_number)
    {
        while (package_index < package_number)
        {
            package_table[package_index].Init();
            package_index += 1;
        }
    }
    else
    {
        Luos_EmptyNode();
    }
}

/******************************************************************************
 * @brief Run each package Loop()
 * @param None
 * @return None
 ******************************************************************************/
void Luos_PackageLoop(void)
{
    uint16_t package_index = 0;
    while (package_index < package_number)
    {
        package_table[package_index].Loop();
        package_index += 1;
    }
}

/******************************************************************************
 * @brief Luos high level state machine
 * @param None
 * @return None
 ******************************************************************************/
void Luos_SetState(node_state_t state)
{
    node_state = state;
}

/******************************************************************************
 * @brief Luos high level state machine
 * @param None
 * @return None
 ******************************************************************************/
void Luos_Run(void)
{
    switch (node_state)
    {
        case NODE_INIT:
            Luos_Init();
            Luos_PackageInit();
            // go to run state after initialization
            Luos_SetState(NODE_RUN);
            break;
        case NODE_RUN:
            Luos_Loop();
            Luos_PackageLoop();
            break;
        default:
            Luos_Loop();
            Luos_PackageLoop();
            break;
    }
}
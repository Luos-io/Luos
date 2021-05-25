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

/*******************************************************************************
 * Variables
 ******************************************************************************/
revision_t luos_version = {.Major = 1, .Minor = 2, .Build = 0};
service_t service_table[MAX_SERVICE_NUMBER];
uint16_t service_number;
volatile routing_table_t *routing_table_pt;

luos_stats_t luos_stats;
general_stats_t general_stats;
/*******************************************************************************
 * Function
 ******************************************************************************/
static error_return_t Luos_MsgHandler(service_t *service, msg_t *input);
static service_t *Luos_GetService(ll_service_t *ll_service);
static uint16_t Luos_GetServiceIndex(service_t *service);
static void Luos_TransmitLocalRoutingTable(service_t *service, msg_t *routeTB_msg);
static void Luos_AutoUpdateManager(void);
static error_return_t Luos_SaveAlias(service_t *service, uint8_t *alias);
static void Luos_WriteAlias(uint16_t local_id, uint8_t *alias);
static error_return_t Luos_ReadAlias(uint16_t local_id, uint8_t *alias);
static error_return_t Luos_IsALuosCmd(service_t *service, uint8_t cmd, uint16_t size);

/******************************************************************************
 * @brief Luos init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Luos_Init(void)
{
    service_number = 0;
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
    uint16_t remaining_msg_number   = 0;
    ll_service_t *oldest_ll_service = NULL;
    msg_t *returned_msg             = NULL;

    // check loop call time stat
    if ((LuosHAL_GetSystick() - last_loop_date) > luos_stats.max_loop_time_ms)
    {
        luos_stats.max_loop_time_ms = LuosHAL_GetSystick() - last_loop_date;
    }
    Robus_Loop();
    // look at all received messages
    while (MsgAlloc_LookAtLuosTask(remaining_msg_number, &oldest_ll_service) != FAILED)
    {
        // There is a message available find the service linked to it
        service_t *service = Luos_GetService(oldest_ll_service);
        // check if this is a Luos Command
        uint8_t cmd   = 0;
        uint16_t size = 0;
        // There is a possibility to receive in IT a restet_detection so check task before doing any treatement
        if ((MsgAlloc_GetLuosTaskCmd(remaining_msg_number, &cmd) != SUCCEED) || (MsgAlloc_GetLuosTaskSize(remaining_msg_number, &size) != SUCCEED))
        {
            break;
        }
        //check if this msg cmd should be consumed by Luos_MsgHandler
        if (Luos_IsALuosCmd(service, cmd, size) == SUCCEED)
        {
            if (MsgAlloc_PullMsgFromLuosTask(remaining_msg_number, &returned_msg) == SUCCEED)
            {
                // be sure the content of this message need to be managed by Luos and do it if it is.
                if (Luos_MsgHandler((service_t *)service, returned_msg) == SUCCEED)
                {
                    // Luos CMD are generic for all services and have to be executed only once
                    // Clear all luos tasks related to this message (in case of multicast message)
                    MsgAlloc_ClearMsgFromLuosTasks(returned_msg);
                }
                else
                {
                    // Here we should not have polling modules.
                    LUOS_ASSERT(service->cont_cb != 0);
                    // This message is for the user, pass it to the user.
                    service->cont_cb(service, returned_msg);
                }
            }
        }
        else
        {
            // This message is for a service
            // check if this continer have a callback?
            if (service->cont_cb != 0)
            {
                // This service have a callback pull the message
                if (MsgAlloc_PullMsgFromLuosTask(remaining_msg_number, &returned_msg) == SUCCEED)
                {
                    // This message is for the user, pass it to the user.
                    service->cont_cb(service, returned_msg);
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
static error_return_t Luos_IsALuosCmd(service_t *service, uint8_t cmd, uint16_t size)
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
            if (service->cont_cb != 0)
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
 * @param service
 * @param input msg
 * @param output msg
 * @return None
 ******************************************************************************/
static error_return_t Luos_MsgHandler(service_t *service, msg_t *input)
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
            // This assert information could be usefull for services, do not remove it.
            consume = FAILED;
            break;
        case RTB_CMD:
            // Depending on the size of this message we have to make different operations
            // If size is 0 someone ask to get local_route table back
            // If size is 2 someone ask us to generate a local route table based on the given service ID then send local route table back.
            // If size is bigger than 2 this is a complete routing table comming. We have to save it.
            switch (input->header.size)
            {
                case 2:
                    // generate local ID
                    RoutingTB_Erase();
                    memcpy(&base_id, &input->data[0], sizeof(uint16_t));
                    if (base_id == 1)
                    {
                        // set service Id based on received data except for the detector one.
                        base_id   = 2;
                        int index = 0;
                        for (uint16_t i = 0; i < service_number; i++)
                        {
                            if (service_table[i].ll_service->id != 1)
                            {
                                service_table[i].ll_service->id = base_id + index;
                                index++;
                            }
                        }
                    }
                    else
                    {
                        // set service Id based on received data
                        for (uint16_t i = 0; i < service_number; i++)
                        {
                            service_table[i].ll_service->id = base_id + i;
                        }
                    }
                case 0:
                    // send back a local routing table
                    output_msg.header.cmd         = RTB_CMD;
                    output_msg.header.target_mode = IDACK;
                    output_msg.header.target      = input->header.source;
                    Luos_TransmitLocalRoutingTable(service, &output_msg);
                    break;
                default:
                    // Check routing table overflow
                    LUOS_ASSERT(((uint32_t)route_tab + input->header.size) <= ((uint32_t)RoutingTB_Get() + (sizeof(routing_table_t) * MAX_RTB_ENTRY)));
                    if (Luos_ReceiveData(service, input, (void *)route_tab) == SUCCEED)
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
                memcpy(output.data, service->revision.unmap, sizeof(revision_t));
                output.header.size   = sizeof(revision_t);
                output.header.target = input->header.source;
                Luos_SendMsg(service, &output);
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
                Luos_SendMsg(service, &output);
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
                Luos_SendMsg(service, &output);
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
                memcpy(&general_stats.service_stat, service->statistics.unmap, sizeof(service_stats_t));
                memcpy(output.data, &general_stats.unmap, sizeof(general_stats_t));
                Luos_SendMsg(service, &output);
                consume = SUCCEED;
            }
            break;
        case WRITE_ALIAS:
            // Make a clean copy with full \0 at the end.
            memset(service->alias, '\0', MAX_ALIAS_SIZE);
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
                memcpy(service->alias, input->data, input->header.size);
                Luos_SaveAlias(service, service->alias);
            }
            else
            {
                // This is a wrong alias or an erase instruction, get back to default one
                Luos_SaveAlias(service, '\0');
                memcpy(service->alias, service->default_alias, MAX_ALIAS_SIZE);
            }
            consume = SUCCEED;
            break;
        case UPDATE_PUB:
            // this service need to be auto updated
            TimeOD_TimeFromMsg(&time, input);
            service->auto_refresh.target      = input->header.source;
            service->auto_refresh.time_ms     = (uint16_t)TimeOD_TimeTo_ms(time);
            service->auto_refresh.last_update = LuosHAL_GetSystick();
            consume                           = SUCCEED;
            break;
        default:
            break;
    }
    return consume;
}
/******************************************************************************
 * @brief get pointer to a service in route table
 * @param ll_service
 * @return service from list
 ******************************************************************************/
static service_t *Luos_GetService(ll_service_t *ll_service)
{
    for (uint16_t i = 0; i < service_number; i++)
    {
        if (ll_service == service_table[i].ll_service)
        {
            return &service_table[i];
        }
    }
    return 0;
}
/******************************************************************************
 * @brief get this index of the service
 * @param service
 * @return service from list
 ******************************************************************************/
static uint16_t Luos_GetServiceIndex(service_t *service)
{
    for (uint16_t i = 0; i < service_number; i++)
    {
        if (service == &service_table[i])
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
static void Luos_TransmitLocalRoutingTable(service_t *service, msg_t *routeTB_msg)
{
    uint16_t entry_nb = 0;
    routing_table_t local_routing_table[service_number + 1];

    //start by saving node entry
    RoutingTB_ConvertNodeToRoutingTable(&local_routing_table[entry_nb], Robus_GetNode());
    entry_nb++;
    // save services entry
    for (uint16_t i = 0; i < service_number; i++)
    {
        RoutingTB_ConvertServiceToRoutingTable((routing_table_t *)&local_routing_table[entry_nb++], &service_table[i]);
    }
    Luos_SendData(service, routeTB_msg, (void *)local_routing_table, (entry_nb * sizeof(routing_table_t)));
}
/******************************************************************************
 * @brief auto update publication for service
 * @param none
 * @return none
 ******************************************************************************/
static void Luos_AutoUpdateManager(void)
{
    // check all services timed_update_t contexts
    for (uint16_t i = 0; i < service_number; i++)
    {
        // check if services have an actual ID. If not, we are in detection mode and should reset the auto refresh
        if (service_table[i].ll_service->id == DEFAULTID)
        {
            // this service have not been detected or is in detection mode. remove auto_refresh parameters
            service_table[i].auto_refresh.target      = 0;
            service_table[i].auto_refresh.time_ms     = 0;
            service_table[i].auto_refresh.last_update = 0;
        }
        else
        {
            // check if there is a timed update setted and if it's time to update it.
            if (service_table[i].auto_refresh.time_ms)
            {
                if ((LuosHAL_GetSystick() - service_table[i].auto_refresh.last_update) >= service_table[i].auto_refresh.time_ms)
                {
                    // This service need to send an update
                    // Create a fake message for it from the service asking for update
                    msg_t updt_msg;
                    updt_msg.header.target      = service_table[i].ll_service->id;
                    updt_msg.header.source      = service_table[i].auto_refresh.target;
                    updt_msg.header.target_mode = IDACK;
                    updt_msg.header.cmd         = ASK_PUB_CMD;
                    updt_msg.header.size        = 0;
                    if ((service_table[i].cont_cb != 0))
                    {
                        service_table[i].cont_cb(&service_table[i], &updt_msg);
                    }
                    else
                    {
                        //store service and msg pointer
                        // todo this can't work for now because this message is not permanent.
                        //mngr_set(&service_table[i], &updt_msg);
                    }
                    service_table[i].auto_refresh.last_update = LuosHAL_GetSystick();
                }
            }
        }
    }
}
/******************************************************************************
 * @brief clear list of service
 * @param none
 * @return none
 ******************************************************************************/
void Luos_ServicesClear(void)
{
    service_number = 0;
    Robus_ServicesClear();
}
/******************************************************************************
 * @brief API to Create a service
 * @param callback msg handler for the service
 * @param type of service corresponding to object dictionnary
 * @param alias for the service string (15 caracters max).
 * @param version FW for the service (tab[MajorVersion,MinorVersion,Patch])
 * @return service object pointer.
 ******************************************************************************/
service_t *Luos_CreateService(CONT_CB cont_cb, uint8_t type, const char *alias, revision_t revision)
{
    uint8_t i           = 0;
    service_t *service  = &service_table[service_number];
    service->ll_service = Robus_ServiceCreate(type);

    // Link the service to his callback
    service->cont_cb = cont_cb;
    // Save default alias
    for (i = 0; i < MAX_ALIAS_SIZE - 1; i++)
    {
        service->default_alias[i] = alias[i];
        if (service->default_alias[i] == '\0')
            break;
    }
    service->default_alias[i] = '\0';
    // Initialise the service alias to 0
    memset((void *)service->alias, 0, sizeof(service->alias));
    if (Luos_ReadAlias(service_number, (uint8_t *)service->alias) == FAILED)
    {
        // if no alias saved keep the default one
        for (i = 0; i < MAX_ALIAS_SIZE - 1; i++)
        {
            service->alias[i] = alias[i];
            if (service->alias[i] == '\0')
                break;
        }
        service->alias[i] = '\0';
    }

    //Initialise the service revision to 0
    memset((void *)service->revision.unmap, 0, sizeof(revision_t));
    // Save firmware version
    for (i = 0; i < sizeof(revision_t); i++)
    {
        service->revision.unmap[i] = revision.unmap[i];
    }

    //initiate service statistics
    service->node_statistics               = &luos_stats;
    service->ll_service->ll_stat.max_retry = &service->statistics.max_retry;

    service_number++;
    return service;
}
/******************************************************************************
 * @brief Send msg through network
 * @param Service who send
 * @param Message to send
 * @return None
 ******************************************************************************/
error_return_t Luos_SendMsg(service_t *service, msg_t *msg)
{
    if (service == 0)
    {
        // There is no service specified here, take the first one
        service = &service_table[0];
    }
    if (Robus_SendMsg(service->ll_service, msg) == FAILED)
    {
        return FAILED;
    }

    return SUCCEED;
}
/******************************************************************************
 * @brief read last msg from buffer for a service
 * @param service who receive the message we are looking for
 * @param returned_msg oldest message of the service
 * @return FAILED if no message available
 ******************************************************************************/
error_return_t Luos_ReadMsg(service_t *service, msg_t **returned_msg)
{
    error_return_t error = SUCCEED;
    while (error == SUCCEED)
    {
        error = MsgAlloc_PullMsg(service->ll_service, returned_msg);
        // check if the content of this message need to be managed by Luos and do it if it is.
        if ((Luos_MsgHandler(service, *returned_msg) == FAILED) & (error == SUCCEED))
        {
            // This message is for the user, pass it to the user.
            return SUCCEED;
        }
        MsgAlloc_ClearMsgFromLuosTasks(*returned_msg);
    }
    return FAILED;
}
/******************************************************************************
 * @brief read last msg from buffer from a specific id service
 * @param service who receive the message we are looking for
 * @param id who sent the message we are looking for
 * @param returned_msg oldest message of the service
 * @return FAILED if no message available
 ******************************************************************************/
error_return_t Luos_ReadFromService(service_t *service, short id, msg_t **returned_msg)
{
    uint16_t remaining_msg_number   = 0;
    ll_service_t *oldest_ll_service = NULL;
    error_return_t error            = SUCCEED;
    while (MsgAlloc_LookAtLuosTask(remaining_msg_number, &oldest_ll_service) != FAILED)
    {
        // Check if this message is for us
        if (oldest_ll_service == service->ll_service)
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
                if ((Luos_MsgHandler(service, *returned_msg) == FAILED) & (error == SUCCEED))
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
 * @param Service who send
 * @param Message to send
 * @param Pointer to the message data table
 * @param Size of the data to transmit
 * @return None
 ******************************************************************************/
void Luos_SendData(service_t *service, msg_t *msg, void *bin_data, uint16_t size)
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
    for (volatile uint16_t chunk = 0; chunk < msg_number; chunk++)
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
        while (Luos_SendMsg(service, msg) == FAILED)
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
 * @param Service who receive
 * @param Message chunk received
 * @param pointer to data
 * @return error
 ******************************************************************************/
error_return_t Luos_ReceiveData(service_t *service, msg_t *msg, void *bin_data)
{
    // Manage buffer session (one per service)
    static uint32_t data_size[MAX_SERVICE_NUMBER]       = {0};
    static uint32_t total_data_size[MAX_SERVICE_NUMBER] = {0};
    static uint16_t last_msg_size                       = 0;
    uint16_t id                                         = Luos_GetServiceIndex(service);
    // check good service index
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
 * @param Service who send
 * @param Message to send
 * @param streaming channel pointer
 * @return None
 ******************************************************************************/
void Luos_SendStreaming(service_t *service, msg_t *msg, streaming_channel_t *stream)
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

        // Send message
        while (Luos_SendMsg(service, msg) == FAILED)
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
 * @param Service who send
 * @param Message to send
 * @param streaming channel pointer
 * @return error
 ******************************************************************************/
error_return_t Luos_ReceiveStreaming(service_t *service, msg_t *msg, streaming_channel_t *stream)
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
 * @brief store alias name service in flash
 * @param service to store
 * @param alias to store
 * @return error
 ******************************************************************************/
static error_return_t Luos_SaveAlias(service_t *service, uint8_t *alias)
{
    // Get service index
    uint16_t i = (uint16_t)(Luos_GetServiceIndex(service));

    if ((i >= 0) && (i != 0xFFFF))
    {
        Luos_WriteAlias(i, alias);
        return SUCCEED;
    }
    return FAILED;
}
/******************************************************************************
 * @brief write alias name service from flash
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
 * @param service sending request
 * @param baudrate
 * @return None
 ******************************************************************************/
void Luos_SendBaudrate(service_t *service, uint32_t baudrate)
{
    msg_t msg;
    memcpy(msg.data, &baudrate, sizeof(uint32_t));
    msg.header.target_mode = BROADCAST;
    msg.header.target      = BROADCAST_VAL;
    msg.header.cmd         = SET_BAUDRATE;
    msg.header.size        = sizeof(uint32_t);
    Robus_SendMsg(service->ll_service, &msg);
}
/******************************************************************************
 * @brief set id of a service trough the network
 * @param service sending request
 * @param target_mode
 * @param target
 * @param newid : The new Id of service(s)
 * @return None
 ******************************************************************************/
void Luos_SetExternId(service_t *service, target_mode_t target_mode, uint16_t target, uint16_t newid)
{
    msg_t msg;
    msg.header.target      = target;
    msg.header.target_mode = target_mode;
    msg.header.cmd         = WRITE_NODE_ID;
    msg.header.size        = 2;
    msg.data[1]            = newid;
    msg.data[0]            = (newid << 8);
    Robus_SendMsg(service->ll_service, &msg);
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

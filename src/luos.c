#include "luos.h"
#include "l0.h"
#include "robus.h"
#include "sys_msg.h"
#include <string.h>
#include "message_mngr.h"

static module_t *luos_module_pointer;
static volatile msg_t luos_pub_msg;
static volatile int luos_pub = LUOS_PROTOCOL_NB;
module_t module_table[MAX_VM_NUMBER];
unsigned char module_number;

//**************** Private functions*********************
static int luos_msg_handler(module_t* module, msg_t* input, msg_t* output) {
    if (input->header.cmd == IDENTIFY_CMD) {
        output->header.cmd = INTRODUCTION_CMD;
        output->header.target_mode = ID;
        output->header.size = MAX_ALIAS_SIZE+1;
        output->header.target = input->header.source;
        for (int i=0; i<MAX_ALIAS_SIZE; i++) {
            output->data[i] = module->alias[i];
        }
        output->data[MAX_ALIAS_SIZE] = module->vm->type;
        luos_pub = IDENTIFY_CMD;
        return 1;
    }
    if (input->header.cmd == INTRODUCTION_CMD) {
        insert_on_route_table(input->header.source, deser_introduction(input));
        return 1;
    }
    if ((input->header.cmd == REVISION) & (input->header.size == 0)) {
        output->header.cmd = REVISION;
        output->header.target_mode = ID;
#ifndef FIRM_REV
#define FIRM_REV "unknown"
#endif
        memcpy(output->data, FIRM_REV, sizeof("unknown"));
        output->header.size = strlen((char*)output->data);
        output->header.target = input->header.source;
        luos_pub = REVISION;
        return 1;
    }
    if ((input->header.cmd == UUID) & (input->header.size == 0)) {
        output->header.cmd = UUID;
        output->header.target_mode = ID;
        output->header.size = sizeof(luos_uuid_t);
        output->header.target = input->header.source;
        luos_uuid_t uuid;
        uuid.uuid[0] = LUOS_UUID[0];
        uuid.uuid[1] = LUOS_UUID[1];
        uuid.uuid[2] = LUOS_UUID[2];
        memcpy(output->data, &uuid.unmap, sizeof(luos_uuid_t));
        luos_pub = UUID;
        return 1;
    }

    if ((input->header.cmd == WRITE_ALIAS)) {
        // Make a clean copy with full \0 at the end.
        memset(module->alias, '\0', sizeof(module->alias));
        if (input->header.size > 16) input->header.size = 16;
        if ((((input->data[0] > 'A') & (input->data[0] < 'Z')) | ((input->data[0] > 'a') & (input->data[0] < 'z')) | (input->data[0] == '\0')) & (input->header.size != 0)) {
            memcpy(module->alias, input->data, input->header.size);
            luos_save_alias(module, module->alias);
        } else {
            // This is an alias erase instruction, get back to default one
            luos_save_alias(module, '\0');
            memcpy(module->alias, module->default_alias, MAX_ALIAS_SIZE);
        }
        return 1;
    }

    return 0;
}

module_t* get_module(vm_t* vm){
    for (int i=0; i < module_number; i++) {
        if (vm == module_table[i].vm) {
            return &module_table[i];
        }
    }
    return 0;
}

int get_module_index(module_t* module){
    for (int i=0; i < module_number; i++) {
        if (module == &module_table[i]) {
            return i;
        }
    }
    return -1;
}

void luos_cb(vm_t *vm, msg_t *msg) {
    // Luos message management
    volatile module_t* module = get_module(vm);
    if (luos_msg_handler(module, msg, (msg_t*)&luos_pub_msg)) {
        luos_module_pointer = module;
        return;
    }
    // L0 message management
    int pub_type = l0_msg_handler(module, msg, (msg_t*)&luos_pub_msg);
    if (pub_type == L0_LED) {
        return;
    }
    if (pub_type != LUOS_PROTOCOL_NB) {
        luos_module_pointer = module;
        luos_pub = pub_type;
        return;
    }
    if ((module->rt >= 1) & (module->mod_cb!=0)){
            module->mod_cb(module, msg);
    }
    else {
        //store module and msg pointer
        mngr_set(module, msg);
    }
}

//************* Public functions *********************

void luos_init(void){
    module_number = 0;
    board_init();
    robus_init(luos_cb);
}


void luos_loop(void) {
    mngr_t chunk;
    if (luos_pub != LUOS_PROTOCOL_NB) {
      luos_send(luos_module_pointer, (msg_t*)&luos_pub_msg);
      luos_pub = LUOS_PROTOCOL_NB;
    }
    // filter stacked module with callback
    int i = get_next_cb_id();
    while( i >= 0){
        mngr_get(i, &chunk);
        chunk.module->mod_cb(chunk.module, chunk.msg);
        i = get_next_cb_id();
    }
}

void luos_modules_clear(void) {
    robus_modules_clear();
}


module_t* luos_module_create(MOD_CB mod_cb, unsigned char type, const char *alias) {
    unsigned char i = 0;
    module_t* module = &module_table[module_number];
    module->vm = robus_module_create(type);
    module->rt = 0;
    module->message_available = 0;

    // Link the module to his callback
    module->mod_cb = mod_cb;
    // Save default alias
    for (i=0; i < MAX_ALIAS_SIZE-1; i++) {
        module->default_alias[i] = alias[i];
        if (module->default_alias[i] == '\0')
            break;
    }
    module->default_alias[i] = '\0';
    // Initialise the module alias to 0
    memset((void *)module->alias, 0, sizeof(module->alias));
    if (!read_alias(module_number, (char *)module->alias)) {
        // if no alias saved keep the default one
        for (i=0; i < MAX_ALIAS_SIZE-1; i++) {
            module->alias[i] = alias[i];
            if (module->alias[i] == '\0')
                break;
        }
        module->alias[i] = '\0';
    }
    module_number++;
    return module;
}

void luos_module_enable_rt(module_t*module) {
    module->rt = 1;
}

unsigned char luos_send(module_t* module, msg_t *msg) {
    return robus_send(module->vm, msg);
}

unsigned char luos_send_data(module_t* module, msg_t*msg, void* bin_data, unsigned short size) {
    int start_index = 0;
    int data_size = size;
    return luos_send_ring_buffer(module, msg, bin_data, &data_size, &start_index, size);
}

unsigned char luos_send_ring_buffer(module_t* module, msg_t* msg, void* ring_buffer,
                                   int *data_size, int *start_index, int stop_index) {

    int msg_number = 1;
    int chunk_size;
    // compute number of message needed to send this data
    if (*data_size > MAX_DATA_MSG_SIZE) {
        msg_number = (*data_size / MAX_DATA_MSG_SIZE);
        msg_number += (msg_number*MAX_DATA_MSG_SIZE < *data_size);
    }
    for (volatile int chunk = 0; chunk < msg_number; chunk++) {
        if (*data_size > MAX_DATA_MSG_SIZE) {
            chunk_size = MAX_DATA_MSG_SIZE;
        } else {
            chunk_size = *data_size;
        }

        volatile int msg_data_index = 0;
        if ((stop_index - *start_index) <= chunk_size) {
            // save the first part of the data
            int remaining_space = (stop_index - *start_index - 1);
            memcpy(&msg->data[msg_data_index], &ring_buffer[*start_index], remaining_space);
            *start_index = 0;
            msg_data_index = remaining_space;
            chunk_size = chunk_size - remaining_space;
        }

        memcpy(&msg->data[msg_data_index], &ring_buffer[*start_index], chunk_size);
        *start_index = *start_index + chunk_size;
        msg->header.size = *data_size;
        luos_send(module, msg);
        if (*data_size > MAX_DATA_MSG_SIZE) {
            *data_size -= MAX_DATA_MSG_SIZE;
        } else {
            *data_size = 0;
        }
    }
    return 0;
}

unsigned char luos_get_data(module_t* module, msg_t* msg, void* bin_data) {
    static uint32_t data_size[MAX_VM_NUMBER] = {0};
    volatile int id = get_module_index(module);
    int start_index = 0;
    if (luos_get_ring_buffer(module, msg, bin_data, &data_size[id], &start_index, data_size[id] + msg->header.size)) {
        // data collection finished
        data_size[id] = 0;
        return 1;
    }
    return 0;
}

unsigned char luos_get_ring_buffer(module_t* module, msg_t* msg, void* ring_buffer,
                                   int* data_size, int *start_index, int stop_index) {
    unsigned short chunk_size = 0;

    // check parameters coherency
    if (*start_index > stop_index){
        while(1);
    }

    //compute the real start_index depending on current data size
    int copy_index = *data_size  + *start_index;
    if (copy_index > stop_index) {
        copy_index = stop_index - copy_index;
    }

    if (msg->header.size > MAX_DATA_MSG_SIZE)
        chunk_size = MAX_DATA_MSG_SIZE;
    else
        chunk_size = msg->header.size;
    *data_size = *data_size + chunk_size;
    // check if chunk size fit into the ring buffer
    if ((stop_index - copy_index) < chunk_size) {
        // save the first part of the data
        int remaining_space = (stop_index - copy_index - 1);
        memcpy(&ring_buffer[copy_index], msg->data, remaining_space);
        copy_index = 0;
        chunk_size = chunk_size - remaining_space;
    }
    memcpy(&ring_buffer[copy_index], msg->data, chunk_size);

    if (!(msg->header.size > MAX_DATA_MSG_SIZE)) {
        // data collection finished
        return 1;
    }
    return 0;
}

msg_t* luos_read(module_t* module) {
    if (module->message_available > MSG_BUFFER_SIZE) {
        // msg read too slow
        ctx.status.rx_error = TRUE;
    }
    if (module->message_available) {
        // check if there is a message for this module
        int i = get_next_module_id(module);
        if (i >= 0) {
            // this module have a message, get it
            mngr_t chunk;
            mngr_get(i, &chunk);
            return chunk.msg;
        }
    }
    return 0;
}

msg_t* luos_read_from(module_t* module, short id) {
    if (module->message_available) {
        // Get the next message manager id containing something for this module
        int mngr_module_id = get_next_module_id(module);
        if (mngr_module_id >= 0) {
            // check if there is a message from this id in this module
            int mngr_msg_id = get_next_msg_id(mngr_module_id, id);
            if (mngr_msg_id >= 0) {
                mngr_t chunk;
                mngr_get_msg(mngr_module_id, mngr_msg_id, &chunk);
                return chunk.msg;
            }
        }
    }
    return 0;
}

char luos_message_available(void) {
    return mngr_message_available();
}

void luos_save_alias(module_t* module, char* alias) {
    // Get module index
    int i = get_module_index(module);
    if (i >= 0) {
        write_alias(i, alias);
    }
}

void luos_set_baudrate(module_t* module, uint32_t baudrate) {
    msg_t msg;
    memcpy(msg.data, &baudrate, sizeof(uint32_t));
    msg.header.target_mode = BROADCAST;
    msg.header.target = BROADCAST_VAL;
    msg.header.cmd = SET_BAUDRATE;
    msg.header.size = sizeof(uint32_t);
    robus_send_sys(module, &msg);
}

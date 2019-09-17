#include "luos.h"
#include "l0.h"
#include "robus.h"
#include "sys_msg.h"
#include <string.h>

static module_t *luos_module_pointer;
static volatile msg_t luos_pub_msg;
static volatile int luos_pub = LUOS_PROTOCOL_NB;
module_t module_table[MAX_VM_NUMBER];
unsigned char module_number;

// no real time callback management
volatile int module_msg_available = 0;
volatile module_t* module_msg_fifo[MSG_BUFFER_SIZE];

/**
 * \struct module_t
 * \brief Module Structure
 *
 * This structure is used to manage modules
 * please refer to the documentation
 */
typedef struct __attribute__((__packed__)) fifo_t{
    module_t* module;
    msg_t* msg;                            /*!< msg ready to be read */
}fifo_t;

//**************** Private functions*********************
static int luos_msg_handler(module_t* module, msg_t* input, msg_t* output) {
    if (input->header.cmd == IDENTIFY_CMD) {
        output->header.cmd = INTRODUCTION_CMD;
        output->header.target_mode = ID;
        output->header.size = MAX_ALIAS_SIZE+1;
        output->header.target = input->header.source;
        for (int i=0; i<MAX_ALIAS_SIZE; i++) {
            output->data[i] = module->vm->alias[i];
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

void fifo_set(module_t* module, msg_t* msg) {
    if ((module_msg_available+1 > MSG_BUFFER_SIZE) || (module->message_available+1 > MSG_BUFFER_SIZE)) {
        // This new message doesn't fit into buffer, don't save it
        return;
    }
    module_msg_fifo[module_msg_available++] = module;
    module->msg_stack[module->message_available++] = msg;
}

void fifo_get_msg(int module_index,int msg_index, fifo_t* chunk){
    if ((module_index < 0) | (msg_index < 0)){
        return;
    }
    // get module
    chunk->module = module_msg_fifo[module_index];
    __disable_irq();
    for (int i = module_index; i < module_msg_available; i++) {
        module_msg_fifo[i] = module_msg_fifo[i+1];
    }
    module_msg_available--;
    __enable_irq();

    // get msg
    chunk->msg = chunk->module->msg_stack[msg_index];
    __disable_irq();
    for (int i = msg_index; i < chunk->module->message_available; i++) {
        chunk->module->msg_stack[i] = chunk->module->msg_stack[i+1];
    }
    chunk->module->message_available--;
    __enable_irq();
}

void fifo_get(int module_index, fifo_t* chunk){
    fifo_get_msg(module_index, 0, chunk);
}

// find the next message for a module with a callback
int get_next_cb_id() {
    for (int i = 0; i < module_msg_available; i++) {
        if (module_msg_fifo[i]->mod_cb){
            //there is a callback in this module return this id
            return i;
        }
    }
    return -1;
}

// find the next message for a specific module
int get_next_module_id(module_t* module) {
    for (int i = 0; i < module_msg_available; i++) {
        if (module_msg_fifo[i] == module){
            //there is the module we are looking for
            return i;
        }
    }
    return -1;
}

// find the next message from a specific id for a specific module
int get_next_msg_id(int fifo_id, short msg_from) {
    // find the next message from the specified id
    for (int i=0; i<module_msg_fifo[fifo_id]->message_available; i++) {
        if (module_msg_fifo[fifo_id]->msg_stack[i]->header.source == msg_from) {
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
        fifo_set(module, msg);
    }
}

//************* Public functions *********************

void luos_init(void){
    module_number = 0;
    robus_init(luos_cb);
}

void luos_loop(void) {
    fifo_t chunk;
    if (luos_pub != LUOS_PROTOCOL_NB) {
      luos_send(luos_module_pointer, (msg_t*)&luos_pub_msg);
      luos_pub = LUOS_PROTOCOL_NB;
    }
    // filter stacked module with callback
    int i = get_next_cb_id();
    while( i >= 0){
        fifo_get(i, &chunk);
        chunk.module->mod_cb(chunk.module, chunk.msg);
        i = get_next_cb_id();
    }
}

void luos_modules_clear(void) {
    robus_modules_clear();
}

module_t* luos_module_create(MOD_CB mod_cb, unsigned char type, const char *alias) {
    module_t* module = &module_table[module_number++];
    module->vm = robus_module_create(type, alias);
    module->rt = 0;
    module->message_available = 0;

    // Link the module to his callback
    module->mod_cb = mod_cb;
    return module;
}

void luos_module_enable_rt(module_t*module) {
    module->rt = 1;
}

unsigned char luos_send(module_t* module, msg_t *msg) {
    return robus_send(module->vm, msg);
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
            fifo_t chunk;
            fifo_get(i, &chunk);
            return chunk.msg;
        }
    }
    return 0;
}

msg_t* luos_read_from(module_t* module, short id) {
    if (module->message_available) {
        // Get the next fifo id containing something for this module
        int fifo_module_id = get_next_module_id(module);
        if (fifo_module_id >= 0) {
            // check if there is a message from this id in this module
            int fifo_msg_id = get_next_msg_id(fifo_module_id, id);
            if (fifo_msg_id >= 0) {
                fifo_t chunk;
                fifo_get_msg(fifo_module_id, fifo_msg_id, &chunk);
                return chunk.msg;
            }
        }
    }
    return 0;
}

char luos_message_available(void) {
    return module_msg_available;
}

unsigned char luos_send_alias(module_t* module, msg_t *msg) {
    msg->header.cmd = WRITE_ALIAS;
    return robus_send_sys(module->vm, msg);
}

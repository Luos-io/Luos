/*
 * message_mngr.h
 *
 *  Created on: 17 sept. 2019
 *      Author: nicolasrabault
 */

#ifndef MESSAGE_MNGR_H_
#define MESSAGE_MNGR_H_

#include "luos.h"
/**
 * \struct module_t
 * \brief Module Structure
 *
 * This structure is used to manage modules
 * please refer to the documentation
 */
typedef struct __attribute__((__packed__)) mngr_t{
    module_t* module;
    msg_t* msg;                            /*!< msg ready to be read */
}mngr_t;

char mngr_message_available(void);
void mngr_set(module_t* module, msg_t* msg);
void mngr_get_msg(int module_index,int msg_index, mngr_t* chunk);
void mngr_get(int module_index, mngr_t* chunk);
int get_next_cb_id();
int get_next_module_id(module_t* module);
int get_next_msg_id(int mngr_id, short msg_from);

#endif /* MESSAGE_MNGR_H_ */

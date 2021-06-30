#ifndef UNIT_TEST_H
#define UNIT_TEST_H

#include <stdio.h>
#include "luos_utils.h"


//#define UNIT_TEST_DEBUG


/*******************************************************************************
 * Function
 ******************************************************************************/
void reset_assert(void);
void assert_activation(uint8_t activation);
bool is_assert(void);

#endif //UNIT_TEST_H
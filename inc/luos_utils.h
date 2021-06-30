/******************************************************************************
 * @file luos utils
 * @brief some tools used to debug
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef LUOS_UTILS_H
#define LUOS_UTILS_H

#include <stdint.h>
#include <stdbool.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LUOS_ASSERTION

#ifdef LUOS_ASSERTION
#ifndef UNIT_TEST
#define LUOS_ASSERT(expr) \
    if (!(expr))          \
    Luos_assert(__FILE__, __LINE__)
#else
#define LUOS_ASSERT(expr) \
    if (!(expr))          \
    Luos_unittest_assert(__FILE__, __LINE__)
#endif
#else
#define LUOS_ASSERT(expr)
#endif

/* This structure is used to manage node assertion informations
 */
typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            uint32_t line;
            char file[100];
        };
        uint8_t unmap[100 + sizeof(uint32_t)];
    };
} luos_assert_t;

#ifdef UNIT_TEST
#include "robus_struct.h"
/* This structure is used for unit test assert
 */
typedef struct
{
    uint8_t enable; // enable = 0 to disable asserts
    uint8_t state;  // state = 1 when an assert has occured
    uint32_t file_size;
    uint32_t line_size;
    msg_t msg;
} ut_luos_assert_t;
#endif

/*******************************************************************************
 * Function
 ******************************************************************************/

void Luos_assert(char *file, uint32_t line);
void node_assert(char *file, uint32_t line);

#ifdef UNIT_TEST
void Luos_unittest_assert(char *file, uint32_t line);
#endif

#endif /* LUOS_UTILS_H */
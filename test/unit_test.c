/******************************************************************************
 * @file unit_test.c
 * @brief toolbox for unit testing
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "unit_test.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
extern ut_luos_assert_t ut_assert;

/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief Check if an assert has occured
 * @param None
 * @return True boolean if an assert has occured
 ******************************************************************************/
bool is_assert(void)
{
    if ((ut_assert.enable == 1) && (ut_assert.state == 1))
    {
        ut_assert.state = 0;

#ifdef UNIT_TEST_DEBUG
        uint32_t line = (ut_assert.msg.data[0]);
        if (ut_assert.line_size > 3)
        {
            line += (ut_assert.msg.data[2] << 4) + (ut_assert.msg.data[1] << 8) + (ut_assert.msg.data[3] << 12);
        }
        else if (ut_assert.line_size > 2)
        {
            line += (ut_assert.msg.data[2] << 4) + (ut_assert.msg.data[1] << 8);
        }
        else if (ut_assert.line_size > 1)
        {
            line += (ut_assert.msg.data[2] << 4);
        }

        printf("\n*\t[INFO] Assert message received\t");
        printf("- Line : ");
        printf("%d\n", line);
        printf("\t- File : ");
        for (int i = 0; i < ut_assert.file_size; i++)
        {
            printf("%c", ut_assert.msg.data[i + ut_assert.line_size]);
        }
        printf("\n");
#endif

        return true;
    }
    else
    {
        ut_assert.state = 0;
        return false;
    }
}

/******************************************************************************
 * @brief Reset assert state to authorize a new assert
 * @param None
 * @return None
 ******************************************************************************/
void reset_assert(void)
{
    ut_assert.state = 0;
}

/******************************************************************************
 * @brief Pull a message allocated to a specific module
 * @param activation : Set to 1 to activate asserts
 * @return None
 ******************************************************************************/
void assert_activation(uint8_t activation)
{
    if (activation)
    {
        ut_assert.enable = 1;
    }
    else
    {
        ut_assert.enable = 0;
    }
    reset_assert();
}

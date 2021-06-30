#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "unity.h"
#include "../test/unit_test.h"
#include "../Robus/inc/robus_struct.h"
#include "../src/msg_alloc.c"

void unittest_MsgAlloc_loop()
{
    //MsgAlloc_loop();  <-- TODO
}

void unittest_MsgAlloc_ValidHeader()
{
#define DATA_SIZE 64
    // Test Case 1 : Invalid header
    printf("\n--- Test Case 1 : Invalid header\n");
    MsgAlloc_Init(NULL);
    {
        uint8_t valid      = false;
        uint16_t data_size = 0;

        data_ptr    = (uint8_t *)&msg_buffer[0];
        current_msg = (msg_t *)&msg_buffer[1];

        MsgAlloc_ValidHeader(valid, data_size);
        TEST_ASSERT_EQUAL((uint8_t *)current_msg, data_ptr);
    }

    // Test Case 2 : Drop message
    printf("--- Test Case 2 : Drop message\n");
    MsgAlloc_Init(NULL);
    {
        uint8_t valid      = true;
        uint16_t data_size = DATA_SIZE;
        uint8_t *ptr       = (uint8_t *)&msg_buffer[0];

        memory_stats_t memory_stats = {.rx_msg_stack_ratio      = 0,
                                       .luos_stack_ratio        = 0,
                                       .tx_msg_stack_ratio      = 0,
                                       .buffer_occupation_ratio = 0,
                                       .msg_drop_number         = 0};

        memset(&memory_stats, 0, sizeof(memory_stats));
        MsgAlloc_Init(&memory_stats);

        current_msg = (msg_t *)ptr++;
        used_msg    = (msg_t *)ptr;

        MsgAlloc_ValidHeader(valid, data_size);

        TEST_ASSERT_EQUAL(100, memory_stats.buffer_occupation_ratio);
        TEST_ASSERT_EQUAL(1, memory_stats.msg_drop_number);
    }

    // Test Case 3 : End of msg_buffer
    printf("--- Test Case 3 : End of msg_buffer\n");
    MsgAlloc_Init(NULL);
    {
        uint8_t valid                        = true;
        uint16_t data_size                   = DATA_SIZE;
        current_msg                          = (msg_t *)&msg_buffer[MSG_BUFFER_SIZE - 1];
        header_t *expected_copy_task_pointer = (header_t *)current_msg;
        uint8_t *expected_data_ptr           = (uint8_t *)&msg_buffer[0] + sizeof(header_t);
        uint8_t *expected_data_end           = expected_data_ptr + data_size + CRC_SIZE;

        MsgAlloc_ValidHeader(valid, data_size);
        TEST_ASSERT_EQUAL(true, mem_clear_needed);
        TEST_ASSERT_EQUAL(expected_data_ptr, data_ptr);
        TEST_ASSERT_EQUAL(expected_data_end, data_end_estimation);
        TEST_ASSERT_EQUAL(expected_copy_task_pointer, copy_task_pointer);
    }

    // Test Case 4 : There is enough space
    printf("--- Test Case 4 : There is enough space\n");
    MsgAlloc_Init(NULL);
    {
        uint8_t *expected_data_end;
        uint8_t valid      = true;
        uint16_t data_size = DATA_SIZE;

        current_msg       = (msg_t *)&msg_buffer[0];
        expected_data_end = (uint8_t *)current_msg + sizeof(header_t) + data_size + CRC_SIZE;

        MsgAlloc_ValidHeader(valid, data_size);
        TEST_ASSERT_EQUAL(true, mem_clear_needed);
        TEST_ASSERT_EQUAL(expected_data_end, data_end_estimation);
    }
}

void unittest_MsgAlloc_InvalidMsg()
{
    //MsgAlloc_InvalidMsg();  <-- TODO
}

void unittest_MsgAlloc_EndMsg()
{
    //MsgAlloc_EndMsg();  <-- TODO
}

#define COPY_LENGTH 128
void unittest_MsgAlloc_SetData()
{
    // Test Case 1 : Set Data
    printf("\n--- Test Case 1 : Set Data\n");
    MsgAlloc_Init(NULL);
    {
        uint8_t expected_datas[COPY_LENGTH];

        // Init variables
        //---------------
        data_ptr = (uint8_t *)&msg_buffer[0];
        memset((void *)&msg_buffer[0], 0xFF, COPY_LENGTH);
        for (int i = 0; i < COPY_LENGTH; i++)
        {
            expected_datas[i] = i;
        }

        // Call function
        //---------------
        for (int i = 0; i < COPY_LENGTH; i++)
        {
            MsgAlloc_SetData(i);
        }

        // Verify
        //---------------
        TEST_ASSERT_EQUAL_MEMORY(expected_datas, (uint8_t *)&msg_buffer[0], COPY_LENGTH);
    }
}

void unittest_MsgAlloc_IsEmpty()
{
    // Test Case 1 : Is Empty
    printf("\n--- Test Case 1 : Is Empty\n");
    MsgAlloc_Init(NULL);
    {
        data_ptr = &msg_buffer[0];
        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_IsEmpty());

        for (int i = 1; i < MSG_BUFFER_SIZE; i++)
        {
            data_ptr = &msg_buffer[i];
            TEST_ASSERT_EQUAL(FAILED, MsgAlloc_IsEmpty());
        }
    }
}

void unittest_MsgAlloc_UsedMsgEnd()
{
    // Test Case 1 : Used Message End
    printf("\n--- Test Case 1 : Used Message End\n");
    MsgAlloc_Init(NULL);
    {
        used_msg = (msg_t *)&msg_buffer[0];
        MsgAlloc_UsedMsgEnd();
        TEST_ASSERT_NULL(used_msg);
    }
}

void unittest_MsgAlloc_PullMsgToInterpret()
{
    // Test Case 1 : Case Failed
    printf("\n--- Test Case 1 : Case FAILED\n");
    MsgAlloc_Init(NULL);
    {
        msg_t *returned_msg = NULL;
        msg_tasks_stack_id  = 0;
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_PullMsgToInterpret(&returned_msg));
    }

    // Test Case 2 : Case SUCCEED
    printf("--- Test Case 2 : Case SUCCEED\n");
    MsgAlloc_Init(NULL);
    {
        msg_t *expected_message[MAX_MSG_NB];
        msg_t *returned_msg;
        error_return_t result;

        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            expected_message[i] = (msg_t *)&msg_buffer[i];
            msg_tasks[i]        = (msg_t *)&msg_buffer[i];
        }

        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            msg_tasks_stack_id = MAX_MSG_NB;
            reset_assert();
            result = MsgAlloc_PullMsgToInterpret(&returned_msg);
            TEST_ASSERT_FALSE(is_assert());
            TEST_ASSERT_EQUAL(SUCCEED, result);
            TEST_ASSERT_EQUAL(expected_message[i], returned_msg);
        }
    }
}

void unittest_MsgAlloc_LuosTaskAlloc()
{
    // Test Case 1 : No more space in luos_tasks
    printf("\n--- Test Case 1 : No more space in luos_tasks\n");
    MsgAlloc_Init(NULL);
    {
        luos_task_t expected_luos_task;
        ll_container_t container;

        // Init variables
        memory_stats_t memory_stats = {.rx_msg_stack_ratio      = 0,
                                       .luos_stack_ratio        = 0,
                                       .tx_msg_stack_ratio      = 0,
                                       .buffer_occupation_ratio = 0,
                                       .msg_drop_number         = 0};

        memset(&memory_stats, 0, sizeof(memory_stats));
        MsgAlloc_Init(&memory_stats);

        luos_tasks_stack_id = MAX_MSG_NB;
        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            luos_tasks[i].msg_pt = (msg_t *)&msg_buffer[i];
        }
        luos_tasks[0].msg_pt = NULL;

        // Launch Test
        MsgAlloc_LuosTaskAlloc(&container, (msg_t *)&msg_buffer[0]);

        // Verify
        TEST_ASSERT_EQUAL(100, memory_stats.luos_stack_ratio);
        TEST_ASSERT_EQUAL(1, memory_stats.msg_drop_number);
        TEST_ASSERT_EQUAL(&msg_buffer[1], luos_tasks[0].msg_pt);
    }

    // Test Case 2 :  Allocation
    printf("--- Test Case 2 : Allocation\n");
    MsgAlloc_Init(NULL);
    {
        msg_t *message;
        ll_container_t *container_concerned;
        uint16_t expected_luos_tasks_stack_id;
        uint8_t expected_mem_stat;

        // Init variables
        memory_stats_t memory_stats = {.rx_msg_stack_ratio      = 0,
                                       .luos_stack_ratio        = 0,
                                       .tx_msg_stack_ratio      = 0,
                                       .buffer_occupation_ratio = 0,
                                       .msg_drop_number         = 0};
        memset(&memory_stats, 0, sizeof(memory_stats));
        MsgAlloc_Init(&memory_stats);

        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            // Init variables
            luos_tasks_stack_id          = i;
            expected_luos_tasks_stack_id = i + 1;
            expected_mem_stat            = (i + 1) * 10;
            message                      = (msg_t *)&msg_buffer[0];
            container_concerned          = (ll_container_t *)&msg_buffer[0];

            // Launch Test
            MsgAlloc_LuosTaskAlloc(container_concerned, message);

            // Verify
            TEST_ASSERT_EQUAL(message, luos_tasks[i].msg_pt);
            TEST_ASSERT_EQUAL(container_concerned, luos_tasks[i].ll_container_pt);
            TEST_ASSERT_EQUAL(expected_luos_tasks_stack_id, luos_tasks_stack_id);
            TEST_ASSERT_EQUAL(luos_tasks[0].msg_pt, oldest_msg);
            TEST_ASSERT_EQUAL(expected_mem_stat, memory_stats.luos_stack_ratio);
        }
    }
}

void unittest_MsgAlloc_PullMsg()
{
    // Test Case 1 :  Case Failed
    printf("\n--- Test Case 1 : Case Failed\n");
    MsgAlloc_Init(NULL);
    {
        msg_t *returned_message;
        ll_container_t *module = (ll_container_t *)0xFFFF;

        // Init variables
        luos_tasks_stack_id = MAX_MSG_NB - 1;
        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            luos_tasks[i].ll_container_pt = (ll_container_t *)i;
        }

        // Launch Test & Verify
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_PullMsg(module, &returned_message));
    }

    // Test Case 2 :  Case Succeed
    printf("--- Test Case 2 : Case Succeed\n");
    MsgAlloc_Init(NULL);
    {
        msg_t *returned_message;
        msg_t *msg_to_clear;
        ll_container_t *module;
        luos_task_t expected_luos_tasks[MAX_MSG_NB];

        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            // Init variables
            MsgAlloc_Init(NULL);
            luos_tasks_stack_id = MAX_MSG_NB;
            for (int j = 0; j < MAX_MSG_NB; j++)
            {
                luos_tasks[j].ll_container_pt = (ll_container_t *)&msg_buffer[j];
                luos_tasks[j].msg_pt          = (msg_t *)(&msg_buffer[0] + MAX_MSG_NB + j);
                expected_luos_tasks[j].msg_pt = (msg_t *)(&msg_buffer[0] + MAX_MSG_NB + j);
            }
            module       = luos_tasks[i].ll_container_pt;
            msg_to_clear = luos_tasks[i].msg_pt;

            // Launch Test & Verify
            TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_PullMsg(module, &returned_message));
            TEST_ASSERT_EQUAL(expected_luos_tasks[i].msg_pt, returned_message);
            TEST_ASSERT_EQUAL(returned_message, used_msg);
            // Verify required message has been deleted
            for (int k = 0; k < MAX_MSG_NB; k++)
            {
                TEST_ASSERT_NOT_EQUAL(msg_to_clear, luos_tasks[k].msg_pt);
            }
        }
    }
}

void unittest_MsgAlloc_PullMsgFromLuosTask()
{
    //MsgAlloc_PullMsgFromLuosTask();    <-- TODO
}

void unittest_MsgAlloc_LookAtLuosTask()
{
    // Test Case 1 :  Case Failed
    printf("\n--- Test Case 1 : Case Failed\n");
    MsgAlloc_Init(NULL);
    {
        uint16_t task_id;
        ll_container_t **allocated_module;

        // Init variables
        luos_tasks_stack_id = 0;
        task_id             = luos_tasks_stack_id;

        // Call function & Verify
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_LookAtLuosTask(task_id, allocated_module));
        task_id++;
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_LookAtLuosTask(task_id, allocated_module));
    }

    // Test Case 2 :  Case Success
    printf("--- Test Case 2 : Case Suceess\n");

    MsgAlloc_Init(NULL);
    {
        // Init variables
        ll_container_t *oldest_ll_container = NULL;
        luos_tasks_stack_id                 = 10;

        for (uint32_t i = 0; i < MAX_MSG_NB; i++)
        {
            luos_tasks[i].ll_container_pt = (ll_container_t *)i;
        }

        // Call function & Verify
        for (uint16_t i = 0; i < MAX_MSG_NB; i++)
        {
            TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_LookAtLuosTask(i, &oldest_ll_container));
            TEST_ASSERT_EQUAL(i, oldest_ll_container);
        }
    }
}

void unittest_MsgAlloc_GetLuosTaskSourceId()
{
    // Test Case 1 :  Case Failed
    printf("\n--- Test Case 1 : Case Failed\n");
    MsgAlloc_Init(NULL);
    {
        uint16_t task_id;
        uint16_t task_id_2;

        // Init variables
        luos_tasks_stack_id = MAX_MSG_NB;
        task_id             = luos_tasks_stack_id;
        task_id_2           = luos_tasks_stack_id + 1;

        // Call function & Verify
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_GetLuosTaskSourceId(task_id, NULL));
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_GetLuosTaskSourceId(task_id_2, NULL));
    }

    // Test Case 2 :  Case Success
    printf("--- Test Case 2 : Case Success\n");
    MsgAlloc_Init(NULL);
    {
        // Init variables
        msg_t message;
        uint16_t source_id  = 0;
        luos_tasks_stack_id = MAX_MSG_NB;

        for (uint16_t i = 0; i < MAX_MSG_NB; i++)
        {
            message.header.source = i;
            luos_tasks[i].msg_pt  = &message;

            // Call function & Verify
            TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_GetLuosTaskSourceId(i, &source_id));
            TEST_ASSERT_EQUAL(i, source_id);
        }
    }
}

void unittest_MsgAlloc_GetLuosTaskCmd()
{
    // Test Case 1 :  Case Failed
    printf("\n--- Test Case 1 : Case Failed\n");
    MsgAlloc_Init(NULL);
    {
        uint16_t task_id;
        uint8_t command = 0xFF;

        luos_tasks_stack_id = 10;
        task_id             = luos_tasks_stack_id;

        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_GetLuosTaskCmd(task_id, &command));
        TEST_ASSERT_EQUAL(0xFF, command);

        task_id++;
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_GetLuosTaskCmd(task_id, &command));
        TEST_ASSERT_EQUAL(0xFF, command);
    }

    // Test Case 2 :  Case Success
    printf("--- Test Case 2 : Case Success\n");
    MsgAlloc_Init(NULL);
    {
        msg_t message;
        uint16_t task_id         = 0;
        uint8_t command          = 0;
        uint8_t expected_command = 1;

        luos_tasks_stack_id = 10;

        message.header.cmd         = expected_command;
        luos_tasks[task_id].msg_pt = &message;

        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_GetLuosTaskCmd(task_id, &command));
        TEST_ASSERT_EQUAL(expected_command, command);
    }
}

void unittest_MsgAlloc_GetLuosTaskSize()
{
    // Test Case 1 :  Case Failed
    printf("\n--- Test Case 1 : Case Failed\n");
    MsgAlloc_Init(NULL);
    {
        uint16_t task_id;
        uint16_t size       = 0xFF;
        luos_tasks_stack_id = 10;
        task_id             = luos_tasks_stack_id;

        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_GetLuosTaskSize(task_id, &size));
        TEST_ASSERT_EQUAL(0xFF, size);
    }

    // Test Case 2 :  Case Success
    printf("--- Test Case 2 : Case Success\n");
    MsgAlloc_Init(NULL);
    {
        msg_t message;
        uint16_t expected_size = 128;
        uint16_t task_id       = 0;
        uint16_t size          = 0;

        luos_tasks_stack_id = 10;

        message.header.size        = expected_size;
        luos_tasks[task_id].msg_pt = &message;

        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_GetLuosTaskSize(task_id, &size));
        TEST_ASSERT_EQUAL(expected_size, size);
    }
}

void unittest_MsgAlloc_LuosTasksNbr()
{
    //MsgAlloc_LuosTasksNbr();    <-- TODO
}

void unittest_MsgAlloc_ClearMsgFromLuosTasks()
{
    // Test Case 1 :  No message to clear
    printf("\n--- Test Case 1 : No message to clear\n");
    MsgAlloc_Init(NULL);
    {
        luos_task_t expected_luos_tasks[MAX_MSG_NB];
        msg_t message;
        ll_container_t container;

        // Init variables
        luos_tasks_stack_id = 0;
        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            luos_tasks[i].msg_pt                   = &message;
            luos_tasks[i].ll_container_pt          = &container;
            expected_luos_tasks[i].msg_pt          = &message;
            expected_luos_tasks[i].ll_container_pt = &container;
            // Call function
            MsgAlloc_ClearMsgFromLuosTasks(luos_tasks[i].msg_pt);

            TEST_ASSERT_EQUAL(expected_luos_tasks[i].msg_pt, luos_tasks[i].msg_pt);
            TEST_ASSERT_EQUAL(expected_luos_tasks[i].ll_container_pt, luos_tasks[i].ll_container_pt);
        }
    }

    // Test Case 2 :  clear all messages
    printf("--- Test Case 2 : Clear a message\n");
    MsgAlloc_Init(NULL);
    {
        msg_t *msg_to_clear;

        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            // Init variables
            MsgAlloc_Init(NULL);
            for (int j = 0; j < MAX_MSG_NB; j++)
            {
                luos_tasks[j].msg_pt = (msg_t *)&msg_buffer[j];
            }

            msg_tasks[0]        = 0;
            tx_tasks[0].data_pt = 0;
            luos_tasks_stack_id = MAX_MSG_NB;
            msg_to_clear        = luos_tasks[i].msg_pt;
            // Call function
            MsgAlloc_ClearMsgFromLuosTasks(luos_tasks[i].msg_pt);

            // Verify required message has been deleted
            for (int k = 0; k < MAX_MSG_NB; k++)
            {
                TEST_ASSERT_NOT_EQUAL(msg_to_clear, luos_tasks[k].msg_pt);
            }
        }
    }
}

void unittest_MsgAlloc_SetTxTask()
{
    //MsgAlloc_SetTxTask();    <-- TODO
}

void unittest_MsgAlloc_PullMsgFromTxTask()
{
    //MsgAlloc_PullMsgFromTxTask()    <-- TODO
}

void unittest_MsgAlloc_PullContainerFromTxTask()
{
    //MsgAlloc_PullContainerFromTxTask()    <-- TODO
}

void unittest_MsgAlloc_GetTxTask()
{
    //MsgAlloc_GetTxTask()    <-- TODO
}

void unittest_MsgAlloc_TxAllComplete()
{
    // Test Case 1 :  Tx All Complete
    printf("\n--- Test Case 1 : Tx All Complete\n");
    MsgAlloc_Init(NULL);
    {
        tx_tasks_stack_id = 0;
        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_TxAllComplete());
        while (tx_tasks_stack_id < MSG_BUFFER_SIZE)
        {
            tx_tasks_stack_id++;
            TEST_ASSERT_EQUAL(FAILED, MsgAlloc_TxAllComplete());
        }
    }
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    //RUN_TEST(unittest_MsgAlloc_loop); // TODO
    RUN_TEST(unittest_MsgAlloc_ValidHeader);
    //RUN_TEST(unittest_MsgAlloc_InvalidMsg); // TODO
    //RUN_TEST(unittest_MsgAlloc_EndMsg);     // TODO

    RUN_TEST(unittest_MsgAlloc_SetData);
    RUN_TEST(unittest_MsgAlloc_IsEmpty);
    RUN_TEST(unittest_MsgAlloc_UsedMsgEnd);
    RUN_TEST(unittest_MsgAlloc_GetLuosTaskSourceId);
    RUN_TEST(unittest_MsgAlloc_GetLuosTaskCmd);
    RUN_TEST(unittest_MsgAlloc_GetLuosTaskSize);
    RUN_TEST(unittest_MsgAlloc_TxAllComplete);

    RUN_TEST(unittest_MsgAlloc_PullMsgToInterpret);
    RUN_TEST(unittest_MsgAlloc_LuosTaskAlloc);
    RUN_TEST(unittest_MsgAlloc_PullMsg);
    //RUN_TEST(unittest_MsgAlloc_PullMsgFromLuosTask);     // TODO
    RUN_TEST(unittest_MsgAlloc_LookAtLuosTask);
    RUN_TEST(unittest_MsgAlloc_ClearMsgFromLuosTasks);
    //RUN_TEST(unittest_MsgAlloc_SetTxTask);               // TODO
    //RUN_TEST(unittest_MsgAlloc_PullMsgFromTxTask);       // TODO
    //RUN_TEST(unittest_MsgAlloc_PullContainerFromTxTask); // TODO
    //RUN_TEST(unittest_MsgAlloc_GetTxTask);               // TODO

    //MsgAlloc_Init         => this function doesn't need unit test
    //MsgAlloc_LuosTasksNbr => this function doesn't need unit test

    UNITY_END();
}

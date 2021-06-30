#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "unity.h"
#include "../Robus/inc/robus_struct.h"
#include "../test/unit_test.h"
#include "../src/msg_alloc.c"

void unittest_DoWeHaveSpace(void)
{
    // Test Case 1 : there is enough space
    printf("\n--- Test Case 1 : There is enough space\n");
    MsgAlloc_Init(NULL);
    {
        for (int i = 0; i < MSG_BUFFER_SIZE; i++)
        {
            TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_DoWeHaveSpace((void *)&msg_buffer[i]));
        }
    }

    // Test Case 2 : there is not enough space
    printf("--- Test Case 2 : There is not enough space\n");
    MsgAlloc_Init(NULL);
    {
        for (int i = MSG_BUFFER_SIZE; i < MSG_BUFFER_SIZE + 10; i++)
        {
            TEST_ASSERT_EQUAL(FAILED, MsgAlloc_DoWeHaveSpace((void *)&msg_buffer[i]));
        }
    }
}

void unittest_CheckMsgSpace(void)
{
    // Test Case
    printf("\n--- Test Case 1 : Check Message Space\n");
    MsgAlloc_Init(NULL);
    {
        // Declaration of dummy message start and end pointer
        uint32_t *mem_start;
        uint32_t *mem_end;

        // Initialize pointer to buffer beginning
        used_msg = (msg_t *)&msg_buffer[0];

        // Test function considering "used_msg"
        // ("oldest_message" unit testing is useless as it's the same behaviour)
        mem_start = (uint32_t *)used_msg - 2;
        mem_end   = mem_start + 1;
        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_CheckMsgSpace((void *)mem_start, (void *)mem_end));

        mem_start = (uint32_t *)used_msg - 1;
        mem_end   = mem_start + 1;
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_CheckMsgSpace((void *)mem_start, (void *)mem_end));

        mem_start = (uint32_t *)used_msg;
        mem_end   = mem_start + 1;
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_CheckMsgSpace((void *)mem_start, (void *)mem_end));

        mem_start = (uint32_t *)used_msg + 1;
        mem_end   = mem_start + 1;
        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_CheckMsgSpace((void *)mem_start, (void *)mem_end));
    }
}

void unittest_BufferAvailableSpaceComputation(void)
{
    // Test Case 1 : Verify assertion cases
    printf("\n--- Test Case 1 : Verify assertion cases\n");
    MsgAlloc_Init(NULL);
    {
#define assert_nb 6
        typedef struct
        {
            bool expected_asserts;
            msg_t *oldest_msg_position;
        } assert_scenario;

        assert_scenario assert_sc[assert_nb];
        uint32_t free_space = MSG_BUFFER_SIZE;

        // Expected Values
        assert_sc[0].expected_asserts    = true;
        assert_sc[0].oldest_msg_position = (msg_t *)&msg_buffer[0] - 1;
        assert_sc[1].expected_asserts    = false;
        assert_sc[1].oldest_msg_position = (msg_t *)&msg_buffer[0];
        assert_sc[2].expected_asserts    = false;
        assert_sc[2].oldest_msg_position = (msg_t *)&msg_buffer[1];
        assert_sc[3].expected_asserts    = false;
        assert_sc[3].oldest_msg_position = (msg_t *)&msg_buffer[MSG_BUFFER_SIZE - 1];
        assert_sc[4].expected_asserts    = true;
        assert_sc[4].oldest_msg_position = (msg_t *)&msg_buffer[MSG_BUFFER_SIZE];
        assert_sc[5].expected_asserts    = true;
        assert_sc[5].oldest_msg_position = (msg_t *)&msg_buffer[MSG_BUFFER_SIZE] + 1;

        // Launch test
        for (int i = 0; i < assert_nb; i++)
        {
            reset_assert();
            oldest_msg = assert_sc[i].oldest_msg_position;
            MsgAlloc_BufferAvailableSpaceComputation();
            TEST_ASSERT_EQUAL(assert_sc[i].expected_asserts, is_assert());
        }
    }

    // Test Case 2 : No tasks availabled
    printf("--- Test Case 2 : No tasks availabled\n");
    MsgAlloc_Init(NULL);
    {
        uint32_t remaining_datas;
        uint32_t expected_size = 0;
        uint32_t free_space    = 0;
        oldest_msg             = (msg_t *)0xFFFFFFFF; //No oldest message

        for (current_msg = (msg_t *)&msg_buffer[0]; current_msg < (msg_t *)&msg_buffer[MSG_BUFFER_SIZE]; current_msg++)
        {
            for (int i = 0; i < sizeof(msg_t); i++)
            {
                reset_assert();
                remaining_datas = (uint32_t)data_end_estimation - (uint32_t)current_msg;
                expected_size   = MSG_BUFFER_SIZE - remaining_datas;

                free_space = MsgAlloc_BufferAvailableSpaceComputation();
                TEST_ASSERT_FALSE(is_assert());
                TEST_ASSERT_EQUAL(expected_size, free_space);

                data_end_estimation++;
            }
        }
    }

    // Test Case 3 : Oldest task is between `data_end_estimation` and the end of the buffer
    printf("--- Test Case 3 : Oldest task is between `data_end_estimation` and the end of the buffer\n");
    MsgAlloc_Init(NULL);
    {
        uint32_t free_space    = 0;
        uint32_t expected_size = 0;
        oldest_msg             = (msg_t *)&msg_buffer[1];
        data_end_estimation    = (uint8_t *)oldest_msg - 1;

        while (data_end_estimation < &msg_buffer[MSG_BUFFER_SIZE])
        {
            oldest_msg = (msg_t *)&msg_buffer[1];
            while (oldest_msg < (msg_t *)&msg_buffer[MSG_BUFFER_SIZE])
            {
                if ((uint32_t)oldest_msg > (uint32_t)data_end_estimation)
                {
                    reset_assert();
                    expected_size = (uint32_t)oldest_msg - (uint32_t)data_end_estimation;
                    free_space    = MsgAlloc_BufferAvailableSpaceComputation();

                    TEST_ASSERT_FALSE(is_assert());
                    TEST_ASSERT_EQUAL(expected_size, free_space);
                }
                oldest_msg++;
            }
            data_end_estimation += sizeof(msg_t);
        }
    }

    // Test Case 3 : Oldest task is between the begin of the buffer and `current_msg`
    printf("--- Test Case 4 : Oldest task is between the begin of the buffer and `current_msg`\n");
    MsgAlloc_Init(NULL);
    {
        uint32_t free_space    = 0;
        uint32_t expected_size = 0;
        data_end_estimation    = (uint8_t *)&msg_buffer[1];
        oldest_msg             = (msg_t *)data_end_estimation - 1;

        while (data_end_estimation < &msg_buffer[MSG_BUFFER_SIZE])
        {
            oldest_msg = (msg_t *)&msg_buffer[1];
            while (oldest_msg < (msg_t *)&msg_buffer[MSG_BUFFER_SIZE])
            {
                if ((uint32_t)oldest_msg < (uint32_t)data_end_estimation)
                {
                    reset_assert();
                    expected_size = (uint32_t)&msg_buffer[MSG_BUFFER_SIZE] - (uint32_t)data_end_estimation;
                    expected_size += (uint32_t)oldest_msg - (uint32_t)&msg_buffer[0];
                    free_space = MsgAlloc_BufferAvailableSpaceComputation();

                    TEST_ASSERT_FALSE(is_assert());
                    TEST_ASSERT_EQUAL(expected_size, free_space);
                }
                oldest_msg++;
            }
            data_end_estimation += sizeof(msg_t);
        }
    }
}

void unittest_OldestMsgCandidate(void)
{
    // Test Case 1 : Verify assertion cases
    printf("\n--- Test Case 1 : Verify assertion cases\n");
    MsgAlloc_Init(NULL);
    {
#define assert_nb 6
        typedef struct
        {
            bool expected_asserts;
            msg_t *oldest_stack_msg_pt;
        } assert_scenario;

        assert_scenario assert_sc[assert_nb];
        uint32_t free_space = MSG_BUFFER_SIZE;

        // Expected Values
        assert_sc[0].expected_asserts    = true;
        assert_sc[0].oldest_stack_msg_pt = (msg_t *)&msg_buffer[0] - 1;
        assert_sc[1].expected_asserts    = false;
        assert_sc[1].oldest_stack_msg_pt = (msg_t *)&msg_buffer[0];
        assert_sc[2].expected_asserts    = false;
        assert_sc[2].oldest_stack_msg_pt = (msg_t *)&msg_buffer[1];
        assert_sc[3].expected_asserts    = false;
        assert_sc[3].oldest_stack_msg_pt = (msg_t *)&msg_buffer[MSG_BUFFER_SIZE - 1];
        assert_sc[4].expected_asserts    = true;
        assert_sc[4].oldest_stack_msg_pt = (msg_t *)&msg_buffer[MSG_BUFFER_SIZE];
        assert_sc[5].expected_asserts    = true;
        assert_sc[5].oldest_stack_msg_pt = (msg_t *)&msg_buffer[MSG_BUFFER_SIZE] + 1;

        // Launch test
        for (int i = 0; i < assert_nb; i++)
        {
            reset_assert();
            MsgAlloc_OldestMsgCandidate(assert_sc[i].oldest_stack_msg_pt);
            TEST_ASSERT_EQUAL(assert_sc[i].expected_asserts, is_assert());
        }
    }

    // Test Case 2 : Verify case "oldest_stack_msg_pt" == 0
    printf("--- Test Case 2 : Verify case \"oldest_stack_msg_pt\" == 0\n");
    MsgAlloc_Init(NULL);
    {
        oldest_msg                          = (msg_t *)&msg_buffer[0];
        volatile msg_t *expected_oldest_msg = oldest_msg;
        msg_t *oldest_stack_msg_pt          = NULL;

        reset_assert();
        MsgAlloc_OldestMsgCandidate(oldest_stack_msg_pt);
        TEST_ASSERT_FALSE(is_assert());
        TEST_ASSERT_EQUAL(expected_oldest_msg, oldest_msg);
    }

    // Test Case 3 : Verify other cases
    printf("--- Test Case 3 : Verify other cases\n");
    MsgAlloc_Init(NULL);
    {
#define CASE 18
        msg_t *oldest_stack_msg_pt;
        volatile msg_t *expected_oldest_msg;

        int cases[CASE][4] = {//current_msg  oldest_msg  oldest_stack_msg_pt  expected_oldest_msg
                              {0, 1, 2, 1},
                              {0, 2, 1, 1},
                              {1, 0, 2, 2},
                              {1, 2, 0, 2},
                              {2, 0, 1, 0},
                              {2, 1, 0, 0},
                              {0, 1, 0, 1},
                              {0, 2, 0, 2},
                              {1, 0, 1, 0},
                              {1, 2, 1, 2},
                              {2, 0, 2, 0},
                              {2, 1, 2, 1},
                              {0, 1, 1, 1},
                              {0, 2, 2, 2},
                              {1, 0, 0, 0},
                              {1, 2, 2, 2},
                              {2, 0, 0, 0},
                              {2, 1, 1, 1}};

        for (int i = 0; i < CASE; i++)
        {
            current_msg         = (msg_t *)&msg_buffer[cases[i][0]];
            oldest_msg          = (msg_t *)&msg_buffer[cases[i][1]];
            oldest_stack_msg_pt = (msg_t *)&msg_buffer[cases[i][2]];
            expected_oldest_msg = (msg_t *)&msg_buffer[cases[i][3]];
            MsgAlloc_OldestMsgCandidate(oldest_stack_msg_pt);
            TEST_ASSERT_EQUAL(expected_oldest_msg, oldest_msg);
        }
    }
}

void unittest_ValidDataIntegrity(void)
{
    // Test Case 1 : No copy needed
    printf("\n--- Test Case 1 : No copy needed\n");
    MsgAlloc_Init(NULL);
    {
        uint8_t expected_msg_buffer[MSG_BUFFER_SIZE];
        mem_clear_needed = 0;
        copy_task_pointer == NULL;

        memset((void *)&msg_buffer[0], 0, MSG_BUFFER_SIZE);
        memset((void *)&expected_msg_buffer[0], 0, MSG_BUFFER_SIZE);
        reset_assert();
        MsgAlloc_ValidDataIntegrity();
        TEST_ASSERT_FALSE(is_assert());
        TEST_ASSERT_EQUAL_MEMORY(expected_msg_buffer, msg_buffer, MSG_BUFFER_SIZE);
    }

    // Test Case 2 : Copy header to begin of message buffer
    printf("--- Test Case 2 : Copy header to begin of message buffer\n");
    MsgAlloc_Init(NULL);
    {
        uint8_t expected_msg_buffer[MSG_BUFFER_SIZE];
        mem_clear_needed  = 0;
        copy_task_pointer = (header_t *)&msg_buffer[sizeof(msg_t)];

        memset((void *)&msg_buffer[0], 0, MSG_BUFFER_SIZE);
        memset((void *)&expected_msg_buffer[0], 0, MSG_BUFFER_SIZE);
        memset((void *)&msg_buffer[sizeof(msg_t)], 1, sizeof(header_t));
        memset((void *)&expected_msg_buffer[0], 1, sizeof(header_t));

        reset_assert();
        MsgAlloc_ValidDataIntegrity();

        TEST_ASSERT_FALSE(is_assert());
        TEST_ASSERT_EQUAL(copy_task_pointer, NULL);
        TEST_ASSERT_EQUAL_MEMORY(expected_msg_buffer, msg_buffer, sizeof(header_t));
    }

    // Test Case 3 : Verify memory cleaning
    printf("--- Test Case 3 : Verify memory cleaning\n");
    MsgAlloc_Init(NULL);
    {
        mem_clear_needed    = 1;
        current_msg         = (msg_t *)&msg_buffer[0];
        data_end_estimation = (uint8_t *)(current_msg + 1);
        reset_assert();
        MsgAlloc_ValidDataIntegrity();
        TEST_ASSERT_FALSE(is_assert());
    }
}

void unittest_ClearMsgSpace(void)
{
    // Test Case 1 : there is not enough space in memory
    printf("\n--- Test Case 1 : There is not enough space in memory\n");
    MsgAlloc_Init(NULL);
    {
        void *memory_start = (void *)&msg_buffer[0];
        void *memory_end   = (void *)&msg_buffer[MSG_BUFFER_SIZE];
        TEST_ASSERT_EQUAL(FAILED, MsgAlloc_ClearMsgSpace(memory_start, memory_end));
    }

    // Test Case 2 : Drop used messages
    printf("--- Test Case 2 : Drop used messages\n");
    MsgAlloc_Init(NULL);
    {
        void *memory_start;
        void *memory_end;
        memory_stats_t memory_stats = {.rx_msg_stack_ratio      = 0,
                                       .luos_stack_ratio        = 0,
                                       .tx_msg_stack_ratio      = 0,
                                       .buffer_occupation_ratio = 0,
                                       .msg_drop_number         = 0};

        memset(&memory_stats, 0, sizeof(memory_stats));
        MsgAlloc_Init(&memory_stats);

        used_msg     = (msg_t *)&msg_buffer[1];
        memory_start = (void *)&msg_buffer[0];
        memory_end   = (void *)&msg_buffer[2];

        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_ClearMsgSpace(memory_start, memory_end));
        TEST_ASSERT_EQUAL(100, memory_stats.buffer_occupation_ratio);
        TEST_ASSERT_EQUAL(1, memory_stats.msg_drop_number);
        TEST_ASSERT_NULL(used_msg);

        for (int i = 1; i < 0xFF; i++)
        {
            used_msg = (msg_t *)&msg_buffer[1];
            MsgAlloc_ClearMsgSpace(memory_start, memory_end);
            TEST_ASSERT_EQUAL(i + 1, memory_stats.msg_drop_number);
        }

        used_msg                     = (msg_t *)&msg_buffer[1];
        memory_stats.msg_drop_number = 255;
        MsgAlloc_ClearMsgSpace(memory_start, memory_end);
        TEST_ASSERT_EQUAL(255, memory_stats.msg_drop_number);
    }

    // Test Case 3 :  Drop all messages from luos_tasks
    printf("--- Test Case 3 : Drop all messages from luos_tasks\n");
    MsgAlloc_Init(NULL);
    {
        void *memory_start;
        void *memory_end;
        memory_stats_t memory_stats = {.rx_msg_stack_ratio      = 0,
                                       .luos_stack_ratio        = 0,
                                       .tx_msg_stack_ratio      = 0,
                                       .buffer_occupation_ratio = 0,
                                       .msg_drop_number         = 0};

        memset(&memory_stats, 0, sizeof(memory_stats));
        MsgAlloc_Init(&memory_stats);

        for (int i = 0; i < MAX_MSG_NB - 2; i++)
        {
            luos_tasks[i].msg_pt          = (msg_t *)&msg_buffer[i + 2];
            luos_tasks[i].ll_container_pt = (ll_container_t *)&msg_buffer[i + 2];
        }
        used_msg            = (msg_t *)&msg_buffer[0];
        oldest_msg          = (msg_t *)&msg_buffer[2];
        memory_start        = (void *)&msg_buffer[1];
        memory_end          = (void *)&msg_buffer[MAX_MSG_NB - 1];
        luos_tasks_stack_id = MAX_MSG_NB;

        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_ClearMsgSpace(memory_start, memory_end));
        TEST_ASSERT_EQUAL(100, memory_stats.buffer_occupation_ratio);
        TEST_ASSERT_EQUAL(2, luos_tasks_stack_id);
        TEST_ASSERT_EQUAL(8, memory_stats.msg_drop_number);
        for (int i = 0; i < MAX_MSG_NB - 2; i++)
        {
            TEST_ASSERT_EQUAL(0, luos_tasks[i].msg_pt);
            TEST_ASSERT_EQUAL(0, luos_tasks[i].ll_container_pt);
        }
    }

    // Test Case 4 :  Drop all messages from msg_tasks
    printf("--- Test Case 4 : Drop all messages from msg_tasks\n");
    MsgAlloc_Init(NULL);
    {
        void *memory_start;
        void *memory_end;
        memory_stats_t memory_stats = {.rx_msg_stack_ratio      = 0,
                                       .luos_stack_ratio        = 0,
                                       .tx_msg_stack_ratio      = 0,
                                       .buffer_occupation_ratio = 0,
                                       .msg_drop_number         = 0};

        memset(&memory_stats, 0, sizeof(memory_stats));
        MsgAlloc_Init(&memory_stats);

        for (int i = 0; i < MAX_MSG_NB - 2; i++)
        {
            msg_tasks[i] = (msg_t *)&msg_buffer[i + 2];
        }
        used_msg           = (msg_t *)&msg_buffer[0];
        oldest_msg         = (msg_t *)&msg_buffer[2];
        memory_start       = (void *)&msg_buffer[1];
        memory_end         = (void *)&msg_buffer[MAX_MSG_NB - 1];
        msg_tasks_stack_id = MAX_MSG_NB;

        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_ClearMsgSpace(memory_start, memory_end));
        TEST_ASSERT_EQUAL(100, memory_stats.buffer_occupation_ratio);
        TEST_ASSERT_EQUAL(2, msg_tasks_stack_id);
        TEST_ASSERT_EQUAL(8, memory_stats.msg_drop_number);
        for (int i = 0; i < MAX_MSG_NB - 2; i++)
        {
            TEST_ASSERT_EQUAL(0, msg_tasks[i]);
        }
    }

    // Test Case 5 :  Drop all messages from tx_tasks
    printf("--- Test Case 5 : Drop all messages from tx_tasks\n");
    MsgAlloc_Init(NULL);
    {
        void *memory_start;
        void *memory_end;
        memory_stats_t memory_stats = {.rx_msg_stack_ratio      = 0,
                                       .luos_stack_ratio        = 0,
                                       .tx_msg_stack_ratio      = 0,
                                       .buffer_occupation_ratio = 0,
                                       .msg_drop_number         = 0};

        memset(&memory_stats, 0, sizeof(memory_stats));
        MsgAlloc_Init(&memory_stats);

        for (int i = 0; i < MAX_MSG_NB - 2; i++)
        {
            tx_tasks[i].data_pt = (uint8_t *)&msg_buffer[i + 2];
            tx_tasks[i].size    = (uint16_t)i;
        }
        used_msg          = (msg_t *)&msg_buffer[0];
        oldest_msg        = (msg_t *)&msg_buffer[2];
        memory_start      = (void *)&msg_buffer[1];
        memory_end        = (void *)&msg_buffer[MAX_MSG_NB - 1];
        tx_tasks_stack_id = MAX_MSG_NB;

        TEST_ASSERT_EQUAL(SUCCEED, MsgAlloc_ClearMsgSpace(memory_start, memory_end));
        TEST_ASSERT_EQUAL(100, memory_stats.buffer_occupation_ratio);
        TEST_ASSERT_EQUAL(2, tx_tasks_stack_id);
        TEST_ASSERT_EQUAL(8, memory_stats.msg_drop_number);
        for (int i = 0; i < MAX_MSG_NB - 2; i++)
        {
            TEST_ASSERT_EQUAL(0, tx_tasks[i].data_pt);
            TEST_ASSERT_EQUAL(0, tx_tasks[i].size);
        }
    }
}

void unittest_ClearMsgTask(void)
{
    // Test Case 1 : Clear Message Task
    printf("\n--- Test Case 1 : Clear Message Task\n");
    MsgAlloc_Init(NULL);
    {
        msg_t *expected_msg_tasks[MAX_MSG_NB];
        msg_tasks_stack_id = MAX_MSG_NB;

        // To avoid assert
        msg_tasks[0]         = (msg_t *)&msg_buffer[0];
        luos_tasks[0].msg_pt = (msg_t *)&msg_buffer[0];
        tx_tasks[0].data_pt  = (uint8_t *)&msg_buffer[0];

        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            msg_tasks[i]          = (msg_t *)(&msg_buffer[0] + i);
            expected_msg_tasks[i] = (msg_t *)(&msg_buffer[0] + i + 1);
        }

        // Last Msg Task must be cleared
        expected_msg_tasks[MAX_MSG_NB - 1] = 0;
        MsgAlloc_ClearMsgTask();

        for (int i = 0; i < MAX_MSG_NB; i++)
        {
            TEST_ASSERT_FALSE(is_assert());
            TEST_ASSERT_EQUAL(expected_msg_tasks[i], msg_tasks[i]);
        }
        TEST_ASSERT_EQUAL(MAX_MSG_NB - 1, msg_tasks_stack_id);
    }
}

void unittest_ClearLuosTask(void)
{
    // Test Case 1 :  Verify assertion cases
    printf("\n--- Test Case 1 : Verify assertion cases\n");
    MsgAlloc_Init(NULL);
    {
        uint16_t luos_task_id;

        for (int i = 0; i <= MAX_MSG_NB + 2; i++)
        {
            MsgAlloc_Init(NULL);
            luos_tasks_stack_id = i;
            for (int j = 0; j <= MAX_MSG_NB + 2; j++)
            {
                luos_task_id = j;
                reset_assert();
                if ((luos_task_id >= luos_tasks_stack_id) || (luos_tasks_stack_id > MAX_MSG_NB))
                {
                    MsgAlloc_ClearLuosTask(luos_task_id);
                    TEST_ASSERT_TRUE(is_assert());
                }
                else
                {
                    MsgAlloc_ClearLuosTask(luos_task_id);
                    TEST_ASSERT_FALSE(is_assert());
                }
            }
        }
    }

    // Test Case 2 :  Clear Luos Tasks
    printf("--- Test Case 2 : Clear Luos Tasks\n");

    MsgAlloc_Init(NULL);
    {
        luos_task_t expected_luos_tasks[MAX_MSG_NB];

        assert_activation(0);
        for (int task_id = 0; task_id < MAX_MSG_NB; task_id++)
        {
            for (int tasks_stack_id = task_id + 1; tasks_stack_id <= MAX_MSG_NB; tasks_stack_id++)
            {
                // Initialisation
                MsgAlloc_Init(NULL);
                luos_tasks_stack_id = tasks_stack_id;

                for (int pt_value = 0; pt_value < MAX_MSG_NB; pt_value++)
                {
                    // Init luos_tasks pointers
                    luos_tasks[pt_value].msg_pt          = (msg_t *)(&msg_buffer[0] + pt_value);
                    luos_tasks[pt_value].ll_container_pt = (ll_container_t *)(&msg_buffer[0] + pt_value);

                    // Init expected pointers
                    if (pt_value == (tasks_stack_id - 1))
                    {
                        expected_luos_tasks[pt_value].msg_pt          = 0;
                        expected_luos_tasks[pt_value].ll_container_pt = 0;
                    }
                    else if (pt_value < task_id)
                    {
                        expected_luos_tasks[pt_value].msg_pt          = (msg_t *)(&msg_buffer[0] + pt_value);
                        expected_luos_tasks[pt_value].ll_container_pt = (ll_container_t *)(&msg_buffer[0] + pt_value);
                    }
                    else
                    {
                        expected_luos_tasks[pt_value].msg_pt          = (msg_t *)(&msg_buffer[0] + pt_value + 1);
                        expected_luos_tasks[pt_value].ll_container_pt = (ll_container_t *)(&msg_buffer[0] + pt_value + 1);
                    }
                }

                // Launch test
                reset_assert();
                MsgAlloc_ClearLuosTask(task_id);

                // Analyze test results
                for (int i = 0; i < tasks_stack_id; i++)
                {
                    TEST_ASSERT_EQUAL(expected_luos_tasks[i].msg_pt, luos_tasks[i].msg_pt);
                    TEST_ASSERT_EQUAL(expected_luos_tasks[i].ll_container_pt, luos_tasks[i].ll_container_pt);
                }
            }
        }
        assert_activation(1);
    }
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(unittest_DoWeHaveSpace);
    RUN_TEST(unittest_CheckMsgSpace);
    RUN_TEST(unittest_BufferAvailableSpaceComputation);
    RUN_TEST(unittest_OldestMsgCandidate);
    RUN_TEST(unittest_ClearMsgTask);
    RUN_TEST(unittest_ClearLuosTask);
    RUN_TEST(unittest_ClearMsgSpace);
    RUN_TEST(unittest_ValidDataIntegrity);
    //MsgAlloc_FindNewOldestMsg => this function doesn't need unit test

    UNITY_END();
}

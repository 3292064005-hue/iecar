#include <string.h>
#include <unity.h>
#include "host_platform.h"
#include "protocol_core_shared.h"

#define CAR_STRATEGY_PLATFORM_HEADER "host_platform.h"
#define CAR_STRATEGY_K210_HEADER "host_k210.h"
#include "task_strategy_common_shared.h"

long long int system_time = 1000;
u8 find_val1 = 0;
u8 find_val2 = 0;
u8 find_val3 = 0;
u8 find_val4 = 0;
int val1_cnt = 0;
int val2_cnt = 0;
int val3_cnt = 0;
int val4_cnt = 0;
static k210_observation_t left_obs;
static k210_observation_t right_obs;

void setUp(void)
{
}

void tearDown(void)
{
}

u8 K210_GetObservation(u8 camera_id, k210_observation_t *out)
{
    if(out == 0)
    {
        return 0u;
    }
    if(camera_id == K210_CAMERA_LEFT)
    {
        *out = left_obs;
        return 1u;
    }
    if(camera_id == K210_CAMERA_RIGHT)
    {
        *out = right_obs;
        return 1u;
    }
    return 0u;
}

void test_k210_openmv_real_parsers(void)
{
    u8 kbuf[HOST_K210_FRAME_SIZE + 1u] = {0};
    u8 obuf[HOST_OPENMV_FRAME_SIZE + 1u] = {0};
    kbuf[0] = HOST_K210_FRAME_SIZE;
    kbuf[1] = HOST_MAGIC0;
    kbuf[2] = HOST_MAGIC1;
    kbuf[3] = HOST_PROTOCOL_VERSION_BYTE;
    kbuf[4] = 0x10u;
    kbuf[5] = 80u;
    kbuf[7] = 12u;
    kbuf[9] = 34u;
    kbuf[10] = 7u;
    kbuf[11] = CarProtocol_Checksum8(&kbuf[1], 10u);
    kbuf[12] = HOST_TAIL;
    car_protocol_k210_frame_t k = CarProtocol_ParseK210RxBuffer(kbuf);
    TEST_ASSERT_EQUAL_UINT8(1u, k.valid);
    TEST_ASSERT_EQUAL_UINT8(0x10u, k.mask);
    TEST_ASSERT_EQUAL_UINT16(12u, k.x);
    TEST_ASSERT_EQUAL_UINT16(34u, k.y);

    obuf[0] = HOST_OPENMV_FRAME_SIZE;
    obuf[1] = HOST_MAGIC0;
    obuf[2] = HOST_MAGIC1;
    obuf[3] = HOST_PROTOCOL_VERSION_BYTE;
    obuf[5] = 160u;
    obuf[7] = 121u;
    obuf[9] = 3u;
    obuf[10] = 4u;
    obuf[11] = CarProtocol_Checksum8(&obuf[1], 10u);
    obuf[12] = HOST_TAIL;
    car_protocol_openmv_frame_t o = CarProtocol_ParseOpenMVRxBuffer(obuf);
    TEST_ASSERT_EQUAL_UINT8(1u, o.valid);
    TEST_ASSERT_EQUAL_INT16(160, o.center_x);
    TEST_ASSERT_EQUAL_INT16(121, o.east_y);
    TEST_ASSERT_EQUAL_INT16(3, o.west_y);
    TEST_ASSERT_EQUAL_UINT8(4u, o.black_num);
}

void test_stream_parser_accepts_contiguous_frames(void)
{
    car_protocol_stream_parser_t parser;
    u8 out[HOST_OPENMV_FRAME_SIZE] = {0};
    u8 frame1[HOST_OPENMV_FRAME_SIZE] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_PROTOCOL_VERSION_BYTE, 0u, 160u,
        0u, 121u, 0u, 3u, 4u, 0u, HOST_TAIL
    };
    u8 frame2[HOST_OPENMV_FRAME_SIZE] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_PROTOCOL_VERSION_BYTE, 0u, 162u,
        0u, 123u, 0u, 5u, 6u, 0u, HOST_TAIL
    };
    frame1[10] = CarProtocol_Checksum8(frame1, 10u);
    frame2[10] = CarProtocol_Checksum8(frame2, 10u);
    CarProtocol_StreamInit(&parser);
    for(u8 i = 0u; i < HOST_OPENMV_FRAME_SIZE - 1u; ++i)
    {
        TEST_ASSERT_EQUAL_UINT8(0u, CarProtocol_StreamFeedByte(&parser, frame1[i], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out));
    }
    TEST_ASSERT_EQUAL_UINT8(1u, CarProtocol_StreamFeedByte(&parser, frame1[HOST_OPENMV_FRAME_SIZE - 1u], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out));
    TEST_ASSERT_EQUAL_MEMORY(frame1, out, HOST_OPENMV_FRAME_SIZE);
    for(u8 i = 0u; i < HOST_OPENMV_FRAME_SIZE - 1u; ++i)
    {
        TEST_ASSERT_EQUAL_UINT8(0u, CarProtocol_StreamFeedByte(&parser, frame2[i], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out));
    }
    TEST_ASSERT_EQUAL_UINT8(1u, CarProtocol_StreamFeedByte(&parser, frame2[HOST_OPENMV_FRAME_SIZE - 1u], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out));
    TEST_ASSERT_EQUAL_MEMORY(frame2, out, HOST_OPENMV_FRAME_SIZE);
}


void test_carlink_stream_parser_legacy_and_checksum_resync(void)
{
    car_protocol_stream_parser_t parser;
    u8 out[HOST_CARLINK_FRAME_SIZE] = {0};
    u8 legacy[HOST_CARLINK_FRAME_SIZE] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_CARLINK_MSG_LEAVE, 0u, 0u, 0u,
        0u, 0u, 0u, 0u, HOST_TAIL
    };
    u8 good[HOST_CARLINK_FRAME_SIZE] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_CARLINK_MSG_SET_DETECT, 5u, 9u, HOST_TAIL,
        HOST_PROTOCOL_VERSION_MAJOR, HOST_PROTOCOL_VERSION_MINOR, 0u, 0u, HOST_TAIL
    };
    u8 bad_then_good[16u] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_CARLINK_MSG_SET_DETECT, 1u, 1u,
        HOST_MAGIC0, HOST_MAGIC1, HOST_CARLINK_MSG_SET_DETECT, 5u, 9u, HOST_TAIL,
        HOST_PROTOCOL_VERSION_MAJOR, HOST_PROTOCOL_VERSION_MINOR, 0u, 0u, HOST_TAIL
    };
    good[8] = CarProtocol_Checksum8(good, 8u);
    bad_then_good[13] = good[8];

    CarProtocol_StreamInit(&parser);
    for(u8 i = 0u; i < HOST_CARLINK_FRAME_SIZE - 1u; ++i)
    {
        TEST_ASSERT_EQUAL_UINT8(0u, CarProtocol_StreamFeedCarLinkByte(&parser, legacy[i], out));
    }
    TEST_ASSERT_EQUAL_UINT8(1u, CarProtocol_StreamFeedCarLinkByte(&parser, legacy[HOST_CARLINK_FRAME_SIZE - 1u], out));
    TEST_ASSERT_EQUAL_MEMORY(legacy, out, HOST_CARLINK_FRAME_SIZE);

    {
        u8 collision_legacy[HOST_CARLINK_FRAME_SIZE] = {
            HOST_MAGIC0, HOST_MAGIC1, HOST_CARLINK_MSG_GO, 84u, 0u, 0u,
            0u, 0u, 0u, 0u, HOST_TAIL
        };
        TEST_ASSERT_EQUAL_UINT8(0u, CarProtocol_Checksum8(collision_legacy, 8u));
        TEST_ASSERT_EQUAL_UINT8(1u, CarProtocol_CarLinkFrameIsValid(collision_legacy, HOST_CARLINK_FRAME_SIZE));
        CarProtocol_StreamInit(&parser);
        for(u8 i = 0u; i < HOST_CARLINK_FRAME_SIZE - 1u; ++i)
        {
            TEST_ASSERT_EQUAL_UINT8(0u, CarProtocol_StreamFeedCarLinkByte(&parser, collision_legacy[i], out));
        }
        TEST_ASSERT_EQUAL_UINT8(1u, CarProtocol_StreamFeedCarLinkByte(&parser, collision_legacy[HOST_CARLINK_FRAME_SIZE - 1u], out));
        TEST_ASSERT_EQUAL_MEMORY(collision_legacy, out, HOST_CARLINK_FRAME_SIZE);
    }

    CarProtocol_StreamInit(&parser);
    for(u8 i = 0u; i < 15u; ++i)
    {
        TEST_ASSERT_EQUAL_UINT8(0u, CarProtocol_StreamFeedCarLinkByte(&parser, bad_then_good[i], out));
    }
    TEST_ASSERT_EQUAL_UINT8(1u, CarProtocol_StreamFeedCarLinkByte(&parser, bad_then_good[15], out));
    TEST_ASSERT_EQUAL_MEMORY(good, out, HOST_CARLINK_FRAME_SIZE);
    TEST_ASSERT_TRUE(parser.resync_count >= 1u);
}

void test_carlink_compat_normalized_sender_role(void)
{
    u8 frame[HOST_CARLINK_FRAME_SIZE] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_CARLINK_MSG_SET_DETECT, 5u, 9u, 2u,
        HOST_PROTOCOL_VERSION_MAJOR, HOST_PROTOCOL_VERSION_MINOR, 0u, 0u, HOST_TAIL
    };
    frame[8] = CarProtocol_Checksum8(frame, 8u);
    TEST_ASSERT_EQUAL_UINT8(1u, CarProtocol_CarLinkFrameIsValid(frame, HOST_CARLINK_FRAME_SIZE));
    TEST_ASSERT_EQUAL_UINT8(1u, CarProtocol_NormalizeCarLinkSenderRole(frame, 0u, 2u, 2u));
}

void test_strategy_invalid_and_valid_detect(void)
{
    strategy_vote_plan_t plan;
    memset(&left_obs, 0, sizeof(left_obs));
    memset(&right_obs, 0, sizeof(right_obs));
    left_obs.valid_mask = 0x01u;
    left_obs.best_target = 1u;
    left_obs.reliable = 1u;
    right_obs.valid_mask = 0x02u;
    right_obs.best_target = 2u;
    right_obs.reliable = 1u;
    find_val1 = 1u;
    find_val2 = 0u;
    find_val3 = 2u;
    find_val4 = 0u;
    val1_cnt = 3;
    val2_cnt = 0;
    val3_cnt = 3;
    val4_cnt = 0;

    plan = Strategy_BuildVotePlan(0u);
    TEST_ASSERT_EQUAL_UINT8(0u, plan.matched);
    TEST_ASSERT_EQUAL_UINT8(0u, Strategy_ObservationSupportsTurn(K210_CAMERA_LEFT, 0u));

    plan = Strategy_BuildVotePlan(1u);
    TEST_ASSERT_EQUAL_UINT8(1u, plan.matched);
    TEST_ASSERT_EQUAL_UINT8(STRATEGY_TURN_LEFT, plan.turn_dir);
    TEST_ASSERT_EQUAL_UINT8(2u, plan.peer_target);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_k210_openmv_real_parsers);
        RUN_TEST(test_stream_parser_accepts_contiguous_frames);
RUN_TEST(test_carlink_stream_parser_legacy_and_checksum_resync);
    RUN_TEST(test_carlink_compat_normalized_sender_role);
    RUN_TEST(test_strategy_invalid_and_valid_detect);
    return UNITY_END();
}

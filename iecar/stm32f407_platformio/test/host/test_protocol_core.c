#include <assert.h>
#include <string.h>
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

static u8 checksum8(const u8 *data, u8 count)
{
    return CarProtocol_Checksum8(data, count);
}

static void test_k210_v2_rx_parser(void)
{
    u8 buf[HOST_K210_FRAME_SIZE + 1u] = {0};
    buf[0] = HOST_K210_FRAME_SIZE;
    buf[1] = HOST_MAGIC0;
    buf[2] = HOST_MAGIC1;
    buf[3] = HOST_PROTOCOL_VERSION_BYTE;
    buf[4] = 0x10u;
    buf[5] = 80u;
    buf[6] = 0u;
    buf[7] = 12u;
    buf[8] = 0u;
    buf[9] = 34u;
    buf[10] = 7u;
    buf[11] = checksum8(&buf[1], 10u);
    buf[12] = HOST_TAIL;

    car_protocol_k210_frame_t parsed = CarProtocol_ParseK210RxBuffer(buf);
    assert(parsed.valid == 1u);
    assert(parsed.is_v2 == 1u);
    assert(parsed.mask == 0x10u);
    assert(parsed.confidence == 80u);
    assert(parsed.x == 12u);
    assert(parsed.y == 34u);
    assert(parsed.seq == 7u);
}

static void test_openmv_v2_rx_parser(void)
{
    u8 buf[HOST_OPENMV_FRAME_SIZE + 1u] = {0};
    buf[0] = HOST_OPENMV_FRAME_SIZE;
    buf[1] = HOST_MAGIC0;
    buf[2] = HOST_MAGIC1;
    buf[3] = HOST_PROTOCOL_VERSION_BYTE;
    buf[4] = 0u;
    buf[5] = 160u;
    buf[6] = 0u;
    buf[7] = 121u;
    buf[8] = 0u;
    buf[9] = 3u;
    buf[10] = 4u;
    buf[11] = checksum8(&buf[1], 10u);
    buf[12] = HOST_TAIL;

    car_protocol_openmv_frame_t parsed = CarProtocol_ParseOpenMVRxBuffer(buf);
    assert(parsed.valid == 1u);
    assert(parsed.is_v2 == 1u);
    assert(parsed.center_x == 160);
    assert(parsed.east_y == 121);
    assert(parsed.west_y == 3);
    assert(parsed.black_num == 4u);
}


static void test_stream_parser_resync_and_sticky_halves(void)
{
    car_protocol_stream_parser_t parser;
    u8 out[HOST_OPENMV_FRAME_SIZE] = {0};
    u8 frame[HOST_OPENMV_FRAME_SIZE] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_PROTOCOL_VERSION_BYTE, 0u, 160u,
        0u, 121u, 0u, 3u, 4u, 0u, HOST_TAIL
    };
    u8 i;
    frame[10] = checksum8(frame, 10u);
    CarProtocol_StreamInit(&parser);
    assert(CarProtocol_StreamFeedByte(&parser, 0x55u, HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 0u);
    for(i = 0u; i < 5u; ++i)
    {
        assert(CarProtocol_StreamFeedByte(&parser, frame[i], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 0u);
    }
    for(; i < HOST_OPENMV_FRAME_SIZE - 1u; ++i)
    {
        assert(CarProtocol_StreamFeedByte(&parser, frame[i], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 0u);
    }
    assert(CarProtocol_StreamFeedByte(&parser, frame[HOST_OPENMV_FRAME_SIZE - 1u], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 1u);
    assert(memcmp(out, frame, HOST_OPENMV_FRAME_SIZE) == 0);
    assert(parser.resync_count == 0u);
}


static void test_stream_parser_sliding_resync_inside_bad_frame(void)
{
    car_protocol_stream_parser_t parser;
    u8 out[HOST_OPENMV_FRAME_SIZE] = {0};
    u8 good[HOST_OPENMV_FRAME_SIZE] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_PROTOCOL_VERSION_BYTE, 0u, 161u,
        0u, 122u, 0u, 4u, 5u, 0u, HOST_TAIL
    };
    u8 bad_then_good[18u] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_PROTOCOL_VERSION_BYTE, 0u, 1u,
        2u, HOST_MAGIC0, HOST_MAGIC1, HOST_PROTOCOL_VERSION_BYTE, 0u, 161u,
        0u, 122u, 0u, 4u, 5u, 0u, HOST_TAIL
    };
    u8 i;
    good[10] = checksum8(good, 10u);
    bad_then_good[16] = good[10];
    CarProtocol_StreamInit(&parser);
    for(i = 0u; i < 17u; ++i)
    {
        assert(CarProtocol_StreamFeedByte(&parser, bad_then_good[i], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 0u);
    }
    assert(CarProtocol_StreamFeedByte(&parser, bad_then_good[17], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 1u);
    assert(memcmp(out, good, HOST_OPENMV_FRAME_SIZE) == 0);
    assert(parser.bad_tail_count == 1u || parser.bad_checksum_count == 1u);
    assert(parser.resync_count >= 1u);
}

static void test_stream_parser_accepts_contiguous_frames(void)
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
    u8 i;
    frame1[10] = checksum8(frame1, 10u);
    frame2[10] = checksum8(frame2, 10u);
    CarProtocol_StreamInit(&parser);
    for(i = 0u; i < HOST_OPENMV_FRAME_SIZE - 1u; ++i)
    {
        assert(CarProtocol_StreamFeedByte(&parser, frame1[i], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 0u);
    }
    assert(CarProtocol_StreamFeedByte(&parser, frame1[HOST_OPENMV_FRAME_SIZE - 1u], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 1u);
    assert(memcmp(out, frame1, HOST_OPENMV_FRAME_SIZE) == 0);
    for(i = 0u; i < HOST_OPENMV_FRAME_SIZE - 1u; ++i)
    {
        assert(CarProtocol_StreamFeedByte(&parser, frame2[i], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 0u);
    }
    assert(CarProtocol_StreamFeedByte(&parser, frame2[HOST_OPENMV_FRAME_SIZE - 1u], HOST_MAGIC0, HOST_MAGIC1, HOST_OPENMV_FRAME_SIZE, HOST_TAIL, 0u, 10u, 10u, out) == 1u);
    assert(memcmp(out, frame2, HOST_OPENMV_FRAME_SIZE) == 0);
}


static void test_carlink_stream_parser_legacy_and_checksum_resync(void)
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
    u8 i;
    good[8] = checksum8(good, 8u);
    bad_then_good[13] = good[8];

    CarProtocol_StreamInit(&parser);
    for(i = 0u; i < HOST_CARLINK_FRAME_SIZE - 1u; ++i)
    {
        assert(CarProtocol_StreamFeedCarLinkByte(&parser, legacy[i], out) == 0u);
    }
    assert(CarProtocol_StreamFeedCarLinkByte(&parser, legacy[HOST_CARLINK_FRAME_SIZE - 1u], out) == 1u);
    assert(memcmp(out, legacy, HOST_CARLINK_FRAME_SIZE) == 0);

    {
        u8 collision_legacy[HOST_CARLINK_FRAME_SIZE] = {
            HOST_MAGIC0, HOST_MAGIC1, HOST_CARLINK_MSG_GO, 84u, 0u, 0u,
            0u, 0u, 0u, 0u, HOST_TAIL
        };
        assert(checksum8(collision_legacy, 8u) == 0u);
        assert(CarProtocol_CarLinkFrameIsValid(collision_legacy, HOST_CARLINK_FRAME_SIZE) == 1u);
        CarProtocol_StreamInit(&parser);
        for(i = 0u; i < HOST_CARLINK_FRAME_SIZE - 1u; ++i)
        {
            assert(CarProtocol_StreamFeedCarLinkByte(&parser, collision_legacy[i], out) == 0u);
        }
        assert(CarProtocol_StreamFeedCarLinkByte(&parser, collision_legacy[HOST_CARLINK_FRAME_SIZE - 1u], out) == 1u);
        assert(memcmp(out, collision_legacy, HOST_CARLINK_FRAME_SIZE) == 0);
    }

    CarProtocol_StreamInit(&parser);
    for(i = 0u; i < 15u; ++i)
    {
        assert(CarProtocol_StreamFeedCarLinkByte(&parser, bad_then_good[i], out) == 0u);
    }
    assert(CarProtocol_StreamFeedCarLinkByte(&parser, bad_then_good[15], out) == 1u);
    assert(memcmp(out, good, HOST_CARLINK_FRAME_SIZE) == 0);
    assert(parser.bad_checksum_count == 1u || parser.bad_tail_count == 1u);
    assert(parser.resync_count >= 1u);
}

static void test_carlink_compat_sender_role_normalization(void)
{
    u8 frame[HOST_CARLINK_FRAME_SIZE] = {
        HOST_MAGIC0, HOST_MAGIC1, HOST_CARLINK_MSG_SET_DETECT, 5u, 9u, 2u,
        HOST_PROTOCOL_VERSION_MAJOR, HOST_PROTOCOL_VERSION_MINOR, 0u, 0u, HOST_TAIL
    };
    frame[8] = checksum8(frame, 8u);
    assert(CarProtocol_CarLinkFrameIsValid(frame, HOST_CARLINK_FRAME_SIZE) == 1u);
    assert(CarProtocol_NormalizeCarLinkSenderRole(frame, 0u, 2u, 2u) == 1u);
}

static void test_strategy_invalid_detect_is_closed(void)
{
    strategy_vote_plan_t plan;
    left_obs.valid_mask = 0xFFu;
    left_obs.best_target = 1u;
    left_obs.reliable = 1u;
    find_val1 = 1u;
    val1_cnt = 3;
    plan = Strategy_BuildVotePlan(0u);
    assert(plan.matched == 0u);
    assert(Strategy_ObservationSupportsTurn(K210_CAMERA_LEFT, 0u) == 0u);
}

static void test_strategy_valid_detect(void)
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
    plan = Strategy_BuildVotePlan(1u);
    assert(plan.matched == 1u);
    assert(plan.turn_dir == STRATEGY_TURN_LEFT);
    assert(plan.peer_target == 2u);
}

int main(void)
{
    test_k210_v2_rx_parser();
    test_openmv_v2_rx_parser();
    test_stream_parser_resync_and_sticky_halves();
    test_stream_parser_sliding_resync_inside_bad_frame();
    test_stream_parser_accepts_contiguous_frames();
    test_carlink_stream_parser_legacy_and_checksum_resync();
    test_carlink_compat_sender_role_normalization();
    test_strategy_invalid_detect_is_closed();
    test_strategy_valid_detect();
    return 0;
}

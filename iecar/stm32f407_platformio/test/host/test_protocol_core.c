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
    test_carlink_compat_sender_role_normalization();
    test_strategy_invalid_detect_is_closed();
    test_strategy_valid_detect();
    return 0;
}

#ifndef TASK_STRATEGY_COMMON_SHARED_H__
#define TASK_STRATEGY_COMMON_SHARED_H__

#ifndef CAR_STRATEGY_PLATFORM_HEADER
#error "CAR_STRATEGY_PLATFORM_HEADER must be defined before including task_strategy_common_shared.h"
#endif
#ifndef CAR_STRATEGY_K210_HEADER
#error "CAR_STRATEGY_K210_HEADER must be defined before including task_strategy_common_shared.h"
#endif

#include CAR_STRATEGY_PLATFORM_HEADER
#include CAR_STRATEGY_K210_HEADER

typedef enum
{
    STRATEGY_TURN_NONE = 0,
    STRATEGY_TURN_LEFT = 1,
    STRATEGY_TURN_RIGHT = 2
} strategy_turn_dir_t;

typedef struct
{
    u8 matched;
    u8 turn_dir;
    u8 peer_target;
    u8 confidence_gated;
    u8 peer_observation_ready;
} strategy_vote_plan_t;

static inline void Strategy_FilterVoteResults(void)
{
    if(val1_cnt < CAR_K210_DECISION_MIN_COUNT) find_val1 = 0;
    if(val2_cnt < CAR_K210_DECISION_MIN_COUNT) find_val2 = 0;
    if(val3_cnt < CAR_K210_DECISION_MIN_COUNT) find_val3 = 0;
    if(val4_cnt < CAR_K210_DECISION_MIN_COUNT) find_val4 = 0;
}

static inline u8 Strategy_FirstValidTarget(u8 first, u8 second)
{
    if(first >= 1 && first <= CAR_K210_CLASS_COUNT)
    {
        return first;
    }
    if(second >= 1 && second <= CAR_K210_CLASS_COUNT)
    {
        return second;
    }
    return 0;
}

static inline u8 Strategy_IsValidWardTarget(u8 target)
{
    return (u8)(target >= 1 && target <= CAR_K210_CLASS_COUNT);
}

static inline u8 Strategy_ObservationSupportsTurn(u8 camera_id, u8 detect)
{
    k210_observation_t obs;
    if(!Strategy_IsValidWardTarget(detect))
    {
        return 0u;
    }
    if(!K210_GetObservation(camera_id, &obs))
    {
        return 0u;
    }
    if(!obs.reliable)
    {
        return 0u;
    }
    if(obs.best_target == detect)
    {
        return 1u;
    }
    return (u8)((obs.valid_mask & (1u << (detect - 1u))) != 0u);
}

static inline u8 Strategy_SelectPeerTarget(u8 camera_id, u8 first, u8 second, u8 *peer_ready)
{
    k210_observation_t obs;
    if(peer_ready)
    {
        *peer_ready = 0u;
    }
    if(!K210_GetObservation(camera_id, &obs))
    {
        return 0u;
    }
    if(obs.reliable)
    {
        if(peer_ready)
        {
            *peer_ready = 1u;
        }
        if(Strategy_IsValidWardTarget(obs.best_target))
        {
            return obs.best_target;
        }
    }
    return Strategy_FirstValidTarget(first, second);
}

static inline strategy_vote_plan_t Strategy_BuildVotePlan(u8 detect)
{
    strategy_vote_plan_t plan;
    plan.matched = 0;
    plan.turn_dir = STRATEGY_TURN_NONE;
    plan.peer_target = 0;
    plan.confidence_gated = 0;
    plan.peer_observation_ready = 0;

    if(!Strategy_IsValidWardTarget(detect))
    {
        return plan;
    }

    Strategy_FilterVoteResults();
    if(find_val1 == detect || find_val2 == detect)
    {
        if(Strategy_ObservationSupportsTurn(K210_CAMERA_LEFT, detect))
        {
            plan.matched = 1;
            plan.turn_dir = STRATEGY_TURN_LEFT;
            plan.peer_target = Strategy_SelectPeerTarget(K210_CAMERA_RIGHT, find_val3, find_val4, &plan.peer_observation_ready);
        }
        else
        {
            plan.confidence_gated = 1;
        }
    }
    else if(find_val3 == detect || find_val4 == detect)
    {
        if(Strategy_ObservationSupportsTurn(K210_CAMERA_RIGHT, detect))
        {
            plan.matched = 1;
            plan.turn_dir = STRATEGY_TURN_RIGHT;
            plan.peer_target = Strategy_SelectPeerTarget(K210_CAMERA_LEFT, find_val1, find_val2, &plan.peer_observation_ready);
        }
        else
        {
            plan.confidence_gated = 1;
        }
    }
    return plan;
}

static inline u8 Strategy_ShouldTurnForDetect(u8 detect, u8 *turn_dir)
{
    strategy_vote_plan_t plan = Strategy_BuildVotePlan(detect);
    if(turn_dir)
    {
        *turn_dir = plan.turn_dir;
    }
    return plan.matched;
}

#endif /* TASK_STRATEGY_COMMON_SHARED_H__ */

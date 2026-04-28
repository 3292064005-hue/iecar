#include "task3.h"
#include "../PLATFORM/car_mainchain.h"
#include "../STRATEGY/common/task_strategy_common.h"
#include "../../../COMMON/STRATEGY/task_runtime_common_shared.h"
#include "../STRATEGY/common/task_link_runtime_common.h"

typedef enum
{
    TASK3_STATE_SCAN = 0,
    TASK3_STATE_MAIN_LOOP,
    TASK3_STATE_TURN_ACTION,
    TASK3_STATE_SAFE_STOP,
    TASK3_STATE_COMPLETE_STOP
} task3_state_t;

static const task_runtime_state_desc_t task3_state_desc[] =
{
    {(u8)TASK3_STATE_SCAN, "TASK3_STATE_SCAN", 0u, CAR_K210_SCAN_SAMPLE_MS},
    {(u8)TASK3_STATE_MAIN_LOOP, "TASK3_STATE_MAIN_LOOP", 1u, 0u},
    {(u8)TASK3_STATE_TURN_ACTION, "TASK3_STATE_TURN_ACTION", 1u, 0u},
    {(u8)TASK3_STATE_SAFE_STOP, "TASK3_STATE_SAFE_STOP", 0u, 0u},
    {(u8)TASK3_STATE_COMPLETE_STOP, "TASK3_STATE_COMPLETE_STOP", 0u, 0u},
};

typedef struct
{
    task3_state_t state;
    task3_state_t next_state;
    task_runtime_clock_t clock_state;
    u8 first;
    u8 turn_dir;
} task3_context_t;

static task3_context_t g_task3_ctx = {TASK3_STATE_SCAN, TASK3_STATE_SCAN, {0,0,0}, 0, 0};

static void Task3_EnterState(task3_state_t state)
{
    g_task3_ctx.state = state;
    TaskRuntime_RecordStateFromTable(3u, task3_state_desc, (u8)(sizeof(task3_state_desc) / sizeof(task3_state_desc[0])), (u8)state, 0u);
    TaskRuntime_MarkEntered(&g_task3_ctx.clock_state);
}

static void Task3_RequestSafeStop(const char *reason)
{
    TaskRuntime_EnterSafeStopForTask(3u, 0u, "T3", reason);
    Task3_EnterState(TASK3_STATE_SAFE_STOP);
}

static void Task3_RequestCompleteStop(const char *reason, u8 result_code)
{
    TaskRuntime_EnterCompleteStopForTask(3u, result_code, "T3C", reason);
    Task3_EnterState(TASK3_STATE_COMPLETE_STOP);
}

static u8 Task3_RecordTurn(u8 step)
{
    if(CarPathStack_Push(step))
    {
        return 1u;
    }
    Task3_RequestSafeStop("stack");
    return 0u;
}

static void Task3_StartTurn(u8 turn_dir, u8 record_path)
{
    g_task3_ctx.turn_dir = turn_dir;
    g_task3_ctx.next_state = TASK3_STATE_MAIN_LOOP;
    TaskRuntime_MarkAction(&g_task3_ctx.clock_state);
    board = 0;
    ttt = CAR_TASK_TURN_HOLD_TICKS;
    if(turn_dir == STRATEGY_TURN_LEFT)
    {
        if(record_path && !Task3_RecordTurn(1u))
        {
            return;
        }
        MOTOR_ANTI(0, CAR_TURN_SPEED);
        Task3_EnterState(TASK3_STATE_TURN_ACTION);
    }
    else if(turn_dir == STRATEGY_TURN_RIGHT)
    {
        if(record_path && !Task3_RecordTurn(3u))
        {
            return;
        }
        MOTOR_ANTI(CAR_TURN_SPEED, 0);
        Task3_EnterState(TASK3_STATE_TURN_ACTION);
    }
}

static void Task3_ShowVoteSummary(void)
{
    sprintf(sss,"val:%d-%d-%d-%d",find_val1,find_val2,find_val3,find_val4);
    OLED_ShowString(0,2,sss,16);
    sprintf(sss,"cnt:%d-%d-%d-%d",val1_cnt,val2_cnt,val3_cnt,val4_cnt);
    OLED_ShowString(0,4,sss,16);
}

static void Task3_HandleVoteTurn(void)
{
    strategy_vote_plan_t plan;
    count_go = 0;
    find_MAX(K210_CAMERA_LEFT);
    find_MAX(K210_CAMERA_RIGHT);
    Strategy_FilterVoteResults();
    Task3_ShowVoteSummary();
    plan = Strategy_BuildVotePlan((u8)detect_num);
    K210_ResetVoteBuffers();
    count_go = 1;
    board = 0;
    if(plan.confidence_gated)
    {
        Task3_RequestSafeStop("k210");
        return;
    }
    if(plan.matched)
    {
        Task3_StartTurn(plan.turn_dir, 1u);
    }
}

void TASK3_GO(void)
{
    Task3_EnterState(TASK3_STATE_SCAN);
    while(1)
    {
        TaskLink_ServiceKeepalive("T3");
        switch(g_task3_ctx.state)
        {
            case TASK3_STATE_SCAN:
                count_go = 1;
                if(TaskRuntime_IsElapsed(g_task3_ctx.clock_state.entered_ms, CAR_K210_SCAN_SAMPLE_MS))
                {
                    detect_num = find_max(K210_CAMERA_LEFT);
                    sprintf(sss,"N-%d",detect_num);
                    OLED_ShowString(0,0,sss,16);
                    sprintf(sss,"omv-%d-b-%d",openmvdata[0],black_num);
                    OLED_ShowString(0,5,sss,16);
                    if(k210_cnt_l[detect_num] >= CAR_K210_SCAN_LOCK_THRESHOLD && K210_ObservationIsReliable(K210_CAMERA_LEFT))
                    {
                        count_go = 0;
                        K210_ResetVoteBuffers();
                        flag_go = 1;
                        g_task3_ctx.first = 0;
                        OLED_ShowString(0,3,"LEGACY-T3",16);
                        Task3_EnterState(TASK3_STATE_MAIN_LOOP);
                    }
                    else
                    {
                        count_go = 0;
                        K210_ResetVoteBuffers();
                        Task3_EnterState(TASK3_STATE_SCAN);
                    }
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK3_STATE_MAIN_LOOP:
            {
                car_line_output_t line_output;
                if(flag_go != 1)
                {
                    Task3_RequestSafeStop("flag");
                    break;
                }
                if(return_sta == 1)
                {
                    if(board == 1 && time_up == 0)
                    {
                        u8 sta_sta;
                        if(!CarPathStack_Pop(&sta_sta))
                        {
                            Task3_RequestSafeStop("stack");
                            break;
                        }
                        if(sta_sta == 1u) Task3_StartTurn(STRATEGY_TURN_RIGHT, 0u);
                        else if(sta_sta == 3u) Task3_StartTurn(STRATEGY_TURN_LEFT, 0u);
                        else Task3_RequestSafeStop("path");
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task3_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        RGB_EN(GREEN);
                        CAR_STOP();
                        Task3_RequestCompleteStop("done", 1u);
                        break;
                    }
                    if(Car_LineEventIsVisionFault(line_output.event))
                    {
                        Task3_RequestSafeStop("omv");
                        break;
                    }
                    Car_ApplyLineOutput(&line_output);
                }
                else
                {
                    if(g_task3_ctx.first == 0)
                    {
                        count_go = 1;
                        g_task3_ctx.first = 1;
                    }
                    if(board == 1 && time_up == 0)
                    {
                        if(detect_num == 1)
                        {
                            Task3_StartTurn(STRATEGY_TURN_LEFT, 1u);
                            break;
                        }
                        if(detect_num == 2)
                        {
                            Task3_StartTurn(STRATEGY_TURN_RIGHT, 1u);
                            break;
                        }
                        Task3_HandleVoteTurn();
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task3_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        Task3_RequestSafeStop("legacy");
                        break;
                    }
                    if(Car_LineEventIsVisionFault(line_output.event))
                    {
                        Task3_RequestSafeStop("omv");
                        break;
                    }
                    Car_ApplyLineOutput(&line_output);
                }
                delay_ms(CAR_TASK_LOOP_DELAY_MS);
                break;
            }

            case TASK3_STATE_TURN_ACTION:
                if(g_task3_ctx.turn_dir == STRATEGY_TURN_LEFT)
                {
                    if(TaskRuntime_IsElapsed(g_task3_ctx.clock_state.action_started_ms, CAR_TURN_LEFT_DELAY_MS))
                    {
                        Task3_EnterState(g_task3_ctx.next_state);
                    }
                }
                else if(g_task3_ctx.turn_dir == STRATEGY_TURN_RIGHT)
                {
                    if(TaskRuntime_IsElapsed(g_task3_ctx.clock_state.action_started_ms, CAR_TURN_RIGHT_DELAY_MS))
                    {
                        Task3_EnterState(g_task3_ctx.next_state);
                    }
                }
                else
                {
                    Task3_EnterState(g_task3_ctx.next_state);
                }
                delay_ms(CAR_TASK_POLL_DELAY_MS);
                break;

            case TASK3_STATE_COMPLETE_STOP:
                CAR_STOP();
                RGB_EN(GREEN);
                delay_ms(CAR_SAFE_STOP_BLINK_MS);
                break;

            case TASK3_STATE_SAFE_STOP:
            default:
                CAR_STOP();
                RGB_EN(RED);
                delay_ms(CAR_SAFE_STOP_BLINK_MS);
                break;
        }
    }
}

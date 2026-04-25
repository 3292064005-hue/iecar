#include "task2.h"
#include "../PLATFORM/car_mainchain.h"
#include "../STRATEGY/common/task_strategy_common.h"
#include "../../../COMMON/STRATEGY/task_runtime_common_shared.h"
#include "../STRATEGY/common/task_link_runtime_common.h"

u8 t2dir=0;
u8 send_num=0;
int ttt=0;

typedef enum
{
    TASK2_STATE_SCAN = 0,
    TASK2_STATE_WAIT_START,
    TASK2_STATE_MAIN_LOOP,
    TASK2_STATE_WAIT_TX_RESULT,
    TASK2_STATE_WAIT_REMOTE_RELEASE_BUTTON,
    TASK2_STATE_TURN_ACTION,
    TASK2_STATE_UTURN_ACTION,
    TASK2_STATE_SAFE_STOP,
    TASK2_STATE_COMPLETE_STOP
} task2_state_t;

static const task_runtime_state_desc_t task2_state_desc[] =
{
    {(u8)TASK2_STATE_SCAN, "TASK2_STATE_SCAN"},
    {(u8)TASK2_STATE_WAIT_START, "TASK2_STATE_WAIT_START"},
    {(u8)TASK2_STATE_MAIN_LOOP, "TASK2_STATE_MAIN_LOOP"},
    {(u8)TASK2_STATE_WAIT_TX_RESULT, "TASK2_STATE_WAIT_TX_RESULT"},
    {(u8)TASK2_STATE_WAIT_REMOTE_RELEASE_BUTTON, "TASK2_STATE_WAIT_REMOTE_RELEASE_BUTTON"},
    {(u8)TASK2_STATE_TURN_ACTION, "TASK2_STATE_TURN_ACTION"},
    {(u8)TASK2_STATE_UTURN_ACTION, "TASK2_STATE_UTURN_ACTION"},
    {(u8)TASK2_STATE_SAFE_STOP, "TASK2_STATE_SAFE_STOP"},
    {(u8)TASK2_STATE_COMPLETE_STOP, "TASK2_STATE_COMPLETE_STOP"},
};

typedef struct
{
    task2_state_t state;
    task2_state_t next_state;
    task_runtime_clock_t clock_state;
    u8 first;
    u8 turn_dir;
    const char *tx_reason;
} task2_context_t;

static task2_context_t g_task2_ctx = {TASK2_STATE_SCAN, TASK2_STATE_SCAN, {0,0,0}, 0, 0, 0};

static void Task2_EnterState(task2_state_t state)
{
    g_task2_ctx.state = state;
    TaskRuntime_RecordStateFromTable(2u, task2_state_desc, (u8)(sizeof(task2_state_desc) / sizeof(task2_state_desc[0])), (u8)state, 0u);
    TaskRuntime_MarkEntered(&g_task2_ctx.clock_state);
}

static void Task2_ClearK210Votes(void)
{
    K210_ResetVoteBuffers();
}

static void Task2_RequestSafeStop(const char *reason)
{
    TaskRuntime_EnterSafeStop("T2", reason);
    Task2_EnterState(TASK2_STATE_SAFE_STOP);
}

static u8 Task2_RecordTurn(u8 step)
{
    if(CarPathStack_Push(step))
    {
        return 1u;
    }
    Task2_RequestSafeStop("stack");
    return 0u;
}

static u8 Task2_SelectPeerTargetForTurn(u8 turn_dir, u8 *peer_target)
{
    u8 peer_ready = 0u;
    find_MAX(K210_CAMERA_LEFT);
    find_MAX(K210_CAMERA_RIGHT);
    Strategy_FilterVoteResults();
    if(turn_dir == STRATEGY_TURN_LEFT)
    {
        *peer_target = Strategy_SelectPeerTarget(K210_CAMERA_RIGHT, find_val3, find_val4, &peer_ready);
    }
    else if(turn_dir == STRATEGY_TURN_RIGHT)
    {
        *peer_target = Strategy_SelectPeerTarget(K210_CAMERA_LEFT, find_val1, find_val2, &peer_ready);
    }
    else
    {
        *peer_target = 0u;
    }
    if(peer_ready == 0u && !Strategy_IsValidWardTarget(*peer_target))
    {
        return 0u;
    }
    return Strategy_IsValidWardTarget(*peer_target);
}

static void Task2_StartTurn(u8 turn_dir, u8 record_path)
{
    g_task2_ctx.turn_dir = turn_dir;
    g_task2_ctx.next_state = TASK2_STATE_MAIN_LOOP;
    TaskRuntime_MarkAction(&g_task2_ctx.clock_state);
    board = 0;
    ttt = CAR_TASK_TURN_HOLD_TICKS;
    if(turn_dir == STRATEGY_TURN_LEFT)
    {
        if(record_path && !Task2_RecordTurn(1u))
        {
            return;
        }
        t2dir = 1u;
        MOTOR_ANTI(0, CAR_TURN_SPEED);
        Task2_EnterState(TASK2_STATE_TURN_ACTION);
    }
    else if(turn_dir == STRATEGY_TURN_RIGHT)
    {
        if(record_path && !Task2_RecordTurn(3u))
        {
            return;
        }
        t2dir = 2u;
        MOTOR_ANTI(CAR_TURN_SPEED, 0);
        Task2_EnterState(TASK2_STATE_TURN_ACTION);
    }
}

static void Task2_StartReturnTurn(u8 step)
{
    if(step == 1u)
    {
        Task2_StartTurn(STRATEGY_TURN_RIGHT, 0u);
    }
    else if(step == 3u)
    {
        Task2_StartTurn(STRATEGY_TURN_LEFT, 0u);
    }
    else
    {
        Task2_RequestSafeStop("path");
    }
}

static void Task2_ShowVoteSummary(void)
{
    sprintf(sss,"val:%d-%d-%d-%d",find_val1,find_val2,find_val3,find_val4);
    OLED_ShowString(0,2,sss,16);
    sprintf(sss,"cnt:%d-%d-%d-%d",val1_cnt,val2_cnt,val3_cnt,val4_cnt);
    OLED_ShowString(0,4,sss,16);
}

static void Task2_BeginTx(u8 msg_type, u8 value, const char *reason, task2_state_t next_state)
{
    if(msg_type == CAR_LINK_MSG_SET_DETECT && !Strategy_IsValidWardTarget(value))
    {
        Task2_RequestSafeStop("peer0");
        return;
    }
    if(!CarLink_SendMessageAsync(msg_type, value))
    {
        Task2_RequestSafeStop("txbusy");
        return;
    }
    g_task2_ctx.tx_reason = reason;
    g_task2_ctx.next_state = next_state;
    Task2_EnterState(TASK2_STATE_WAIT_TX_RESULT);
}

static void Task2_ServiceTxState(void)
{
    CarLink_ServiceTx();
    if(CarLink_GetTxState() == CAR_LINK_TX_SUCCESS)
    {
        CarLink_ClearTxState();
        if(g_task2_ctx.next_state == TASK2_STATE_UTURN_ACTION)
        {
            g_task2_ctx.clock_state.action_started_ms = 0;
        }
        Task2_EnterState(g_task2_ctx.next_state);
        return;
    }
    if(CarLink_GetTxState() == CAR_LINK_TX_FAILED)
    {
        CarLink_ClearTxState();
        Task2_RequestSafeStop(g_task2_ctx.tx_reason ? g_task2_ctx.tx_reason : "tx");
        return;
    }
    TaskLink_ServiceKeepalive("T2");
    delay_ms(CAR_TASK_POLL_DELAY_MS);
}

static void Task2_HandleVoteTurn(void)
{
    strategy_vote_plan_t plan;
    count_go = 0;
    find_MAX(K210_CAMERA_LEFT);
    find_MAX(K210_CAMERA_RIGHT);
    Strategy_FilterVoteResults();
    Task2_ShowVoteSummary();
    plan = Strategy_BuildVotePlan((u8)detect_num);
    Task2_ClearK210Votes();
    count_go = 1;
    board = 0;
    if(plan.confidence_gated)
    {
        Task2_RequestSafeStop("k210");
        return;
    }
    if(!plan.matched)
    {
        return;
    }
    send_num = plan.peer_target;
    if(!Strategy_IsValidWardTarget(send_num))
    {
        Task2_RequestSafeStop("peer0");
        return;
    }
    Task2_StartTurn(plan.turn_dir, 1u);
}

void TASK2_GO(void)
{
    Task2_EnterState(TASK2_STATE_SCAN);
    while(1)
    {
        TaskLink_ServiceKeepalive("T2");
        switch(g_task2_ctx.state)
        {
            case TASK2_STATE_SCAN:
                count_go = 1;
                if(TaskRuntime_IsElapsed(g_task2_ctx.clock_state.entered_ms, CAR_K210_SCAN_SAMPLE_MS))
                {
                    detect_num = find_max(K210_CAMERA_LEFT);
                    sprintf(sss,"N-%d",detect_num);
                    OLED_ShowString(0,0,sss,16);
                    sprintf(sss,"omv-%d-b-%d",openmvdata[0],black_num);
                    OLED_ShowString(0,5,sss,16);
                    if(k210_cnt_l[detect_num] >= CAR_K210_SCAN_LOCK_THRESHOLD && K210_ObservationIsReliable(K210_CAMERA_LEFT))
                    {
                        count_go = 0;
                        Task2_ClearK210Votes();
                        OLED_ShowString(0,3,"GOGOGO!",16);
                        Task2_EnterState(TASK2_STATE_WAIT_START);
                    }
                    else
                    {
                        count_go = 0;
                        Task2_ClearK210Votes();
                        Task2_EnterState(TASK2_STATE_SCAN);
                    }
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK2_STATE_WAIT_START:
                if(PAin(5)==0)
                {
                    delay_ms(CAR_START_BUTTON_DEBOUNCE_MS);
                    flag_go=1;
                    g_task2_ctx.first = 0;
                    Task2_EnterState(TASK2_STATE_MAIN_LOOP);
                }
                else
                {
                    delay_ms(CAR_RETURN_BUTTON_POLL_MS);
                }
                break;

            case TASK2_STATE_MAIN_LOOP:
            {
                car_line_output_t line_output;
                if(flag_go != 1)
                {
                    Task2_RequestSafeStop("flag");
                    break;
                }
                if(return_sta == 1)
                {
                    if(board==1 && time_up==0)
                    {
                        u8 sta_sta;
                        if(!CarPathStack_Pop(&sta_sta))
                        {
                            Task2_RequestSafeStop("stack");
                            break;
                        }
                        Task2_StartReturnTurn(sta_sta);
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task2_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        RGB_EN(GREEN);
                        CAR_STOP();
                        Task2_BeginTx(CAR_LINK_MSG_GO, 1u, "go", TASK2_STATE_COMPLETE_STOP);
                        break;
                    }
                    if(Car_LineEventIsVisionFault(line_output.event))
                    {
                        Task2_RequestSafeStop("omv");
                        break;
                    }
                    Car_ApplyLineOutput(&line_output);
                }
                else
                {
                    if(g_task2_ctx.first == 0)
                    {
                        count_go = 1;
                        g_task2_ctx.first = 1;
                    }
                    if(board==1 && time_up==0)
                    {
                        if(detect_num == 1)
                        {
                            if(!Task2_SelectPeerTargetForTurn(STRATEGY_TURN_LEFT, &send_num))
                            {
                                Task2_RequestSafeStop("peer0");
                                break;
                            }
                            Task2_StartTurn(STRATEGY_TURN_LEFT, 1u);
                            break;
                        }
                        if(detect_num == 2)
                        {
                            if(!Task2_SelectPeerTargetForTurn(STRATEGY_TURN_RIGHT, &send_num))
                            {
                                Task2_RequestSafeStop("peer0");
                                break;
                            }
                            Task2_StartTurn(STRATEGY_TURN_RIGHT, 1u);
                            break;
                        }
                        Task2_HandleVoteTurn();
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task2_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        delay_ms(CAR_STATION_STOP_DELAY_MS);
                        RGB_EN(RED);
                        CAR_STOP();
                        count_go = 0;
                        Task2_BeginTx(CAR_LINK_MSG_SET_DETECT, send_num, "det", TASK2_STATE_WAIT_REMOTE_RELEASE_BUTTON);
                        break;
                    }
                    if(Car_LineEventIsVisionFault(line_output.event))
                    {
                        Task2_RequestSafeStop("omv");
                        break;
                    }
                    Car_ApplyLineOutput(&line_output);
                }
                delay_ms(CAR_TASK_LOOP_DELAY_MS);
                break;
            }

            case TASK2_STATE_WAIT_TX_RESULT:
                Task2_ServiceTxState();
                break;

            case TASK2_STATE_WAIT_REMOTE_RELEASE_BUTTON:
                if(PAin(5)==1)
                {
                    Task2_BeginTx(CAR_LINK_MSG_LEAVE, 1u, "leave", TASK2_STATE_UTURN_ACTION);
                }
                else
                {
                    delay_ms(CAR_RETURN_BUTTON_POLL_MS);
                }
                break;

            case TASK2_STATE_UTURN_ACTION:
                if(g_task2_ctx.clock_state.action_started_ms < g_task2_ctx.clock_state.entered_ms)
                {
                    TaskRuntime_MarkAction(&g_task2_ctx.clock_state);
                    RGB_EN(DARK);
                    MOTOR_ANTI(CAR_U_TURN_SPEED,-CAR_U_TURN_SPEED);
                }
                if(TaskRuntime_IsElapsed(g_task2_ctx.clock_state.action_started_ms, CAR_U_TURN_DELAY_MS))
                {
                    CAR_STOP();
                    if(return_sta == 0)
                    {
                        return_sta = 1;
                        board = 0;
                        count_go = 0;
                        g_task2_ctx.first = 0;
                        Task2_EnterState(TASK2_STATE_MAIN_LOOP);
                    }
                    else
                    {
                        Task2_EnterState(TASK2_STATE_COMPLETE_STOP);
                    }
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK2_STATE_TURN_ACTION:
                if(g_task2_ctx.turn_dir == STRATEGY_TURN_LEFT)
                {
                    if(TaskRuntime_IsElapsed(g_task2_ctx.clock_state.action_started_ms, CAR_TURN_LEFT_DELAY_MS))
                    {
                        Task2_EnterState(g_task2_ctx.next_state);
                    }
                }
                else if(g_task2_ctx.turn_dir == STRATEGY_TURN_RIGHT)
                {
                    if(TaskRuntime_IsElapsed(g_task2_ctx.clock_state.action_started_ms, CAR_TURN_RIGHT_DELAY_MS))
                    {
                        Task2_EnterState(g_task2_ctx.next_state);
                    }
                }
                else
                {
                    Task2_EnterState(g_task2_ctx.next_state);
                }
                delay_ms(CAR_TASK_POLL_DELAY_MS);
                break;

            case TASK2_STATE_COMPLETE_STOP:
                CAR_STOP();
                RGB_EN(GREEN);
                delay_ms(CAR_SAFE_STOP_BLINK_MS);
                break;

            case TASK2_STATE_SAFE_STOP:
            default:
                CAR_STOP();
                RGB_EN(RED);
                delay_ms(CAR_SAFE_STOP_BLINK_MS);
                break;
        }
    }
}

void CAR_xunji_2(void)
{
    car_line_output_t line_output;
    if(!Car_BuildLineOutput(&line_output))
    {
        CAR_STOP();
        return;
    }
    Car_ApplyLineOutput(&line_output);
}

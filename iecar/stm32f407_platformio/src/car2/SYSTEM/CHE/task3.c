#include "task3.h"
#include "../PLATFORM/car_mainchain.h"
#include "../STRATEGY/common/task_strategy_common.h"
#include "../../../COMMON/STRATEGY/task_runtime_common_shared.h"
#include "../STRATEGY/common/task_link_runtime_common.h"

typedef enum
{
    TASK3_STATE_SCAN = 0,
    TASK3_STATE_WAIT_LEAVE,
    TASK3_STATE_FIRST_LOOP,
    TASK3_STATE_WAIT_GO,
    TASK3_STATE_GO_UTURN_ACTION,
    TASK3_STATE_MAIN_LOOP,
    TASK3_STATE_TURN_ACTION,
    TASK3_STATE_COMPLETE_STOP,
    TASK3_STATE_SAFE_STOP
} task3_state_t;

static const task_runtime_state_desc_t task3_state_desc[] =
{
    {(u8)TASK3_STATE_SCAN, "TASK3_STATE_SCAN", 0u, CAR_K210_SCAN_SAMPLE_MS},
    {(u8)TASK3_STATE_WAIT_LEAVE, "TASK3_STATE_WAIT_LEAVE", 0u, CAR_REMOTE_SIGNAL_TIMEOUT_MS},
    {(u8)TASK3_STATE_FIRST_LOOP, "TASK3_STATE_FIRST_LOOP", 1u, 0u},
    {(u8)TASK3_STATE_WAIT_GO, "TASK3_STATE_WAIT_GO", 0u, CAR_REMOTE_SIGNAL_TIMEOUT_MS},
    {(u8)TASK3_STATE_GO_UTURN_ACTION, "TASK3_STATE_GO_UTURN_ACTION", 1u, 0u},
    {(u8)TASK3_STATE_MAIN_LOOP, "TASK3_STATE_MAIN_LOOP", 1u, 0u},
    {(u8)TASK3_STATE_TURN_ACTION, "TASK3_STATE_TURN_ACTION", 1u, 0u},
    {(u8)TASK3_STATE_COMPLETE_STOP, "TASK3_STATE_COMPLETE_STOP", 0u, 0u},
    {(u8)TASK3_STATE_SAFE_STOP, "TASK3_STATE_SAFE_STOP", 0u, 0u},
};

typedef struct
{
    task3_state_t state;
    task3_state_t next_state;
    long long int scan_started_ms;
    long long int signal_wait_started_ms;
    long long int action_started_ms;
    u8 bound;
    u8 first;
    u8 turn_dir;
} task3_context_t;

static task3_context_t g_task3_ctx = {TASK3_STATE_SCAN, TASK3_STATE_SCAN, 0, 0, 0, 0, 0, 0};

static void Task3_EnterState(task3_state_t state)
{
    g_task3_ctx.state = state;
    TaskRuntime_RecordStateFromTable(3u, task3_state_desc, (u8)(sizeof(task3_state_desc) / sizeof(task3_state_desc[0])), (u8)state, 0u);
    if(state == TASK3_STATE_SCAN)
    {
        g_task3_ctx.scan_started_ms = system_time;
    }
    else if(state == TASK3_STATE_WAIT_LEAVE || state == TASK3_STATE_WAIT_GO)
    {
        g_task3_ctx.signal_wait_started_ms = system_time;
    }
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
        return 1;
    }
    Task3_RequestSafeStop("stack");
    return 0;
}

static void Task3_StartTurn(u8 turn_dir)
{
    g_task3_ctx.action_started_ms = system_time;
    g_task3_ctx.next_state = (g_task3_ctx.state == TASK3_STATE_FIRST_LOOP) ? TASK3_STATE_FIRST_LOOP : TASK3_STATE_MAIN_LOOP;
    g_task3_ctx.turn_dir = turn_dir;
    board = 0;
    ttt = CAR_TASK_TURN_HOLD_TICKS;
    if(turn_dir == STRATEGY_TURN_LEFT)
    {
        MOTOR_ANTI(0,CAR_TURN_SPEED);
        g_task3_ctx.bound += 1;
        Task3_EnterState(TASK3_STATE_TURN_ACTION);
    }
    else if(turn_dir == STRATEGY_TURN_RIGHT)
    {
        MOTOR_ANTI(CAR_TURN_SPEED,0);
        g_task3_ctx.bound += 1;
        Task3_EnterState(TASK3_STATE_TURN_ACTION);
    }
}

static void Task3_StartReturnTurn(u8 sta_sta)
{
    g_task3_ctx.action_started_ms = system_time;
    g_task3_ctx.next_state = TASK3_STATE_MAIN_LOOP;
    if(sta_sta == 1)
    {
        g_task3_ctx.turn_dir = STRATEGY_TURN_RIGHT;
        MOTOR_ANTI(CAR_TURN_SPEED,0);
        Task3_EnterState(TASK3_STATE_TURN_ACTION);
    }
    else if(sta_sta == 3)
    {
        g_task3_ctx.turn_dir = STRATEGY_TURN_LEFT;
        MOTOR_ANTI(0,CAR_TURN_SPEED);
        Task3_EnterState(TASK3_STATE_TURN_ACTION);
    }
}

static void Task3_StartGoUTurn(void)
{
    g_task3_ctx.action_started_ms = system_time;
    MOTOR_ANTI(CAR_U_TURN_SPEED,-CAR_U_TURN_SPEED);
    Task3_EnterState(TASK3_STATE_GO_UTURN_ACTION);
}

static void Task3_ShowVoteSummary(void)
{
    sprintf(sss,"val:%d-%d-%d-%d",find_val1,find_val2,find_val3,find_val4);
    OLED_ShowString(0,2,sss,16);
    sprintf(sss,"cnt:%d-%d-%d-%d",val1_cnt,val2_cnt,val3_cnt,val4_cnt);
    OLED_ShowString(0,4,sss,16);
}

/*
 * 功能: 处理 car2 Task3 巡线中遇到转向板后的视觉投票决策。
 * 入参: 无，消费当前 K210 票池和 detect_num。
 * 出参: 无，可能进入转向态或安全停车态。
 * 异常: K210 观测被 confidence/position/freshness 门控时进入 SAFE_STOP。
 * 边界: 该函数会清空当前投票窗口，调用方不得在同一窗口重复决策。
 */
static void Task3_HandleBoardDecision(void)
{
    strategy_vote_plan_t plan;
    count_go = 0;
    find_MAX(1);
    find_MAX(2);
    Strategy_FilterVoteResults();
    K210_ResetVoteBuffers();
    Task3_ShowVoteSummary();
    plan = Strategy_BuildVotePlan((u8)detect_num);
    count_go = 1;
    board = 0;
    if(plan.confidence_gated)
    {
        Task3_RequestSafeStop("k210");
        return;
    }
    if(plan.matched)
    {
        if(plan.turn_dir == STRATEGY_TURN_LEFT)
        {
            if(!Task3_RecordTurn(1))
            {
                return;
            }
        }
        else if(plan.turn_dir == STRATEGY_TURN_RIGHT)
        {
            if(!Task3_RecordTurn(3))
            {
                return;
            }
        }
        Task3_StartTurn(plan.turn_dir);
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
                if(TaskRuntime_IsElapsed(g_task3_ctx.scan_started_ms, CAR_K210_SCAN_SAMPLE_MS))
                {
                    count_go=1;
                    detect_num=find_max(1);
                    f(1,detect_num,k210_cnt_l[detect_num]);
                    sprintf(sss,"N-%d",detect_num);
                    OLED_ShowString(0,0,sss,16);
                    sprintf(sss,"omv-%d-b-%d",openmvdata[0],black_num);
                    OLED_ShowString(0,5,sss,16);
                    if(k210_cnt_l[detect_num] >= CAR_K210_SCAN_LOCK_THRESHOLD && K210_ObservationIsReliable(K210_CAMERA_LEFT))
                    {
                        count_go=0;
                        K210_ResetVoteBuffers();
                        OLED_ShowString(0,3,"GOGOGO!",16);
                        Task3_EnterState(TASK3_STATE_WAIT_LEAVE);
                    }
                    else
                    {
                        count_go=0;
                        K210_ResetVoteBuffers();
                        g_task3_ctx.scan_started_ms = system_time;
                    }
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK3_STATE_WAIT_LEAVE:
            {
                task_remote_wait_result_t wait_result;
                RGB_EN(YELLOW);
                sprintf(sss,"lv%ld", (long)(system_time - g_task3_ctx.signal_wait_started_ms));
                OLED_ShowString(0,5,sss,16);
                wait_result = TaskLink_WaitForRemoteFlag(&leave_flag, g_task3_ctx.signal_wait_started_ms, CAR_REMOTE_SIGNAL_TIMEOUT_MS, "T3");
                if(wait_result == TASK_REMOTE_WAIT_READY)
                {
                    RGB_EN(DARK);
                    board=0;
                    g_task3_ctx.bound=0;
                    flag_go=1;
                    Task3_EnterState(TASK3_STATE_FIRST_LOOP);
                }
                else if(wait_result == TASK_REMOTE_WAIT_TIMEOUT)
                {
                    Task3_RequestSafeStop("leave");
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;
            }

            case TASK3_STATE_FIRST_LOOP:
            {
                car_line_output_t line_output;
                if(flag_go!=1)
                {
                    Task3_RequestSafeStop("flag");
                    break;
                }
                if(board==1 && time_up==0)
                {
                    Task3_StartTurn(STRATEGY_TURN_RIGHT);
                    break;
                }
                if(!Car_BuildLineOutput(&line_output))
                {
                    Task3_RequestSafeStop("line");
                    break;
                }
                if(line_output.event == CAR_LINE_EVENT_STATION)
                {
                    CAR_STOP();
                    Task3_EnterState(TASK3_STATE_WAIT_GO);
                    break;
                }
                if(Car_LineEventIsVisionFault(line_output.event))
                {
                    Task3_RequestSafeStop("omv");
                    break;
                }
                Car_ApplyLineOutput(&line_output);
                if(g_task3_ctx.bound >= 2)
                {
                    RGB_EN(BLUE);
                    g_task3_ctx.first = 0;
                    Task3_EnterState(TASK3_STATE_MAIN_LOOP);
                }
                delay_ms(CAR_TURN_SETTLE_MS);
                break;
            }

            case TASK3_STATE_WAIT_GO:
            {
                task_remote_wait_result_t wait_result;
                sprintf(sss,"go%ld", (long)(system_time - g_task3_ctx.signal_wait_started_ms));
                OLED_ShowString(0,5,sss,16);
                wait_result = TaskLink_WaitForRemoteFlag(&go_flag, g_task3_ctx.signal_wait_started_ms, CAR_REMOTE_SIGNAL_TIMEOUT_MS, "T3");
                if(wait_result == TASK_REMOTE_WAIT_READY)
                {
                    Task3_StartGoUTurn();
                }
                else if(wait_result == TASK_REMOTE_WAIT_TIMEOUT)
                {
                    Task3_RequestSafeStop("go");
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;
            }

            case TASK3_STATE_GO_UTURN_ACTION:
                if(TaskRuntime_IsElapsed(g_task3_ctx.action_started_ms, CAR_U_TURN_DELAY_MS))
                {
                    g_task3_ctx.bound=1;
                    Task3_EnterState(TASK3_STATE_FIRST_LOOP);
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK3_STATE_MAIN_LOOP:
            {
                car_line_output_t line_output;
                if(flag_go!=1)
                {
                    Task3_RequestSafeStop("flag");
                    break;
                }
                if(return_sta==1)
                {
                    if(board==1 && time_up==0)
                    {
                        u8 sta_sta;
                        if(!CarPathStack_Pop(&sta_sta))
                        {
                            Task3_RequestSafeStop("stack");
                            break;
                        }
                        Task3_StartReturnTurn(sta_sta);
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task3_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        Task3_RequestCompleteStop("ret", 1u);
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
                    if(g_task3_ctx.first==0)
                    {
                        count_go=1;
                        g_task3_ctx.first=1;
                    }
                    if(board==1 && time_up==0 && (detect_num==1 || detect_num==2))
                    {
                        if(detect_num==1)
                        {
                            if(!Task3_RecordTurn(1))
                            {
                                break;
                            }
                            Task3_StartTurn(STRATEGY_TURN_LEFT);
                        }
                        else
                        {
                            if(!Task3_RecordTurn(3))
                            {
                                break;
                            }
                            Task3_StartTurn(STRATEGY_TURN_RIGHT);
                        }
                        break;
                    }
                    if(board==1 && time_up==0 && (detect_num!=1 && detect_num!=2))
                    {
                        Task3_HandleBoardDecision();
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task3_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        Task3_RequestCompleteStop("done", 2u);
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
                    if(TaskRuntime_IsElapsed(g_task3_ctx.action_started_ms, CAR_TURN_LEFT_DELAY_MS))
                    {
                        Task3_EnterState(g_task3_ctx.next_state);
                    }
                }
                else if(g_task3_ctx.turn_dir == STRATEGY_TURN_RIGHT)
                {
                    if(TaskRuntime_IsElapsed(g_task3_ctx.action_started_ms, CAR_TURN_RIGHT_DELAY_MS))
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

void CAR_xunji_3(void)
{
    car_line_output_t line_output;
    if(!Car_BuildLineOutput(&line_output))
    {
        CAR_STOP();
        return;
    }
    Car_ApplyLineOutput(&line_output);
}

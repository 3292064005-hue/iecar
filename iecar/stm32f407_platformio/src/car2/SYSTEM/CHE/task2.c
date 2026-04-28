#include "task2.h"
#include "../PLATFORM/car_mainchain.h"
#include "../STRATEGY/common/task_strategy_common.h"
#include "../../../COMMON/STRATEGY/task_runtime_common_shared.h"
#include "../STRATEGY/common/task_link_runtime_common.h"

typedef enum
{
    TASK2_STATE_WAIT_DETECT = 0,
    TASK2_STATE_WAIT_START,
    TASK2_STATE_MAIN_LOOP,
    TASK2_STATE_WAIT_REMOTE_LEAVE,
    TASK2_STATE_POST_LEAVE_DELAY,
    TASK2_STATE_TURN_ACTION,
    TASK2_STATE_UTURN_ACTION,
    TASK2_STATE_COMPLETE_STOP,
    TASK2_STATE_SAFE_STOP
} task2_state_t;

static const task_runtime_state_desc_t task2_state_desc[] =
{
    {(u8)TASK2_STATE_WAIT_DETECT, "TASK2_STATE_WAIT_DETECT", 0u, 0u},
    {(u8)TASK2_STATE_WAIT_START, "TASK2_STATE_WAIT_START", 0u, 0u},
    {(u8)TASK2_STATE_MAIN_LOOP, "TASK2_STATE_MAIN_LOOP", 1u, 0u},
    {(u8)TASK2_STATE_WAIT_REMOTE_LEAVE, "TASK2_STATE_WAIT_REMOTE_LEAVE", 0u, 0u},
    {(u8)TASK2_STATE_POST_LEAVE_DELAY, "TASK2_STATE_POST_LEAVE_DELAY", 0u, 0u},
    {(u8)TASK2_STATE_TURN_ACTION, "TASK2_STATE_TURN_ACTION", 1u, 0u},
    {(u8)TASK2_STATE_UTURN_ACTION, "TASK2_STATE_UTURN_ACTION", 1u, 0u},
    {(u8)TASK2_STATE_COMPLETE_STOP, "TASK2_STATE_COMPLETE_STOP", 0u, 0u},
    {(u8)TASK2_STATE_SAFE_STOP, "TASK2_STATE_SAFE_STOP", 0u, 0u},
};

typedef enum
{
    TASK2_RESULT_NONE = 0,
    TASK2_RESULT_LOCAL_COMPLETE = 1,
    TASK2_RESULT_REMOTE_COMPLETE = 2,
    TASK2_RESULT_LOCAL_FAIL = 3,
    TASK2_RESULT_REMOTE_FAIL = 4
} task2_completion_result_t;

typedef struct
{
    task2_state_t state;
    task2_state_t next_state;
    long long int detect_wait_started_ms;
    long long int signal_wait_started_ms;
    long long int action_started_ms;
    u8 first;
    u8 jinru;
    task2_completion_result_t result;
    const char *complete_reason;
} task2_context_t;

u8 t2dir=0;
u8 send_num=0;
int ttt=0;

static task2_context_t g_task2_ctx = {TASK2_STATE_WAIT_DETECT, TASK2_STATE_WAIT_DETECT, 0, 0, 0, 0, 0, TASK2_RESULT_NONE, 0};

static void Task2_EnterState(task2_state_t state)
{
    g_task2_ctx.state = state;
    TaskRuntime_RecordStateFromTable(2u, task2_state_desc, (u8)(sizeof(task2_state_desc) / sizeof(task2_state_desc[0])), (u8)state, 0u);
    if(state == TASK2_STATE_WAIT_DETECT)
    {
        g_task2_ctx.detect_wait_started_ms = system_time;
    }
    else if(state == TASK2_STATE_WAIT_REMOTE_LEAVE)
    {
        g_task2_ctx.signal_wait_started_ms = system_time;
    }
}

static void Task2_RequestSafeStop(const char *reason)
{
    g_task2_ctx.result = TASK2_RESULT_LOCAL_FAIL;
    TaskRuntime_EnterSafeStopForTask(2u, 0u, "T2", reason);
    g_task2_ctx.next_state = TASK2_STATE_SAFE_STOP;
    Task2_EnterState(TASK2_STATE_SAFE_STOP);
}

/*
 * 功能: 进入 car2 Task2 正常完成态，并记录完成原因/结果，避免把正常到站误归类为 SAFE_STOP。
 * 入参: reason 为 OLED/诊断短原因；result 为本地完成或远端协同完成枚举。
 * 出参: 无。
 * 异常: 空 reason 只影响显示，不影响停车和状态切换。
 * 边界: 只用于业务成功闭环；异常路径必须继续调用 Task2_RequestSafeStop。
 */
static void Task2_RequestCompleteStop(const char *reason, task2_completion_result_t result)
{
    g_task2_ctx.result = result;
    g_task2_ctx.complete_reason = reason;
    TaskRuntime_EnterCompleteStopForTask(2u, (u8)result, "T2C", reason);
    Task2_EnterState(TASK2_STATE_COMPLETE_STOP);
}

static u8 Task2_RecordTurn(u8 step)
{
    if(CarPathStack_Push(step))
    {
        return 1;
    }
    Task2_RequestSafeStop("stack");
    return 0;
}

static void Task2_StartTurn(u8 turn_dir)
{
    g_task2_ctx.action_started_ms = system_time;
    g_task2_ctx.next_state = TASK2_STATE_MAIN_LOOP;
    board = 0;
    ttt = CAR_TASK_TURN_HOLD_TICKS;
    if(turn_dir == STRATEGY_TURN_LEFT)
    {
        if(!Task2_RecordTurn(1))
        {
            return;
        }
        t2dir = 1;
        MOTOR_ANTI(0,CAR_TURN_SPEED);
        Task2_EnterState(TASK2_STATE_TURN_ACTION);
    }
    else if(turn_dir == STRATEGY_TURN_RIGHT)
    {
        if(!Task2_RecordTurn(3))
        {
            return;
        }
        t2dir = 2;
        MOTOR_ANTI(CAR_TURN_SPEED,0);
        Task2_EnterState(TASK2_STATE_TURN_ACTION);
    }
}

static void Task2_StartReturnTurn(u8 sta_sta)
{
    g_task2_ctx.action_started_ms = system_time;
    g_task2_ctx.next_state = TASK2_STATE_MAIN_LOOP;
    board = 0;
    ttt = CAR_TASK_TURN_HOLD_TICKS;
    if(sta_sta == 1)
    {
        t2dir = 2;
        MOTOR_ANTI(CAR_TURN_SPEED,0);
        Task2_EnterState(TASK2_STATE_TURN_ACTION);
    }
    else if(sta_sta == 3)
    {
        t2dir = 1;
        MOTOR_ANTI(0,CAR_TURN_SPEED);
        Task2_EnterState(TASK2_STATE_TURN_ACTION);
    }
}

static void Task2_BeginUTurn(void)
{
    g_task2_ctx.action_started_ms = system_time;
    g_task2_ctx.jinru = 1;
    ttt = CAR_TASK_ENTRY_TURN_HOLD_TICKS;
    MOTOR_ANTI(CAR_U_TURN_SPEED,-CAR_U_TURN_SPEED);
    Task2_EnterState(TASK2_STATE_UTURN_ACTION);
}

static void Task2_ShowVoteSummary(void)
{
    sprintf(sss,"val:%d-%d-%d-%d",find_val1,find_val2,find_val3,find_val4);
    OLED_ShowString(0,2,sss,16);
    sprintf(sss,"cnt:%d-%d-%d-%d",val1_cnt,val2_cnt,val3_cnt,val4_cnt);
    OLED_ShowString(0,4,sss,16);
}

static void Task2_HandleBoardDecision(void)
{
    strategy_vote_plan_t plan;
    count_go = 0;
    find_MAX(1);
    find_MAX(2);
    Strategy_FilterVoteResults();
    K210_ResetVoteBuffers();
    Task2_ShowVoteSummary();
    plan = Strategy_BuildVotePlan((u8)detect_num);
    count_go = 1;
    board = 0;
    if(plan.confidence_gated)
    {
        Task2_RequestSafeStop("k210");
        return;
    }
    if(plan.matched)
    {
        Task2_StartTurn(plan.turn_dir);
    }
}

void TASK2_GO(void)
{
    Task2_EnterState(TASK2_STATE_WAIT_DETECT);
    while(1)
    {
        TaskLink_ServiceKeepalive("T2");
        switch(g_task2_ctx.state)
        {
            case TASK2_STATE_WAIT_DETECT:
                if(detect_num == CAR_K210_INVALID_TARGET)
                {
                    sprintf(sss,"de-%d",detect_num);
                    OLED_ShowString(0,3,sss,16);
                    RGB_EN(WHITE);
                    if(TaskRuntime_IsElapsed(g_task2_ctx.detect_wait_started_ms, CAR_REMOTE_SIGNAL_TIMEOUT_MS))
                    {
                        Task2_RequestSafeStop("detect");
                    }
                }
                else
                {
                    sprintf(sss,"de-%d",detect_num);
                    OLED_ShowString(0,3,sss,16);
                    RGB_EN(DARK);
                    Task2_EnterState(TASK2_STATE_WAIT_START);
                }
                delay_ms(CAR_TASK_POLL_DELAY_MS);
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
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK2_STATE_MAIN_LOOP:
            {
                car_line_output_t line_output;
                if(flag_go!=1)
                {
                    Task2_RequestSafeStop("flag");
                    break;
                }
                if(return_sta==1)
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
                        Task2_RequestCompleteStop("return", TASK2_RESULT_LOCAL_COMPLETE);
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
                    if(g_task2_ctx.first==0)
                    {
                        count_go=1;
                        g_task2_ctx.first=1;
                    }
                    if(board==1 && time_up==0 && (detect_num==1 || detect_num==2))
                    {
                        if(detect_num==1) Task2_StartTurn(STRATEGY_TURN_LEFT);
                        else Task2_StartTurn(STRATEGY_TURN_RIGHT);
                        break;
                    }
                    if(board==1 && time_up==0 && (detect_num!=1 && detect_num!=2))
                    {
                        Task2_HandleBoardDecision();
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task2_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        if(g_task2_ctx.jinru==1)
                        {
                            Task2_RequestCompleteStop("entry", TASK2_RESULT_REMOTE_COMPLETE);
                            break;
                        }
                        RGB_EN(YELLOW);
                        CAR_STOP();
                        Task2_EnterState(TASK2_STATE_WAIT_REMOTE_LEAVE);
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

            case TASK2_STATE_WAIT_REMOTE_LEAVE:
            {
                task_remote_wait_result_t wait_result;
                sprintf(sss,"leave%ld", (long)(system_time - g_task2_ctx.signal_wait_started_ms));
                OLED_ShowString(0,5,sss,16);
                wait_result = TaskLink_WaitForRemoteFlag(&leave_flag, g_task2_ctx.signal_wait_started_ms, CAR_REMOTE_SIGNAL_TIMEOUT_MS, "T2");
                if(wait_result == TASK_REMOTE_WAIT_READY)
                {
                    RGB_EN(DARK);
                    g_task2_ctx.action_started_ms = system_time;
                    Task2_EnterState(TASK2_STATE_POST_LEAVE_DELAY);
                }
                else if(wait_result == TASK_REMOTE_WAIT_TIMEOUT)
                {
                    Task2_RequestSafeStop("leave");
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;
            }

            case TASK2_STATE_POST_LEAVE_DELAY:
                if(TaskRuntime_IsElapsed(g_task2_ctx.action_started_ms, CAR_REMOTE_POST_LEAVE_DELAY_MS))
                {
                    Task2_BeginUTurn();
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK2_STATE_TURN_ACTION:
                if(t2dir == 1)
                {
                    if(TaskRuntime_IsElapsed(g_task2_ctx.action_started_ms, CAR_TURN_LEFT_DELAY_MS))
                    {
                        Task2_EnterState(g_task2_ctx.next_state);
                    }
                }
                else if(t2dir == 2)
                {
                    if(TaskRuntime_IsElapsed(g_task2_ctx.action_started_ms, CAR_TURN_RIGHT_DELAY_MS))
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

            case TASK2_STATE_UTURN_ACTION:
                if(TaskRuntime_IsElapsed(g_task2_ctx.action_started_ms, CAR_U_TURN_DELAY_MS))
                {
                    Task2_EnterState(TASK2_STATE_MAIN_LOOP);
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK2_STATE_COMPLETE_STOP:
                CAR_STOP();
                RGB_EN(GREEN);
                if(g_task2_ctx.complete_reason != 0)
                {
                    TaskRuntime_ShowReason("T2C", g_task2_ctx.complete_reason);
                }
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

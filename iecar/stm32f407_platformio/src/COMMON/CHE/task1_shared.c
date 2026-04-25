#include "../STRATEGY/task_runtime_common_shared.h"

typedef enum
{
    TASK1_STATE_SCAN = 0,
    TASK1_STATE_WAIT_START,
    TASK1_STATE_MAIN_LOOP,
    TASK1_STATE_WAIT_RETURN_BUTTON,
    TASK1_STATE_TURN_ACTION,
    TASK1_STATE_UTURN_ACTION,
    TASK1_STATE_SAFE_STOP,
    TASK1_STATE_COMPLETE_STOP
} task1_state_t;

typedef struct
{
    task1_state_t state;
    task1_state_t next_state;
    task_runtime_clock_t clock_state;
    u8 first;
    u8 turn_dir;
} task1_context_t;

static task1_context_t g_task1_ctx = {TASK1_STATE_SCAN, TASK1_STATE_SCAN, {0,0,0}, 0, 0};

static void Task1_EnterState(task1_state_t state)
{
    g_task1_ctx.state = state;
    TaskRuntime_RecordState(1u, (u8)state, 0u);
    TaskRuntime_MarkEntered(&g_task1_ctx.clock_state);
}

static void Task1_ClearK210Votes(void)
{
    K210_ResetVoteBuffers();
}

static void Task1_RequestSafeStop(const char *reason)
{
    TaskRuntime_EnterSafeStop("T1", reason);
    Task1_EnterState(TASK1_STATE_SAFE_STOP);
}

static u8 Task1_RecordTurn(u8 step)
{
    if(CarPathStack_Push(step))
    {
        return 1u;
    }
    Task1_RequestSafeStop("stack");
    return 0u;
}

static void Task1_StartTurn(u8 turn_dir, u8 record_path)
{
    g_task1_ctx.turn_dir = turn_dir;
    g_task1_ctx.next_state = TASK1_STATE_MAIN_LOOP;
    TaskRuntime_MarkAction(&g_task1_ctx.clock_state);
    board = 0;
    ttt = CAR_TASK_TURN_HOLD_TICKS;
    if(turn_dir == STRATEGY_TURN_LEFT)
    {
        if(record_path && !Task1_RecordTurn(1u))
        {
            return;
        }
        MOTOR_ANTI(0, CAR_TURN_SPEED);
        Task1_EnterState(TASK1_STATE_TURN_ACTION);
    }
    else if(turn_dir == STRATEGY_TURN_RIGHT)
    {
        if(record_path && !Task1_RecordTurn(3u))
        {
            return;
        }
        MOTOR_ANTI(CAR_TURN_SPEED, 0);
        Task1_EnterState(TASK1_STATE_TURN_ACTION);
    }
}

static void Task1_StartReturnTurn(u8 step)
{
    if(step == 1u)
    {
        Task1_StartTurn(STRATEGY_TURN_RIGHT, 0u);
    }
    else if(step == 3u)
    {
        Task1_StartTurn(STRATEGY_TURN_LEFT, 0u);
    }
    else
    {
        Task1_RequestSafeStop("path");
    }
}

static void Task1_ShowVoteSummary(void)
{
    sprintf(sss,"val:%d-%d-%d-%d",find_val1,find_val2,find_val3,find_val4);
    OLED_ShowString(0,2,sss,16);
    sprintf(sss,"cnt:%d-%d-%d-%d",val1_cnt,val2_cnt,val3_cnt,val4_cnt);
    OLED_ShowString(0,4,sss,16);
}

static void Task1_HandleVoteTurn(void)
{
    strategy_vote_plan_t plan;
    count_go = 0;
    find_MAX(K210_CAMERA_LEFT);
    find_MAX(K210_CAMERA_RIGHT);
    Strategy_FilterVoteResults();
    Task1_ShowVoteSummary();
    plan = Strategy_BuildVotePlan((u8)detect_num);
    Task1_ClearK210Votes();
    count_go = 1;
    board = 0;
    if(plan.confidence_gated)
    {
        Task1_RequestSafeStop("k210");
        return;
    }
    if(plan.matched)
    {
        Task1_StartTurn(plan.turn_dir, 1u);
    }
}

void TASK1_GO(void)
{
    Task1_EnterState(TASK1_STATE_SCAN);
    while(1)
    {
        CarLink_ServiceHeartbeat();
        switch(g_task1_ctx.state)
        {
            case TASK1_STATE_SCAN:
                count_go = 1;
                if(TaskRuntime_IsElapsed(g_task1_ctx.clock_state.entered_ms, CAR_K210_SCAN_SAMPLE_MS))
                {
                    detect_num = find_max(K210_CAMERA_LEFT);
                    sprintf(sss,"N-%d",detect_num);
                    OLED_ShowString(0,0,sss,16);
                    sprintf(sss,"omv-%d-b-%d",openmvdata[0],black_num);
                    OLED_ShowString(0,5,sss,16);
                    if(k210_cnt_l[detect_num] >= CAR_K210_SCAN_LOCK_THRESHOLD && K210_ObservationIsReliable(K210_CAMERA_LEFT))
                    {
                        count_go = 0;
                        Task1_ClearK210Votes();
                        OLED_ShowString(0,3,"GOGOGO!",16);
                        Task1_EnterState(TASK1_STATE_WAIT_START);
                    }
                    else
                    {
                        count_go = 0;
                        Task1_ClearK210Votes();
                        Task1_EnterState(TASK1_STATE_SCAN);
                    }
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK1_STATE_WAIT_START:
                if(PAin(5) == 0)
                {
                    delay_ms(CAR_START_BUTTON_DEBOUNCE_MS);
                    flag_go = 1;
                    g_task1_ctx.first = 0;
                    count_go = 0;
                    Task1_EnterState(TASK1_STATE_MAIN_LOOP);
                }
                else
                {
                    delay_ms(CAR_RETURN_BUTTON_POLL_MS);
                }
                break;

            case TASK1_STATE_MAIN_LOOP:
            {
                car_line_output_t line_output;
                if(flag_go != 1)
                {
                    Task1_RequestSafeStop("flag");
                    break;
                }
                if(return_sta == 1)
                {
                    if(board == 1 && time_up == 0)
                    {
                        u8 sta_sta;
                        if(!CarPathStack_Pop(&sta_sta))
                        {
                            Task1_RequestSafeStop("stack");
                            break;
                        }
                        Task1_StartReturnTurn(sta_sta);
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task1_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        RGB_EN(GREEN);
                        CAR_STOP();
                        Task1_EnterState(TASK1_STATE_COMPLETE_STOP);
                        break;
                    }
                    if(Car_LineEventIsVisionFault(line_output.event))
                    {
                        Task1_RequestSafeStop("omv");
                        break;
                    }
                    Car_ApplyLineOutput(&line_output);
                }
                else
                {
                    if(g_task1_ctx.first == 0)
                    {
                        count_go = 1;
                        g_task1_ctx.first = 1;
                    }
                    if(board == 1 && time_up == 0)
                    {
                        if(detect_num == 1)
                        {
                            Task1_StartTurn(STRATEGY_TURN_LEFT, 1u);
                            break;
                        }
                        if(detect_num == 2)
                        {
                            Task1_StartTurn(STRATEGY_TURN_RIGHT, 1u);
                            break;
                        }
                        Task1_HandleVoteTurn();
                        break;
                    }
                    if(!Car_BuildLineOutput(&line_output))
                    {
                        Task1_RequestSafeStop("line");
                        break;
                    }
                    if(line_output.event == CAR_LINE_EVENT_STATION)
                    {
                        delay_ms(CAR_STATION_STOP_DELAY_MS);
                        RGB_EN(RED);
                        CAR_STOP();
                        count_go = 0;
                        TaskRuntime_MarkAction(&g_task1_ctx.clock_state);
                        Task1_EnterState(TASK1_STATE_WAIT_RETURN_BUTTON);
                        break;
                    }
                    if(Car_LineEventIsVisionFault(line_output.event))
                    {
                        Task1_RequestSafeStop("omv");
                        break;
                    }
                    Car_ApplyLineOutput(&line_output);
                }
                delay_ms(CAR_TASK_LOOP_DELAY_MS);
                break;
            }

            case TASK1_STATE_WAIT_RETURN_BUTTON:
                if(!TaskRuntime_IsElapsed(g_task1_ctx.clock_state.action_started_ms, CAR_K210_SCAN_SAMPLE_MS))
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                    break;
                }
                if(PAin(5) == 1)
                {
                    delay_ms(CAR_WAIT_BUTTON_RELEASE_MS);
                    RGB_EN(DARK);
                    MOTOR_ANTI(CAR_U_TURN_SPEED, -CAR_U_TURN_SPEED);
                    TaskRuntime_MarkAction(&g_task1_ctx.clock_state);
                    Task1_EnterState(TASK1_STATE_UTURN_ACTION);
                }
                else
                {
                    delay_ms(CAR_RETURN_BUTTON_POLL_MS);
                }
                break;

            case TASK1_STATE_UTURN_ACTION:
                if(TaskRuntime_IsElapsed(g_task1_ctx.clock_state.action_started_ms, CAR_U_TURN_DELAY_MS))
                {
                    CAR_STOP();
                    if(return_sta == 0)
                    {
                        return_sta = 1;
                        board = 0;
                        count_go = 0;
                        g_task1_ctx.first = 0;
                        Task1_EnterState(TASK1_STATE_MAIN_LOOP);
                    }
                    else
                    {
                        Task1_EnterState(TASK1_STATE_COMPLETE_STOP);
                    }
                }
                else
                {
                    delay_ms(CAR_TASK_POLL_DELAY_MS);
                }
                break;

            case TASK1_STATE_TURN_ACTION:
                if(g_task1_ctx.turn_dir == STRATEGY_TURN_LEFT)
                {
                    if(TaskRuntime_IsElapsed(g_task1_ctx.clock_state.action_started_ms, CAR_TURN_LEFT_DELAY_MS))
                    {
                        Task1_EnterState(g_task1_ctx.next_state);
                    }
                }
                else if(g_task1_ctx.turn_dir == STRATEGY_TURN_RIGHT)
                {
                    if(TaskRuntime_IsElapsed(g_task1_ctx.clock_state.action_started_ms, CAR_TURN_RIGHT_DELAY_MS))
                    {
                        Task1_EnterState(g_task1_ctx.next_state);
                    }
                }
                else
                {
                    Task1_EnterState(g_task1_ctx.next_state);
                }
                delay_ms(CAR_TASK_POLL_DELAY_MS);
                break;

            case TASK1_STATE_COMPLETE_STOP:
                CAR_STOP();
                RGB_EN(GREEN);
                delay_ms(CAR_SAFE_STOP_BLINK_MS);
                break;

            case TASK1_STATE_SAFE_STOP:
            default:
                CAR_STOP();
                RGB_EN(RED);
                delay_ms(CAR_SAFE_STOP_BLINK_MS);
                break;
        }
    }
}

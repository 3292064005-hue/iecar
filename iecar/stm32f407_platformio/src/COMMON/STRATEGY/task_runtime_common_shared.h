#ifndef TASK_RUNTIME_COMMON_SHARED_H__
#define TASK_RUNTIME_COMMON_SHARED_H__

#include "../PLATFORM/car_platform_shared.h"

typedef struct
{
    long long int entered_ms;
    long long int action_started_ms;
    long long int signal_started_ms;
} task_runtime_clock_t;

typedef struct
{
    u8 state_id;
    const char *name;
} task_runtime_state_desc_t;

static inline const task_runtime_state_desc_t *TaskRuntime_FindStateDesc(const task_runtime_state_desc_t *table, u8 count, u8 state_id)
{
    u8 i;
    if(table == 0)
    {
        return 0;
    }
    for(i = 0u; i < count; ++i)
    {
        if(table[i].state_id == state_id)
        {
            return &table[i];
        }
    }
    return 0;
}

/*
 * 功能: 基于统一状态描述表记录任务状态转移，避免各任务散落裸状态值。
 * 入参: task_id 为任务号；table/count 为状态描述表；state_id 为目标状态；reason_code 为转移原因。
 * 出参: 返回 1 表示目标状态存在于状态表，返回 0 表示状态未登记但仍会记录诊断。
 * 异常: table 为空或状态未登记时不阻塞任务主链路，只暴露诊断结果。
 * 边界: 本函数只做状态登记与诊断，不修改调用方状态变量。
 */
static inline u8 TaskRuntime_RecordStateFromTable(u8 task_id,
                                                  const task_runtime_state_desc_t *table,
                                                  u8 count,
                                                  u8 state_id,
                                                  u8 reason_code)
{
    const task_runtime_state_desc_t *desc = TaskRuntime_FindStateDesc(table, count, state_id);
    CarDiag_Record(CAR_DIAG_TASK_STATE, task_id, state_id, reason_code);
    return (u8)(desc != 0);
}

static inline void TaskRuntime_MarkEntered(task_runtime_clock_t *clock_state)
{
    if(clock_state)
    {
        clock_state->entered_ms = system_time;
    }
}

static inline void TaskRuntime_MarkAction(task_runtime_clock_t *clock_state)
{
    if(clock_state)
    {
        clock_state->action_started_ms = system_time;
    }
}

static inline void TaskRuntime_MarkSignal(task_runtime_clock_t *clock_state)
{
    if(clock_state)
    {
        clock_state->signal_started_ms = system_time;
    }
}

static inline u8 TaskRuntime_IsElapsed(long long int start_ms, u32 duration_ms)
{
    return (u8)((u32)(system_time - start_ms) >= duration_ms);
}

static inline void TaskRuntime_RecordState(u8 task_id, u8 state_id, u8 reason_code)
{
    CarDiag_Record(CAR_DIAG_TASK_STATE, task_id, state_id, reason_code);
}

static inline void TaskRuntime_RecordComplete(u8 task_id, u8 reason_code)
{
    CarDiag_Record(CAR_DIAG_TASK_COMPLETE, task_id, reason_code, 0u);
}

static inline void TaskRuntime_RecordSafeStop(u8 task_id, u8 reason_code)
{
    CarDiag_Record(CAR_DIAG_TASK_SAFE_STOP, task_id, reason_code, 0u);
}

static inline void TaskRuntime_ShowReason(const char *prefix, const char *reason)
{
    if(prefix == 0 || reason == 0)
    {
        return;
    }
    sprintf(sss, "%s-%s", prefix, reason);
    OLED_ShowString(0, 6, sss, 16);
}

static inline void TaskRuntime_EnterSafeStop(const char *prefix, const char *reason)
{
    TaskRuntime_ShowReason(prefix, reason);
    CAR_STOP();
    RGB_EN(RED);
    flag_go = 0;
    CarDiag_Record(CAR_DIAG_TASK_SAFE_STOP, 0u, 0u, 0u);
}

#endif /* TASK_RUNTIME_COMMON_SHARED_H__ */

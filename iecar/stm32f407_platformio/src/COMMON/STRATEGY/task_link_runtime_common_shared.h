#ifndef TASK_LINK_RUNTIME_COMMON_SHARED_H__
#define TASK_LINK_RUNTIME_COMMON_SHARED_H__

#ifndef CAR_TASK_LINK_CTRL_HEADER
#error "CAR_TASK_LINK_CTRL_HEADER must be defined before including task_link_runtime_common_shared.h"
#endif

#include CAR_TASK_LINK_CTRL_HEADER
#include "task_runtime_common_shared.h"

typedef enum
{
    TASK_REMOTE_WAIT_PENDING = 0,
    TASK_REMOTE_WAIT_READY = 1,
    TASK_REMOTE_WAIT_TIMEOUT = 2
} task_remote_wait_result_t;

static inline void TaskLink_ServiceKeepalive(const char *prefix)
{
    car_link_health_t health;
    CarLink_ServiceRx();
    CarLink_ServiceTx();
    CarLink_ServiceTxQueue();
    CarLink_ServiceHeartbeat();
    CarLink_ServiceTxQueue();
    health = CarLink_GetHealth();
    if(prefix != 0 && health != CAR_LINK_HEALTHY)
    {
        TaskRuntime_ShowReason(prefix, (health == CAR_LINK_LOST) ? "link-l" : "link-w");
        CarDiag_RecordThrottled(CAR_DIAG_LINK_DEGRADED, (u8)health, 0u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
    }
}

/*
 * 功能: 等待指定远端消息，优先消费 CarLink 事件邮箱，兼容旧 flag 镜像。
 * 入参: msg_type 为期望消息；flag 为旧实现兼容标志；started_ms/timeout_ms 控制超时；prefix 用于显示。
 * 出参: READY/TIMEOUT/PENDING。
 * 异常: 链路 LOST 或等待超时均返回 TIMEOUT，由任务层决定是否安全停车。
 * 边界: 事件消费成功会自动清掉对应旧 flag，避免重复触发。
 */
static inline task_remote_wait_result_t TaskLink_WaitForRemoteMessage(u8 msg_type,
                                                                      volatile u8 *flag,
                                                                      long long int started_ms,
                                                                      u32 timeout_ms,
                                                                      const char *prefix)
{
    u8 value = 0u;
    TaskLink_ServiceKeepalive(prefix);
    if(CarLink_GetHealth() == CAR_LINK_LOST)
    {
        CarDiag_RecordThrottled(CAR_DIAG_LINK_DEGRADED, CAR_LINK_LOST, 1u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
        return TASK_REMOTE_WAIT_TIMEOUT;
    }
    if(CarLink_ConsumeEventType(msg_type, &value))
    {
        if(flag != 0)
        {
            *flag = 0;
        }
        return TASK_REMOTE_WAIT_READY;
    }
    if(flag != 0 && *flag)
    {
        *flag = 0;
        return TASK_REMOTE_WAIT_READY;
    }
    if(TaskRuntime_IsElapsed(started_ms, timeout_ms))
    {
        return TASK_REMOTE_WAIT_TIMEOUT;
    }
    return TASK_REMOTE_WAIT_PENDING;
}

static inline task_remote_wait_result_t TaskLink_WaitForRemoteFlag(volatile u8 *flag,
                                                                   long long int started_ms,
                                                                   u32 timeout_ms,
                                                                   const char *prefix)
{
    u8 msg_type = CAR_LINK_MSG_LEAVE;
    if(flag == &go_flag)
    {
        msg_type = CAR_LINK_MSG_GO;
    }
    return TaskLink_WaitForRemoteMessage(msg_type, flag, started_ms, timeout_ms, prefix);
}

#endif /* TASK_LINK_RUNTIME_COMMON_SHARED_H__ */

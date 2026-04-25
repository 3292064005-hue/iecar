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
    CarLink_ServiceHeartbeat();
    health = CarLink_GetHealth();
    if(prefix != 0 && health != CAR_LINK_HEALTHY)
    {
        TaskRuntime_ShowReason(prefix, (health == CAR_LINK_LOST) ? "link-l" : "link-w");
        CarDiag_RecordThrottled(CAR_DIAG_LINK_DEGRADED, (u8)health, 0u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
    }
}

static inline task_remote_wait_result_t TaskLink_WaitForRemoteFlag(volatile u8 *flag,
                                                                   long long int started_ms,
                                                                   u32 timeout_ms,
                                                                   const char *prefix)
{
    TaskLink_ServiceKeepalive(prefix);
    if(CarLink_GetHealth() == CAR_LINK_LOST)
    {
        CarDiag_RecordThrottled(CAR_DIAG_LINK_DEGRADED, CAR_LINK_LOST, 1u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
        return TASK_REMOTE_WAIT_TIMEOUT;
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

#endif /* TASK_LINK_RUNTIME_COMMON_SHARED_H__ */

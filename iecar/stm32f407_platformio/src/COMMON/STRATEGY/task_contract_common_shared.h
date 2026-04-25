#ifndef TASK_CONTRACT_COMMON_SHARED_H__
#define TASK_CONTRACT_COMMON_SHARED_H__

#include "../PLATFORM/car_platform_shared.h"
#include "../CHE/car_project_config_shared.h"

static inline u8 TaskContract_Bit(u8 task)
{
    if(task == 1u) return CAR_TASK_MASK_TASK1;
    if(task == 2u) return CAR_TASK_MASK_TASK2;
    if(task == 3u) return CAR_TASK_MASK_TASK3;
    return 0u;
}

static inline u8 TaskContract_IsAllowed(u8 task)
{
    u8 bit = TaskContract_Bit(task);
    return (u8)(bit != 0u && ((CAR_ALLOWED_TASK_MASK & bit) != 0u));
}

static inline u8 TaskContract_IsLegacy(u8 task)
{
    u8 bit = TaskContract_Bit(task);
    return (u8)(bit != 0u && ((CAR_LEGACY_TASK_MASK & bit) != 0u));
}

static inline const char *TaskContract_RoleName(void)
{
    if(CAR_ROLE_ID == 1u) return "CAR1";
    if(CAR_ROLE_ID == 2u) return "CAR2";
    return "CAR?";
}

#endif /* TASK_CONTRACT_COMMON_SHARED_H__ */

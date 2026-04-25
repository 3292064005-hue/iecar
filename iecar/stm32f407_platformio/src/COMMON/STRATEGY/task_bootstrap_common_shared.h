#ifndef TASK_BOOTSTRAP_COMMON_SHARED_H__
#define TASK_BOOTSTRAP_COMMON_SHARED_H__

#ifndef CAR_TASK_BOOTSTRAP_PLATFORM_HEADER
#error "CAR_TASK_BOOTSTRAP_PLATFORM_HEADER must be defined before including task_bootstrap_common_shared.h"
#endif

#include CAR_TASK_BOOTSTRAP_PLATFORM_HEADER
#include "task_contract_common_shared.h"

typedef void (*task_bootstrap_entry_t)(void);

static inline u8 TaskBootstrap_ReadSelector(void)
{
    u8 tak1 = PEin(2);
    u8 tak2 = PEin(3);
    sprintf(sss,"S-%d%d",tak1,tak2);
    OLED_ShowString(60,0,sss,8);
    if(tak1==0 && tak2==0) return 1u;
    if(tak1==1 && tak2==0) return 2u;
    if(tak1==0 && tak2==1) return 3u;
    return 4u;
}

static inline void TaskBootstrap_ShowRoleContract(u8 task)
{
    sprintf(sss, "%s", TaskContract_RoleName());
    OLED_ShowString(0, 0, sss, 8);
    if(TaskContract_IsAllowed(task))
    {
        OLED_ShowString(0, 1, "MODE:CURR", 8);
    }
    else if(TaskContract_IsLegacy(task))
    {
        OLED_ShowString(0, 1, "MODE:LEG", 8);
    }
    else
    {
        OLED_ShowString(0, 1, "MODE:BLOCK", 8);
    }
}

static inline u8 TaskBootstrap_DispatchTaskWithContract(u8 task,
                                                        task_bootstrap_entry_t task1_entry,
                                                        task_bootstrap_entry_t task2_entry,
                                                        task_bootstrap_entry_t task3_entry)
{
    TaskBootstrap_ShowRoleContract(task);
    if(!TaskContract_IsAllowed(task) && !TaskContract_IsLegacy(task))
    {
        sprintf(sss, "T%d-BLOCK", task);
        OLED_ShowString(0, 6, sss, 8);
        RGB_EN(RED);
        CAR_STOP();
        return 0u;
    }
    if(task == 1u && task1_entry)
    {
        task1_entry();
        return 1u;
    }
    if(task == 2u && task2_entry)
    {
        task2_entry();
        return 1u;
    }
    if(task == 3u && task3_entry)
    {
        task3_entry();
        return 1u;
    }
    OLED_ShowString(0, 6, "T?-BLOCK", 8);
    RGB_EN(RED);
    CAR_STOP();
    return 0u;
}

#endif /* TASK_BOOTSTRAP_COMMON_SHARED_H__ */

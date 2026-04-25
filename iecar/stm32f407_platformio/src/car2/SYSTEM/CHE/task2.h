#ifndef task_h__
#define task_h__
#include "../PLATFORM/car_platform.h"

extern u8 t2dir;
extern u8 send_num;
extern int ttt;

/*
 * 功能: 执行任务2完整业务流程。
 * 入参: 无。
 * 出参: 无。
 * 异常: 视觉链/通信链失活时进入安全停车或等待分支。
 * 边界: 为顶层任务入口，内部自行维护任务状态，不应被并发重入。
 */
void TASK2_GO(void);

/*
 * 功能: 提供任务2专用的巡线控制输出。
 * 入参: 无。
 * 出参: 无。
 * 异常: OpenMV 失活时降级为保守直行；上层状态机会在超时/通信异常时进入安全停车。
 * 边界: 仅负责横向修正与速度下发，不直接改变任务状态机。
 */
void CAR_xunji_2(void);

#endif //task_h__

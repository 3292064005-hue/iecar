#ifndef TASK_1_H
#define TASK_1_H
#include "../PLATFORM/car_platform.h"

/*
 * 功能: 执行任务1完整业务流程。
 * 入参: 无。
 * 出参: 无。
 * 异常: 识别结果不足时维持扫描/巡线，不强行做目标决策。
 * 边界: 依赖 K210 与 OpenMV 持续供数，不适合被多处重复调用。
 */
void TASK1_GO(void);

#endif //TASK_1_H

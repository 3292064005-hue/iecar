#ifndef car_h__
#define car_h__
#include "../PLATFORM/car_platform.h"
extern u8 ss1,ss2,ss3,ss4,ss5;
extern u8 car_stop;
extern int Speed1;
extern int Speed2;
extern u8 board;
extern u8 car_sta;
extern int time_up;
extern int detect_num;
extern int vel;
extern float Kp_qwq;
extern int Speed;

typedef enum
{
    CAR_LINE_EVENT_NONE = 0,
    CAR_LINE_EVENT_STATION,
    CAR_LINE_EVENT_LINE_LOST,
    CAR_LINE_EVENT_OPENMV_NO_FRAME,
    CAR_LINE_EVENT_OPENMV_STALE
} car_line_event_t;

typedef struct
{
    int left_speed;
    int right_speed;
    u8 line_seen;
    u8 fresh;
    car_line_event_t event;
} car_line_output_t;

/*
 * 功能: 基于最新 OpenMV 数据执行基础巡线控制。
 * 入参: 无。
 * 出参: 无。
 * 异常: OpenMV 数据失活或事件未被上层消费时自动停车，避免沿用脏观测继续输出。
 * 边界: 仅负责基础巡线，不处理扩展任务的额外状态切换。
 */
void CAR_xunji(void);

/*
 * 功能: 在线调整巡线期望速度。
 * 入参: new_speed 为新的目标速度。
 * 出参: 无。
 * 异常: 本函数不做额外限幅，调用方应保证速度配置与电机能力匹配。
 * 边界: 仅修改速度配置，不立即清零当前电机状态。
 */
void Car_SetLineSpeed(int new_speed);

/*
 * 功能: 根据中心偏差计算巡线修正量。
 * 入参: center_x 为图像中心线位置。
 * 出参: 返回限幅后的转向修正值。
 * 异常: 输入异常时仍会经过统一限幅，避免输出无限放大。
 * 边界: 只做线性比例校正，复杂控制由上层状态机决定是否叠加。
 */
int Car_ComputeLineCorrection(int center_x);

/*
 * 功能: 读取当前巡线控制输出与事件，不直接执行任务级动作。
 * 入参: out 为输出对象。
 * 出参: 返回 1 表示 out 已刷新，返回 0 表示入参非法。
 * 异常: OpenMV 失活、黑块触发和无中心线都会通过 event 暴露给上层。
 * 边界: 只给出控制/事件结果，不修改 return_sta、flag_go、路径栈等任务状态。
 */
u8 Car_BuildLineOutput(car_line_output_t *out);

/*
 * 功能: 把纯控制输出下发到电机。
 * 入参: out 为由 Car_BuildLineOutput 生成的控制输出。
 * 出参: 无。
 * 异常: 非法指针或不可驱动事件时直接停车。
 * 边界: 只做速度下发，不消费任务级事件。
 */
void Car_ApplyLineOutput(const car_line_output_t *out);

/*
 * 功能: 判断巡线事件是否属于视觉输入故障。
 * 入参: event 为 Car_BuildLineOutput 产生的事件。
 * 出参: 返回 1 表示首帧缺失或数据超时，返回 0 表示可由业务继续解释。
 * 异常: 无。
 * 边界: 站点事件不属于视觉故障，调用方应按任务语义单独消费。
 */
u8 Car_LineEventIsVisionFault(car_line_event_t event);

#endif //car_h__

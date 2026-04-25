#ifndef car_uart2_h_
#define car_uart2_h_
#include "../PLATFORM/car_platform.h"
extern u8 black_num;
extern u8 zuo1,you1;
extern int openmvdata[10];
extern u8 USART3_RX_BUF[];
extern u8 USART4_RX_BUF[];
extern volatile long long int openmv_last_rx_time;
extern volatile u8 openmv_has_valid_frame;
extern volatile u16 openmv_bad_frame_count;

typedef enum
{
    OPENMV_STATUS_NO_FRAME = 0,
    OPENMV_STATUS_FRESH = 1,
    OPENMV_STATUS_STALE = 2
} openmv_status_t;

/*
 * 功能: 解析 USART3 可选镜像口上的一帧 OpenMV 巡线数据并更新 openmvdata/black_num/board。
 * 入参: 无，消费 USART3_RX_BUF 当前完整帧；仅在 CAR_OPENMV_USART3_ENABLED=1 时生效。
 * 出参: 无。
 * 异常: 帧长、头尾或校验不通过时直接丢弃。
 * 边界: 仅接受当前协议版本的固定长度帧，不做分包重组。
 */
void GXT_DATA_DEAL(void);

/*
 * 功能: 在检测到黑块/站点时拉起 board 标志并刷新保持时间。
 * 入参: 无。
 * 出参: 无。
 * 异常: 无额外异常分支，调用方需保证在有效检测条件下触发。
 * 边界: 连续触发会重置保持时间，等价于延长站点有效窗口。
 */
void detect_board(void);

/*
 * 功能: 判断 OpenMV 数据是否仍然鲜活。
 * 入参: timeout_ms 为允许的最大静默时间。
 * 出参: 返回 1 表示鲜活，返回 0 表示超时。
 * 异常: system_time 异常会影响判定结果。
 * 边界: 适合巡线控制前做失效保护，超时后上层应降级为停车或保守动作。
 */
u8 OpenMV_IsFresh(u32 timeout_ms);

/*
 * 功能: 判断 OpenMV 是否至少收到过一帧通过头尾与校验的正式帧。
 * 入参: 无。
 * 出参: 返回 1 表示已有有效帧，返回 0 表示仍处于首帧前状态。
 * 异常: 无。
 * 边界: 首帧前上层不得把默认零值 openmvdata 当成真实巡线观测。
 */
u8 OpenMV_HasValidFrame(void);

/*
 * 功能: 返回 OpenMV 当前数据状态，区分首帧缺失、鲜活和超时。
 * 入参: timeout_ms 为最大允许静默时间。
 * 出参: OPENMV_STATUS_NO_FRAME/FRESH/STALE。
 * 异常: system_time 异常会影响超时判断。
 * 边界: 首帧缺失优先级高于 stale，便于任务启动阶段做保守降级。
 */
openmv_status_t OpenMV_GetStatus(u32 timeout_ms);
#endif //car_uart1_h_

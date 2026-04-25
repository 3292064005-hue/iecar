#ifndef CAR_DIAG_SHARED_H__
#define CAR_DIAG_SHARED_H__

#ifndef CAR_DIAG_PLATFORM_HEADER
#error "CAR_DIAG_PLATFORM_HEADER must be defined before including car_diag_shared.h"
#endif

#include CAR_DIAG_PLATFORM_HEADER

typedef enum
{
    CAR_DIAG_NONE = 0,
    CAR_DIAG_TASK_STATE = 1,
    CAR_DIAG_TASK_SAFE_STOP = 2,
    CAR_DIAG_TASK_COMPLETE = 3,
    CAR_DIAG_OPENMV_NO_FRAME = 10,
    CAR_DIAG_OPENMV_STALE = 11,
    CAR_DIAG_OPENMV_BAD_FRAME = 12,
    CAR_DIAG_K210_BAD_FRAME = 20,
    CAR_DIAG_K210_UNRELIABLE = 21,
    CAR_DIAG_LINK_RX = 30,
    CAR_DIAG_LINK_DUP = 31,
    CAR_DIAG_LINK_UNAUTH = 32,
    CAR_DIAG_LINK_BAD_FRAME = 33,
    CAR_DIAG_LINK_TX_FAIL = 34,
    CAR_DIAG_LINK_DEGRADED = 35
} car_diag_event_t;

#ifndef CAR_DIAG_FAULT_THROTTLE_MS
#define CAR_DIAG_FAULT_THROTTLE_MS 250u
#endif

typedef struct
{
    long long int time_ms;
    u8 event;
    u8 arg0;
    u8 arg1;
    u8 arg2;
} car_diag_record_t;

extern car_diag_record_t car_diag_ring[CAR_DIAG_RING_SIZE];
extern volatile u8 car_diag_write_index;
extern volatile u16 car_diag_overrun_count;

/*
 * 功能: 记录一次结构化诊断事件，供 OLED 摘要、串口转储或赛后复盘使用。
 * 入参: event 为 car_diag_event_t；arg0~arg2 为调用方定义的轻量参数。
 * 出参: 无，记录写入固定长度环形缓存。
 * 异常: 环形缓存满时覆盖最旧槽位并递增 overrun 计数，不阻塞实时链路。
 * 边界: ISR 和主循环均可调用，但不做锁保护；只用于诊断，不作为业务判定依据。
 */
void CarDiag_Record(u8 event, u8 arg0, u8 arg1, u8 arg2);

/*
 * 功能: 按事件与参数节流记录高频故障，避免无帧/stale/链路降级等循环状态刷爆 ring buffer。
 * 入参: event/arg0~arg2 为诊断内容；min_interval_ms 为同类事件最小记录间隔。
 * 出参: 返回 1 表示写入，返回 0 表示被节流抑制。
 * 异常: 不阻塞，不作为业务判定依据。
 * 边界: 不同 event 或参数变化会立即记录，便于保留状态切换现场。
 */
u8 CarDiag_RecordThrottled(u8 event, u8 arg0, u8 arg1, u8 arg2, u32 min_interval_ms);

/*
 * 功能: 清空诊断环形缓存与覆盖计数。
 * 入参: 无。
 * 出参: 无。
 * 异常: 无。
 * 边界: 建议仅在启动或任务切换前调用，避免清除现场故障线索。
 */
void CarDiag_Clear(void);

/*
 * 功能: 读取最近一次写入前的诊断记录。
 * 入参: out 为输出对象。
 * 出参: 返回 1 表示读取成功，返回 0 表示入参非法或暂无记录。
 * 异常: 无。
 * 边界: 只读最近记录，不消费环形缓存。
 */
u8 CarDiag_GetLatest(car_diag_record_t *out);

#endif /* CAR_DIAG_SHARED_H__ */

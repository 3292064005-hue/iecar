#ifndef CTRL_NUM_SHARED_H__
#define CTRL_NUM_SHARED_H__

#ifndef CAR_CTRLNUM_PLATFORM_HEADER
#error "CAR_CTRLNUM_PLATFORM_HEADER must be defined before including ctrl_num_shared.h"
#endif

#include CAR_CTRLNUM_PLATFORM_HEADER

typedef enum
{
    CAR_LINK_HEALTHY = 0,
    CAR_LINK_DEGRADED = 1,
    CAR_LINK_LOST = 2
} car_link_health_t;

typedef enum
{
    CAR_LINK_TX_IDLE = 0,
    CAR_LINK_TX_PENDING = 1,
    CAR_LINK_TX_SUCCESS = 2,
    CAR_LINK_TX_FAILED = 3
} car_link_tx_state_t;

extern u8 return_sta;
extern u8 sta_num[];
extern u8 sta_i;
extern u8 car_task;
extern u8 u2_send[];
extern u8 leave_flag;
extern u8 go_flag;

extern volatile u8 car_link_last_rx_seq;
extern volatile u8 car_link_last_ack_seq;
extern volatile u8 car_link_last_tx_seq;
extern volatile long long int car_link_last_rx_time;
extern volatile long long int car_link_last_tx_time;
extern volatile long long int car_link_last_ack_time;
extern volatile long long int car_link_last_heartbeat_tx_time;
extern volatile long long int car_link_last_heartbeat_rx_time;
extern volatile u8 car_link_last_rx_transport;
extern volatile car_link_tx_state_t car_link_tx_state;
extern volatile u16 car_link_bad_frame_count;
extern volatile u16 car_link_duplicate_count;
extern volatile u16 car_link_unauthorized_count;
extern volatile u16 car_link_legacy_rx_count;

void U2_DATA_DEAL(void);
void UART2_Send_Str(u8 *s,u8 cnt_s);
/*
 * 功能: 校验并消费一帧 CarLink 数据，支持版本校验、角色矩阵、重复包幂等处理和 ACK。
 * 入参: frame/len 为帧数据和长度；send_ack=1 时对非 ACK 合法帧回 ACK。
 * 出参: 返回 1 表示合法帧已接收或重复帧已幂等确认，返回 0 表示非法/越权帧。
 * 异常: 非法帧会增加 car_link_bad_frame_count；越权帧增加 car_link_unauthorized_count。
 * 边界: 重复 seq 不会二次写业务 flag，但仍会补发 ACK，避免发送端因 ACK 丢失持续重试。
 */
u8 CarLink_ProcessFrame(const u8 *frame, u8 len, u8 send_ack);
u8 CarLink_SendMessage(u8 msg_type, u8 value);
void CarLink_SendAck(u8 seq);
u8 CarLink_IsFresh(u32 timeout_ms);
u8 CarPathStack_Push(u8 step);
u8 CarPathStack_Pop(u8 *step);

/*
 * 功能: 启动一次异步车间命令发送，由后台轮询 ACK/重试直至完成。
 * 入参: msg_type/value 为待发送消息；需先保证当前没有未完成发送。
 * 出参: 返回 1 表示异步发送已受理，返回 0 表示参数非法或已有发送进行中。
 * 异常: 链路异常不会阻塞当前调用，结果由 CarLink_ServiceTx/CarLink_GetTxState 暴露。
 * 边界: 单次只允许一个挂起发送；兼容旧阻塞接口时会由 CarLink_SendMessage 包装调用。
 */
u8 CarLink_SendMessageAsync(u8 msg_type, u8 value);

/*
 * 功能: 推进异步发送状态机，处理 ACK、超时重发和最终失败。
 * 入参: 无。
 * 出参: 无。
 * 异常: 链路超时会把状态切到 CAR_LINK_TX_FAILED，不自动改业务层标志位。
 * 边界: 需要由任务主循环周期性调用；ISR 不直接调用。
 */
void CarLink_ServiceTx(void);

/*
 * 功能: 周期性发送心跳帧，维持无业务负载时的链路活性观测。
 * 入参: 无。
 * 出参: 无。
 * 异常: 若存在挂起异步发送，本函数不会抢占该发送。
 * 边界: 心跳仅在空闲期发送；健康度仍以“最近一次有效接收”为准。
 */
void CarLink_ServiceHeartbeat(void);

/*
 * 功能: 返回当前异步发送状态。
 * 入参: 无。
 * 出参: CAR_LINK_TX_IDLE/PENDING/SUCCESS/FAILED。
 * 异常: 无。
 * 边界: SUCCESS/FAILED 为保持态，需要上层显式清零或重新发起下一次发送。
 */
car_link_tx_state_t CarLink_GetTxState(void);

/*
 * 功能: 清空当前异步发送状态，允许开始下一次发送。
 * 入参: 无。
 * 出参: 无。
 * 异常: 无。
 * 边界: 只清发送状态，不影响最近一次 ACK/接收时间戳。
 */
void CarLink_ClearTxState(void);

/*
 * 功能: 读取当前链路健康级别。
 * 入参: 无。
 * 出参: HEALTHY/DEGRADED/LOST。
 * 异常: 若 system_time 不递增，结果不可信。
 * 边界: 在启用心跳后，结果反映“最近一次有效接收或心跳接收”的活性。
 */
car_link_health_t CarLink_GetHealth(void);

#endif /* CTRL_NUM_SHARED_H__ */

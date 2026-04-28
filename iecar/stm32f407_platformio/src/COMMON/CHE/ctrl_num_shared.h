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

typedef struct
{
    u8 msg_type;
    u8 value;
    u8 seq;
    u8 source_transport;
    long long int time_ms;
} car_link_event_t;

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
extern volatile u16 car_link_rx_queue_overrun_count;
extern volatile u16 car_link_tx_queue_overrun_count;
extern volatile u16 car_link_event_queue_overrun_count;

void U2_DATA_DEAL(void);
void UART2_Send_Str(u8 *s,u8 cnt_s);

/*
 * 功能: 从 USART2 IDLE 中断提交一帧候选 CarLink 数据到接收队列。
 * 入参: frame/len 为 ISR 收到的线缆帧与长度。
 * 出参: 返回 1 表示已入队，返回 0 表示入参非法或队列满。
 * 异常: 队列满时递增 car_link_rx_queue_overrun_count，不在 ISR 内解析业务或发送 ACK。
 * 边界: 本函数只复制固定长度帧，真正协议校验由 CarLink_ServiceRx 在主循环完成。
 */
u8 CarLink_SubmitRxFrameFromIsr(const u8 *frame, u8 len);

/*
 * 功能: 从 ISR 提交一帧候选 CarLink 数据，并显式指定后续是否发送 ACK。
 * 入参: frame/len 为线缆帧；send_ack=1 用于正式 USART2 链路，send_ack=0 用于 USART1 调试兼容入口。
 * 出参: 返回 1 表示入队成功，返回 0 表示参数非法或队列满。
 * 异常: 队列满只递增溢出计数；不在 ISR 内做业务解析或阻塞发送。
 * 边界: 业务处理必须由非中断上下文 CarLink_ServiceRx 完成。
 */
u8 CarLink_SubmitRxFrameFromIsrEx(const u8 *frame, u8 len, u8 send_ack);

/*
 * 功能: 在非中断上下文消费 CarLink 接收队列，完成解析、业务事件投递与 ACK 排队。
 * 入参: 无。
 * 出参: 无。
 * 异常: 坏帧/越权帧会记录诊断；ACK 队列满会记录发送队列溢出计数。
 * 边界: 需要由任务循环或通信 service 周期性调用；ISR 不调用。
 */
void CarLink_ServiceRx(void);

/*
 * 功能: 刷新 CarLink 待发送队列，主要用于把 ACK 从主循环上下文发出。
 * 入参: 无。
 * 出参: 无。
 * 异常: 串口底层仍为阻塞式发送，因此调用方应避免在 ISR 内调用。
 * 边界: 只处理队列帧，不推进业务 ACK/retry 状态机。
 */
void CarLink_ServiceTxQueue(void);

/*
 * 功能: 推进 CarLink 通信服务，按顺序处理 RX 队列、异步发送 ACK/retry、TX 队列。
 * 入参: 无。
 * 出参: 无。
 * 异常: 诊断计数由子服务维护。
 * 边界: 不主动发送心跳，心跳仍由 CarLink_ServiceHeartbeat 控制节奏。
 */
void CarLink_Service(void);

/*
 * 功能: 校验并消费一帧 CarLink 数据，支持版本校验、角色矩阵、重复包幂等处理和 ACK。
 * 入参: frame/len 为帧数据和长度；send_ack=1 时对非 ACK 合法帧排队 ACK。
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
 * 功能: 读取并消费一条 CarLink 业务事件。
 * 入参: out 为输出对象。
 * 出参: 返回 1 表示读取成功，返回 0 表示无事件或入参非法。
 * 异常: 事件队列满时覆盖会被拒绝并增加 car_link_event_queue_overrun_count。
 * 边界: 为兼容旧任务，事件投递同时镜像到 leave_flag/go_flag/detect_num。
 */
u8 CarLink_TakeEvent(car_link_event_t *out);

/*
 * 功能: 消费指定类型的业务事件，供任务层替代直接轮询全局 flag。
 * 入参: msg_type 为目标消息类型；value 可为空。
 * 出参: 返回 1 表示找到并消费，返回 0 表示未找到。
 * 异常: 不阻塞，不修改其他类型事件。
 * 边界: 保持事件顺序扫描；当前实现为轻量队列，不支持按 seq 回滚。
 */
u8 CarLink_ConsumeEventType(u8 msg_type, u8 *value);

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

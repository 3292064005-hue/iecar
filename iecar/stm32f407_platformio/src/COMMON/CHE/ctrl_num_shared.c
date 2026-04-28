#include "ctrl_num.h"
#include "car_mainchain.h"
#include "protocol_core_shared.h"

u8 return_sta=0;
u8 car_task=0;
u8 leave_flag=0;
u8 go_flag=0;
u8 sta_num[CAR_PATH_STACK_CAPACITY]={0};
u8 sta_i=CAR_PATH_STACK_INIT_DEPTH;

u8 CarPathStack_Push(u8 step)
{
    if(sta_i >= CAR_PATH_STACK_CAPACITY)
    {
        return 0;
    }
    sta_num[sta_i++] = step;
    return 1;
}

u8 CarPathStack_Pop(u8 *step)
{
    if(step == 0 || sta_i == 0)
    {
        return 0;
    }
    *step = sta_num[--sta_i];
    return 1;
}

u8 u2_send[CAR_LINK_FRAME_SIZE]={0};
volatile u8 car_link_last_rx_seq=0;
volatile u8 car_link_last_ack_seq=0;
volatile u8 car_link_last_tx_seq=0;
volatile long long int car_link_last_rx_time=0;
volatile long long int car_link_last_tx_time=0;
volatile long long int car_link_last_ack_time=0;
volatile long long int car_link_last_heartbeat_tx_time=0;
volatile long long int car_link_last_heartbeat_rx_time=0;
volatile u8 car_link_last_rx_transport=0;
volatile car_link_tx_state_t car_link_tx_state = CAR_LINK_TX_IDLE;
volatile u16 car_link_bad_frame_count = 0;
volatile u16 car_link_duplicate_count = 0;
volatile u16 car_link_unauthorized_count = 0;
volatile u16 car_link_legacy_rx_count = 0;
volatile u16 car_link_rx_queue_overrun_count = 0;
volatile u16 car_link_tx_queue_overrun_count = 0;
volatile u16 car_link_event_queue_overrun_count = 0;

static u8 car_link_pending_type = 0;
static u8 car_link_pending_value = 0;
static u8 car_link_pending_seq = 0;
static u8 car_link_pending_retry = 0;
static long long int car_link_pending_deadline_ms = 0;
static u8 car_link_last_rx_type = 0;
static u8 car_link_last_rx_value = 0;

static u8 car_link_rx_queue[CAR_LINK_RX_QUEUE_DEPTH][CAR_LINK_FRAME_SIZE];
static u8 car_link_rx_len_queue[CAR_LINK_RX_QUEUE_DEPTH];
static u8 car_link_rx_send_ack_queue[CAR_LINK_RX_QUEUE_DEPTH];
static volatile u8 car_link_rx_head = 0u;
static volatile u8 car_link_rx_tail = 0u;
static volatile u8 car_link_rx_count = 0u;

static u8 car_link_tx_queue[CAR_LINK_TX_QUEUE_DEPTH][CAR_LINK_FRAME_SIZE];
static volatile u8 car_link_tx_head = 0u;
static volatile u8 car_link_tx_tail = 0u;
static volatile u8 car_link_tx_count = 0u;

static car_link_event_t car_link_event_queue[CAR_LINK_EVENT_QUEUE_DEPTH];
static volatile u8 car_link_event_head = 0u;
static volatile u8 car_link_event_tail = 0u;
static volatile u8 car_link_event_count = 0u;
static car_protocol_stream_parser_t car_link_rx_stream;

/*
 * 功能: 保护 USART ISR 生产、主循环消费的 RX ring 元数据。
 * 入参/出参: 返回进入临界区前的 PRIMASK，中断状态由调用方传回恢复。
 * 异常: 仅屏蔽极短临界区，避免 head/tail/count 多变量更新被中断打断。
 * 边界: 使用保存/恢复语义，避免在 ISR 或外层临界区内无条件重新打开中断。
 */
static u32 car_link_irq_save(void)
{
    u32 primask = __get_PRIMASK();
    INTX_DISABLE();
    return primask;
}

static void car_link_irq_restore(u32 primask)
{
    __set_PRIMASK(primask);
}

static u8 car_link_expected_peer_role(void)
{
    if(CAR_ROLE_ID == 1u)
    {
        return 2u;
    }
    if(CAR_ROLE_ID == 2u)
    {
        return 1u;
    }
    return 0u;
}

static u8 car_link_checksum_bytes(const u8 *frame)
{
    return CarProtocol_CarLinkChecksum(frame);
}

static void car_link_fill_frame(u8 *frame, u8 msg_type, u8 value, u8 seq)
{
    frame[0] = CAR_LINK_MAGIC0;
    frame[1] = CAR_LINK_MAGIC1;
    frame[2] = msg_type;
    frame[3] = value;
    frame[4] = seq;
    frame[5] = (CAR_LINK_FIELD5_IS_ROLE != 0u) ? CAR_ROLE_ID : car_task;
    frame[6] = CAR_PROTOCOL_VERSION_MAJOR;
    frame[7] = CAR_PROTOCOL_VERSION_MINOR;
    frame[8] = car_link_checksum_bytes(frame);
    frame[9] = 0;
    frame[10] = CAR_LINK_TAIL;
}

static u8 car_link_frame_is_legacy(const u8 *frame)
{
    return CarProtocol_CarLinkFrameIsLegacy(frame);
}

static u8 car_link_frame_is_valid(const u8 *frame, u8 len)
{
    return CarProtocol_CarLinkFrameIsValid(frame, len);
}

static u8 car_link_sender_role_is_valid(u8 sender_role, u8 legacy)
{
    u8 expected_peer = car_link_expected_peer_role();
    if(legacy || CAR_LINK_STRICT_ROLE_MATRIX == 0u)
    {
        return 1u;
    }
    if(expected_peer == 0u)
    {
        return 0u;
    }
    return (u8)(sender_role == expected_peer);
}

static u8 car_link_message_is_allowed(u8 msg_type, u8 sender_role)
{
    if(msg_type == CAR_LINK_MSG_ACK || msg_type == CAR_LINK_MSG_HEARTBEAT)
    {
        return 1u;
    }
    if(CAR_LINK_STRICT_ROLE_MATRIX == 0u)
    {
        return 1u;
    }
    if(CAR_ROLE_ID == 2u && sender_role == 1u)
    {
        return (u8)(msg_type == CAR_LINK_MSG_SET_DETECT || msg_type == CAR_LINK_MSG_LEAVE || msg_type == CAR_LINK_MSG_GO);
    }
    return 0u;
}

static u8 car_link_is_duplicate(u8 msg_type, u8 value, u8 seq, u8 legacy)
{
    if(legacy || seq == 0u)
    {
        return 0u;
    }
    return (u8)(seq == car_link_last_rx_seq && msg_type == car_link_last_rx_type && value == car_link_last_rx_value);
}

static u8 car_link_push_tx_frame(const u8 *frame)
{
    u8 i;
    if(frame == 0)
    {
        return 0u;
    }
    if(car_link_tx_count >= CAR_LINK_TX_QUEUE_DEPTH)
    {
        car_link_tx_queue_overrun_count++;
        CarDiag_Record(CAR_DIAG_LINK_TX_FAIL, CAR_LINK_MSG_ACK, 0u, 0u);
        return 0u;
    }
    for(i = 0u; i < CAR_LINK_FRAME_SIZE; ++i)
    {
        car_link_tx_queue[car_link_tx_tail][i] = frame[i];
    }
    car_link_tx_tail++;
    if(car_link_tx_tail >= CAR_LINK_TX_QUEUE_DEPTH)
    {
        car_link_tx_tail = 0u;
    }
    car_link_tx_count++;
    return 1u;
}

static u8 car_link_pop_tx_frame(u8 *frame)
{
    u8 i;
    if(frame == 0 || car_link_tx_count == 0u)
    {
        return 0u;
    }
    for(i = 0u; i < CAR_LINK_FRAME_SIZE; ++i)
    {
        frame[i] = car_link_tx_queue[car_link_tx_head][i];
    }
    car_link_tx_head++;
    if(car_link_tx_head >= CAR_LINK_TX_QUEUE_DEPTH)
    {
        car_link_tx_head = 0u;
    }
    car_link_tx_count--;
    return 1u;
}

static void car_link_send_frame_now(u8 msg_type, u8 value, u8 seq)
{
    u8 frame[CAR_LINK_FRAME_SIZE];
    car_link_fill_frame(frame, msg_type, value, seq);
    car_link_last_tx_time = system_time;
    UART2_Send_Str(frame, CAR_LINK_FRAME_SIZE);
}

static void car_link_queue_frame(u8 msg_type, u8 value, u8 seq)
{
    u8 frame[CAR_LINK_FRAME_SIZE];
    car_link_fill_frame(frame, msg_type, value, seq);
    (void)car_link_push_tx_frame(frame);
}

static u8 car_link_push_event(u8 msg_type, u8 value, u8 seq, u8 source_transport)
{
    car_link_event_t event;
    if(car_link_event_count >= CAR_LINK_EVENT_QUEUE_DEPTH)
    {
        car_link_event_queue_overrun_count++;
        CarDiag_Record(CAR_DIAG_LINK_BAD_FRAME, msg_type, value, seq);
        return 0u;
    }
    event.msg_type = msg_type;
    event.value = value;
    event.seq = seq;
    event.source_transport = source_transport;
    event.time_ms = system_time;
    car_link_event_queue[car_link_event_tail] = event;
    car_link_event_tail++;
    if(car_link_event_tail >= CAR_LINK_EVENT_QUEUE_DEPTH)
    {
        car_link_event_tail = 0u;
    }
    car_link_event_count++;
    return 1u;
}

static void car_link_apply_payload(u8 msg_type, u8 value, u8 seq, u8 source_transport)
{
    if(msg_type == CAR_LINK_MSG_SET_DETECT)
    {
        if(car_task == 2 && Strategy_IsValidWardTarget(value))
        {
            detect_num = value;
            (void)car_link_push_event(msg_type, value, seq, source_transport);
        }
    }
    else if(msg_type == CAR_LINK_MSG_LEAVE)
    {
        leave_flag = 1;
        (void)car_link_push_event(msg_type, value, seq, source_transport);
    }
    else if(msg_type == CAR_LINK_MSG_GO)
    {
        go_flag = 1;
        (void)car_link_push_event(msg_type, value, seq, source_transport);
    }
    else if(msg_type == CAR_LINK_MSG_HEARTBEAT)
    {
        car_link_last_heartbeat_rx_time = system_time;
        (void)car_link_push_event(msg_type, value, seq, source_transport);
    }
}

static u8 car_link_pop_rx_frame(u8 *frame, u8 *len, u8 *send_ack)
{
    u8 i;
    u8 local_head;
    if(frame == 0 || len == 0 || send_ack == 0)
    {
        return 0u;
    }
    u32 irq_state = car_link_irq_save();
    if(car_link_rx_count == 0u)
    {
        car_link_irq_restore(irq_state);
        return 0u;
    }
    local_head = car_link_rx_head;
    *len = car_link_rx_len_queue[local_head];
    *send_ack = car_link_rx_send_ack_queue[local_head];
    for(i = 0u; i < CAR_LINK_FRAME_SIZE; ++i)
    {
        frame[i] = car_link_rx_queue[local_head][i];
    }
    car_link_rx_head++;
    if(car_link_rx_head >= CAR_LINK_RX_QUEUE_DEPTH)
    {
        car_link_rx_head = 0u;
    }
    car_link_rx_count--;
    car_link_irq_restore(irq_state);
    return 1u;
}

void U2_DATA_DEAL(void)
{
    u8 i;
    u8 frame[CAR_LINK_FRAME_SIZE];
    u8 len = USART2_RX_BUF[0];
    for(i = 0u; i < len; ++i)
    {
        if(CarProtocol_StreamFeedCarLinkByte(&car_link_rx_stream,
                                             USART2_RX_BUF[(u8)(i + 1u)],
                                             frame))
        {
            (void)CarLink_SubmitRxFrameFromIsr(frame, CAR_LINK_FRAME_SIZE);
        }
    }
}

u8 CarLink_SubmitRxFrameFromIsrEx(const u8 *frame, u8 len, u8 send_ack)
{
    u8 i;
    u8 local_tail;
    u8 accepted = 0u;
    if(frame == 0 || len != CAR_LINK_FRAME_SIZE)
    {
        car_link_bad_frame_count++;
        return 0u;
    }
    u32 irq_state = car_link_irq_save();
    if(car_link_rx_count < CAR_LINK_RX_QUEUE_DEPTH)
    {
        local_tail = car_link_rx_tail;
        for(i = 0u; i < CAR_LINK_FRAME_SIZE; ++i)
        {
            car_link_rx_queue[local_tail][i] = frame[i];
        }
        car_link_rx_len_queue[local_tail] = len;
        car_link_rx_send_ack_queue[local_tail] = (send_ack != 0u) ? 1u : 0u;
        car_link_rx_tail++;
        if(car_link_rx_tail >= CAR_LINK_RX_QUEUE_DEPTH)
        {
            car_link_rx_tail = 0u;
        }
        car_link_rx_count++;
        accepted = 1u;
    }
    else
    {
        car_link_rx_queue_overrun_count++;
    }
    car_link_irq_restore(irq_state);
    return accepted;
}

u8 CarLink_SubmitRxFrameFromIsr(const u8 *frame, u8 len)
{
    return CarLink_SubmitRxFrameFromIsrEx(frame, len, 1u);
}

void CarLink_ServiceRx(void)
{
    u8 frame[CAR_LINK_FRAME_SIZE];
    u8 len = 0u;
    u8 send_ack = 0u;
    while(car_link_pop_rx_frame(frame, &len, &send_ack))
    {
        (void)CarLink_ProcessFrame(frame, len, send_ack);
    }
    CarLink_ServiceTxQueue();
}

void CarLink_ServiceTxQueue(void)
{
    u8 frame[CAR_LINK_FRAME_SIZE];
    while(car_link_pop_tx_frame(frame))
    {
        car_link_last_tx_time = system_time;
        UART2_Send_Str(frame, CAR_LINK_FRAME_SIZE);
    }
}

void CarLink_Service(void)
{
    CarLink_ServiceRx();
    CarLink_ServiceTx();
    CarLink_ServiceTxQueue();
}

u8 CarLink_ProcessFrame(const u8 *frame, u8 len, u8 send_ack)
{
    u8 msg_type;
    u8 value;
    u8 seq;
    u8 legacy;
    u8 sender_role;
    u8 duplicate;
    u8 transport = send_ack ? 2u : 1u;

    if(frame == 0 || !car_link_frame_is_valid(frame, len))
    {
        car_link_bad_frame_count++;
        CarDiag_Record(CAR_DIAG_LINK_BAD_FRAME, len, 0u, 0u);
        return 0;
    }

    legacy = car_link_frame_is_legacy(frame);
    if(legacy)
    {
        car_link_legacy_rx_count++;
    }
    msg_type = frame[2];
    value = frame[3];
    seq = legacy ? 0 : frame[4];
    sender_role = CarProtocol_NormalizeCarLinkSenderRole(frame, legacy, CAR_ROLE_ID, car_task);

    if(!car_link_sender_role_is_valid(sender_role, legacy) || !car_link_message_is_allowed(msg_type, sender_role))
    {
        car_link_unauthorized_count++;
        CarDiag_Record(CAR_DIAG_LINK_UNAUTH, msg_type, sender_role, CAR_ROLE_ID);
        return 0;
    }

    duplicate = car_link_is_duplicate(msg_type, value, seq, legacy);

    car_link_last_rx_time = system_time;
    car_link_last_rx_transport = transport;

    if(msg_type == CAR_LINK_MSG_ACK)
    {
        car_link_last_rx_seq = seq;
        car_link_last_rx_type = msg_type;
        car_link_last_rx_value = value;
        car_link_last_ack_seq = value;
        car_link_last_ack_time = system_time;
        if(car_link_tx_state == CAR_LINK_TX_PENDING && value == car_link_pending_seq)
        {
            car_link_tx_state = CAR_LINK_TX_SUCCESS;
        }
        CarDiag_Record(CAR_DIAG_LINK_RX, msg_type, value, seq);
        return 1;
    }

    if(duplicate)
    {
        car_link_duplicate_count++;
        CarDiag_Record(CAR_DIAG_LINK_DUP, msg_type, value, seq);
        if(send_ack)
        {
            CarLink_SendAck(seq);
        }
        return 1;
    }

    car_link_last_rx_seq = seq;
    car_link_last_rx_type = msg_type;
    car_link_last_rx_value = value;
    car_link_apply_payload(msg_type, value, seq, transport);
    CarDiag_Record(CAR_DIAG_LINK_RX, msg_type, value, seq);
    if(send_ack)
    {
        CarLink_SendAck(seq);
    }
    return 1;
}

void CarLink_SendAck(u8 seq)
{
    car_link_queue_frame(CAR_LINK_MSG_ACK, seq, seq);
}

u8 CarLink_TakeEvent(car_link_event_t *out)
{
    if(out == 0 || car_link_event_count == 0u)
    {
        return 0u;
    }
    *out = car_link_event_queue[car_link_event_head];
    car_link_event_head++;
    if(car_link_event_head >= CAR_LINK_EVENT_QUEUE_DEPTH)
    {
        car_link_event_head = 0u;
    }
    car_link_event_count--;
    return 1u;
}

u8 CarLink_ConsumeEventType(u8 msg_type, u8 *value)
{
    u8 i;
    u8 count = car_link_event_count;
    for(i = 0u; i < count; ++i)
    {
        car_link_event_t event;
        if(!CarLink_TakeEvent(&event))
        {
            return 0u;
        }
        if(event.msg_type == msg_type)
        {
            if(value != 0)
            {
                *value = event.value;
            }
            return 1u;
        }
        if(car_link_event_count < CAR_LINK_EVENT_QUEUE_DEPTH)
        {
            car_link_event_queue[car_link_event_tail] = event;
            car_link_event_tail++;
            if(car_link_event_tail >= CAR_LINK_EVENT_QUEUE_DEPTH)
            {
                car_link_event_tail = 0u;
            }
            car_link_event_count++;
        }
        else
        {
            car_link_event_queue_overrun_count++;
            return 0u;
        }
    }
    return 0u;
}

u8 CarLink_SendMessageAsync(u8 msg_type, u8 value)
{
    if(car_link_tx_state == CAR_LINK_TX_PENDING)
    {
        return 0;
    }
    if(msg_type != CAR_LINK_MSG_SET_DETECT && msg_type != CAR_LINK_MSG_LEAVE && msg_type != CAR_LINK_MSG_GO)
    {
        return 0;
    }
    if(msg_type == CAR_LINK_MSG_SET_DETECT && !Strategy_IsValidWardTarget(value))
    {
        return 0;
    }
    car_link_pending_type = msg_type;
    car_link_pending_value = value;
    car_link_pending_seq = (u8)(car_link_last_tx_seq + 1u);
    car_link_last_tx_seq = car_link_pending_seq;
    car_link_pending_retry = 0u;
    car_link_tx_state = CAR_LINK_TX_PENDING;
    car_link_pending_deadline_ms = system_time + CAR_LINK_ACK_TIMEOUT_MS;
    car_link_send_frame_now(car_link_pending_type, car_link_pending_value, car_link_pending_seq);
    return 1;
}

void CarLink_ServiceTx(void)
{
    if(car_link_tx_state != CAR_LINK_TX_PENDING)
    {
        return;
    }
    if(car_link_last_ack_seq == car_link_pending_seq)
    {
        car_link_last_ack_time = system_time;
        car_link_tx_state = CAR_LINK_TX_SUCCESS;
        return;
    }
    if((u32)(system_time - car_link_pending_deadline_ms) <= 0x7fffffffu)
    {
        if(car_link_pending_retry + 1u >= CAR_LINK_RETRY_COUNT)
        {
            car_link_tx_state = CAR_LINK_TX_FAILED;
            CarDiag_Record(CAR_DIAG_LINK_TX_FAIL, car_link_pending_type, car_link_pending_value, car_link_pending_seq);
            return;
        }
        car_link_pending_retry++;
        car_link_pending_deadline_ms = system_time + CAR_LINK_ACK_TIMEOUT_MS + CAR_LINK_RETRY_INTERVAL_MS;
        car_link_send_frame_now(car_link_pending_type, car_link_pending_value, car_link_pending_seq);
    }
}

void CarLink_ServiceHeartbeat(void)
{
    u32 elapsed_since_tx;
    u32 elapsed_since_hb;
    CarLink_ServiceRx();
    if(system_time < CAR_LINK_HEARTBEAT_STARTUP_GRACE_MS)
    {
        return;
    }
    if(car_link_tx_state == CAR_LINK_TX_PENDING)
    {
        return;
    }
    elapsed_since_tx = (u32)(system_time - car_link_last_tx_time);
    elapsed_since_hb = (u32)(system_time - car_link_last_heartbeat_tx_time);
    if(elapsed_since_tx < CAR_LINK_HEARTBEAT_INTERVAL_MS || elapsed_since_hb < CAR_LINK_HEARTBEAT_INTERVAL_MS)
    {
        return;
    }
    car_link_last_tx_seq = (u8)(car_link_last_tx_seq + 1u);
    car_link_last_heartbeat_tx_time = system_time;
    car_link_send_frame_now(CAR_LINK_MSG_HEARTBEAT, CAR_ROLE_ID, car_link_last_tx_seq);
}

car_link_tx_state_t CarLink_GetTxState(void)
{
    return car_link_tx_state;
}

void CarLink_ClearTxState(void)
{
    if(car_link_tx_state != CAR_LINK_TX_PENDING)
    {
        car_link_tx_state = CAR_LINK_TX_IDLE;
    }
}

u8 CarLink_SendMessage(u8 msg_type, u8 value)
{
    if(!CarLink_SendMessageAsync(msg_type, value))
    {
        return 0;
    }
    while(CarLink_GetTxState() == CAR_LINK_TX_PENDING)
    {
        CarLink_ServiceRx();
        CarLink_ServiceTx();
        CarLink_ServiceTxQueue();
        delay_ms(1);
    }
    if(CarLink_GetTxState() == CAR_LINK_TX_SUCCESS)
    {
        CarLink_ClearTxState();
        return 1;
    }
    CarLink_ClearTxState();
    return 0;
}

u8 CarLink_IsFresh(u32 timeout_ms)
{
    return (u8)(car_link_last_rx_time != 0 && (u32)(system_time - car_link_last_rx_time) <= timeout_ms);
}

car_link_health_t CarLink_GetHealth(void)
{
    u32 idle_ms;
    CarLink_ServiceRx();
    if(car_link_last_rx_time == 0)
    {
        if(system_time <= CAR_LINK_HEARTBEAT_STARTUP_GRACE_MS)
        {
            return CAR_LINK_HEALTHY;
        }
        return CAR_LINK_DEGRADED;
    }
    idle_ms = (u32)(system_time - car_link_last_rx_time);
    if(idle_ms <= CAR_LINK_FRESHNESS_MS)
    {
        return CAR_LINK_HEALTHY;
    }
    if(idle_ms <= CAR_LINK_LOST_TIMEOUT_MS)
    {
        CarDiag_RecordThrottled(CAR_DIAG_LINK_DEGRADED, (u8)(idle_ms & 0xFFu), (u8)(idle_ms > CAR_LINK_DEGRADED_TIMEOUT_MS), 0u, CAR_DIAG_FAULT_THROTTLE_MS);
        return CAR_LINK_DEGRADED;
    }
    CarDiag_RecordThrottled(CAR_DIAG_LINK_DEGRADED, (u8)(idle_ms & 0xFFu), 2u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
    return CAR_LINK_LOST;
}

void UART2_Send_Str(u8 *s,u8 cnt_s)
{
    u8 i;
    for(i=0;i<cnt_s;i++)
    {
        USART_SendData(USART2,s[i]);
        while( USART_GetFlagStatus(USART2,USART_FLAG_TXE)!= SET);
    }
}

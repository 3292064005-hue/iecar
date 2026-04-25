#include "protocol_core_shared.h"
int aaa,bbb;
int left_max,right_max;
u8 find_val1,find_val2,find_val3,find_val4;
int val1_cnt,val2_cnt,val3_cnt,val4_cnt;
u8 k210_left_conf=0;
u8 k210_right_conf=0;
u16 k210_left_x=0;
u16 k210_left_y=0;
u16 k210_right_x=0;
u16 k210_right_y=0;
u8 k210_left_seq=0;
u8 k210_right_seq=0;
volatile long long int k210_left_last_rx_time = 0;
volatile long long int k210_right_last_rx_time = 0;
volatile u8 k210_left_has_valid_frame = 0;
volatile u8 k210_right_has_valid_frame = 0;
volatile u16 k210_bad_frame_count = 0;

static u8 k210_observation_mask_from_array(const u8 *target)
{
    u8 i;
    u8 mask = 0;
    for(i = 1; i <= CAR_K210_CLASS_COUNT; ++i)
    {
        if(target[i])
        {
            mask |= (u8)(1u << (i - 1u));
        }
    }
    return mask;
}

static u8 k210_position_is_valid(u16 x, u16 y)
{
    if(CAR_K210_POSITION_REQUIRED == 0u)
    {
        return 1u;
    }
    if(x < CAR_K210_POSITION_MIN_X || x > CAR_K210_POSITION_MAX_X)
    {
        return 0u;
    }
    if(y < CAR_K210_POSITION_MIN_Y || y > CAR_K210_POSITION_MAX_Y)
    {
        return 0u;
    }
    return 1u;
}

static u8 k210_mask_passes_contract(u8 mask, u8 conf, u16 x, u16 y)
{
    if(mask == 0u)
    {
        return 0u;
    }
    if(conf < CAR_K210_CONFIDENCE_MIN)
    {
        return 0u;
    }
    return k210_position_is_valid(x, y);
}

static void k210_reset_observation(u8 *target)
{
    u8 i;
    for(i = 0; i <= CAR_K210_CLASS_COUNT; ++i)
    {
        target[i] = 0;
    }
}

static void k210_apply_mask(u8 *target, u8 mask)
{
    u8 i;
    for(i = 1; i <= CAR_K210_CLASS_COUNT; ++i)
    {
        target[i] = (u8)((mask >> (i - 1u)) & 0x01u);
    }
}

static void k210_reset_counter_array(int *target)
{
    u8 i;
    for(i = 1; i <= CAR_K210_CLASS_COUNT; ++i)
    {
        target[i] = 0;
    }
}

void K210_ResetVoteBuffers(void)
{
    k210_reset_counter_array(k210_cnt_l);
    k210_reset_counter_array(k210_cnt_r);
}

static void k210_process_frame(volatile u8 *buf,
                               u8 *target,
                               int *counter,
                               u8 *conf,
                               u16 *x,
                               u16 *y,
                               u8 *seq,
                               volatile long long int *rx_time,
                               volatile u8 *has_valid_frame)
{
    car_protocol_k210_frame_t parsed = CarProtocol_ParseK210RxBuffer(buf);
    u8 mask;
    if(!parsed.valid)
    {
        k210_reset_observation(target);
        k210_reset_counter_array(counter);
        *conf = 0;
        *x = 0;
        *y = 0;
        *seq = 0;
        if(has_valid_frame)
        {
            *has_valid_frame = 0u;
        }
        k210_bad_frame_count++;
        CarDiag_RecordThrottled(CAR_DIAG_K210_BAD_FRAME, buf ? buf[0] : 0u, 0u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
        return;
    }

    mask = parsed.mask;
    *conf = parsed.confidence;
    *x = parsed.x;
    *y = parsed.y;
    *seq = parsed.seq;
    *rx_time = system_time;
    if(has_valid_frame)
    {
        *has_valid_frame = 1u;
    }

    if(!k210_mask_passes_contract(mask, *conf, *x, *y))
    {
        mask = 0u;
        k210_reset_counter_array(counter);
    }
    k210_apply_mask(target, mask);

    if(count_go == 1 && mask != 0u)
    {
        u8 i;
        for(i = 1; i <= CAR_K210_CLASS_COUNT; ++i)
        {
            if(target[i] == 1)
            {
                counter[i]++;
            }
        }
    }
    else if(count_go != 1)
    {
        k210_reset_counter_array(counter);
    }
}

u8 USART6_RX_BUF[CAR_K210_RX_BUF_SIZE]={0};
void USART6_IRQHandler(void)
{
    uint8_t res;
    uint8_t clear = 0;
    static uint8_t Rx_Sta = 1;
    static uint8_t Rx_Overflow = 0;
    if(USART_GetITStatus(USART6, USART_IT_RXNE) != RESET)
    {
        res =USART6->DR;
        if(Rx_Overflow == 0)
        {
            if(Rx_Sta < sizeof(USART6_RX_BUF))
            {
                USART6_RX_BUF[Rx_Sta++] = res;
            }
            else
            {
                Rx_Overflow = 1;
                Rx_Sta = 1;
                USART6_RX_BUF[0] = 0;
            }
        }
    }
    else if(USART_GetITStatus(USART6, USART_IT_IDLE) != RESET)
    {
        clear = USART6->SR;
        clear = USART6->DR;
        if(Rx_Overflow == 0)
        {
            USART6_RX_BUF[0] = Rx_Sta - 1;
            bbb++;
            ZSP_DEAL2();
        }
        else
        {
            USART6_RX_BUF[0] = 0;
            Rx_Overflow = 0;
        }
        Rx_Sta = 1;
    }
}

u8 USART5_RX_BUF[CAR_K210_RX_BUF_SIZE]={0};
void UART5_IRQHandler(void)
{
    uint8_t res;
    uint8_t clear = 0;
    static uint8_t Rx_Sta = 1;
    static uint8_t Rx_Overflow = 0;
    if(USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)
    {
        res =UART5->DR;
        if(Rx_Overflow == 0)
        {
            if(Rx_Sta < sizeof(USART5_RX_BUF))
            {
                USART5_RX_BUF[Rx_Sta++] = res;
            }
            else
            {
                Rx_Overflow = 1;
                Rx_Sta = 1;
                USART5_RX_BUF[0] = 0;
            }
        }
    }
    else if(USART_GetITStatus(UART5, USART_IT_IDLE) != RESET)
    {
        clear = UART5->SR;
        clear = UART5->DR;
        if(Rx_Overflow == 0)
        {
            USART5_RX_BUF[0] = Rx_Sta - 1;
            aaa++;
            ZSP_DEAL1();
        }
        else
        {
            USART5_RX_BUF[0] = 0;
            Rx_Overflow = 0;
        }
        Rx_Sta = 1;
    }
}

u8 k210_left[10]={0};
u8 k210_right[10]={0};
u8 count_go=0;
int k210_cnt_l[9]={0};
int k210_cnt_r[9]={0};

void ZSP_DEAL1(void)
{
    k210_process_frame(USART5_RX_BUF, k210_left, k210_cnt_l, &k210_left_conf, &k210_left_x, &k210_left_y, &k210_left_seq, &k210_left_last_rx_time, &k210_left_has_valid_frame);
}

void ZSP_DEAL2(void)
{
    k210_process_frame(USART6_RX_BUF, k210_right, k210_cnt_r, &k210_right_conf, &k210_right_x, &k210_right_y, &k210_right_seq, &k210_right_last_rx_time, &k210_right_has_valid_frame);
}

int find_max(u8 i)
{
    int best_count = 0;
    int best_idx = 0;
    u8 t;
    int *src = (i == K210_CAMERA_LEFT) ? k210_cnt_l : k210_cnt_r;
    for(t = 1; t <= CAR_K210_CLASS_COUNT; ++t)
    {
        if(src[t] > best_count)
        {
            best_count = src[t];
            best_idx = t;
        }
    }
    return best_idx;
}

void find_MAX(u8 a)
{
    int i;
    int max = 0;
    int second_max = 0;
    u8 first_idx = 0;
    u8 second_idx = 0;
    int *src = (a == K210_CAMERA_LEFT) ? k210_cnt_l : k210_cnt_r;

    for(i = 1; i <= CAR_K210_CLASS_COUNT; ++i)
    {
        if(src[i] > max)
        {
            second_max = max;
            second_idx = first_idx;
            max = src[i];
            first_idx = (u8)i;
        }
        else if(src[i] > second_max)
        {
            second_max = src[i];
            second_idx = (u8)i;
        }
    }

    if(a == K210_CAMERA_LEFT)
    {
        find_val1 = first_idx;
        find_val2 = second_idx;
        val1_cnt = max;
        val2_cnt = second_max;
    }
    else if(a == K210_CAMERA_RIGHT)
    {
        find_val3 = first_idx;
        find_val4 = second_idx;
        val3_cnt = max;
        val4_cnt = second_max;
    }
}

u8 K210_GetObservation(u8 camera_id, k210_observation_t *out)
{
    const u8 *target;
    int *counter;
    volatile long long int *rx_time;
    volatile u8 *has_valid_frame;
    u8 conf;
    u16 x;
    u16 y;
    u8 seq;
    if(out == 0)
    {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->camera_id = camera_id;

    if(camera_id == K210_CAMERA_LEFT)
    {
        target = k210_left;
        counter = k210_cnt_l;
        rx_time = &k210_left_last_rx_time;
        conf = k210_left_conf;
        x = k210_left_x;
        y = k210_left_y;
        seq = k210_left_seq;
        has_valid_frame = &k210_left_has_valid_frame;
    }
    else if(camera_id == K210_CAMERA_RIGHT)
    {
        target = k210_right;
        counter = k210_cnt_r;
        rx_time = &k210_right_last_rx_time;
        conf = k210_right_conf;
        x = k210_right_x;
        y = k210_right_y;
        seq = k210_right_seq;
        has_valid_frame = &k210_right_has_valid_frame;
    }
    else
    {
        return 0;
    }

    out->valid_mask = k210_observation_mask_from_array(target);
    out->best_target = (u8)find_max(camera_id);
    if(out->best_target >= 1u && out->best_target <= CAR_K210_CLASS_COUNT)
    {
        out->stable_votes = counter[out->best_target];
    }
    out->confidence = conf;
    out->x = x;
    out->y = y;
    out->seq = seq;
    out->rx_time_ms = *rx_time;
    out->has_valid_frame = (u8)(has_valid_frame != 0 && *has_valid_frame != 0u);
    out->fresh = (u8)(out->has_valid_frame && ((u32)(system_time - *rx_time) <= CAR_K210_FRESHNESS_MS));
    out->confidence_ok = (u8)(conf >= CAR_K210_CONFIDENCE_MIN);
    out->position_ok = k210_position_is_valid(x, y);
    out->reliable = (u8)(out->fresh && out->confidence_ok && out->position_ok && out->valid_mask != 0u);
    if(!out->reliable)
    {
        CarDiag_RecordThrottled(CAR_DIAG_K210_UNRELIABLE, camera_id, out->confidence, out->best_target, CAR_DIAG_FAULT_THROTTLE_MS);
    }
    return 1;
}

u8 K210_ObservationIsReliable(u8 camera_id)
{
    k210_observation_t obs;
    if(!K210_GetObservation(camera_id, &obs))
    {
        return 0u;
    }
    return obs.reliable;
}

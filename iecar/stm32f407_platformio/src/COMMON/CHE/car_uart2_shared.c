#include "car_uart2.h"
#include "car_mainchain.h"
#include "protocol_core_shared.h"
volatile long long int openmv_last_rx_time = 0;
volatile u8 openmv_has_valid_frame = 0;
volatile u16 openmv_bad_frame_count = 0;
#if CAR_OPENMV_USART3_ENABLED
static car_protocol_stream_parser_t openmv_usart3_stream;
#endif
static car_protocol_stream_parser_t openmv_uart4_stream;

static void openmv_copy_wire_frame_to_rxbuf(volatile u8 *buf, const u8 *wire)
{
    u8 i;
    if(buf == 0 || wire == 0)
    {
        return;
    }
    buf[0] = CAR_OPENMV_FRAME_SIZE;
    for(i = 0u; i < CAR_OPENMV_FRAME_SIZE; ++i)
    {
        buf[(u8)(i + 1u)] = wire[i];
    }
}

static void openmv_apply_frame(volatile u8 *buf)
{
    car_protocol_openmv_frame_t parsed = CarProtocol_ParseOpenMVRxBuffer(buf);
    if(!parsed.valid)
    {
        openmv_bad_frame_count++;
        CarDiag_RecordThrottled(CAR_DIAG_OPENMV_BAD_FRAME, buf ? buf[0] : 0u, 0u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
        return;
    }

    openmvdata[0] = parsed.center_x;
    openmvdata[7] = parsed.east_y;
    openmvdata[9] = parsed.west_y;
    black_num = parsed.black_num;
    openmv_last_rx_time = system_time;
    openmv_has_valid_frame = 1u;
    if(openmvdata[0] > CAR_OPENMV_MAX_X || openmvdata[0] < 0)
    {
        openmvdata[0] = CAR_OPENMV_TARGET_X;
    }
    if(board == 0 && ttt == 0)
    {
        if(openmvdata[7] > CAR_OPENMV_BOARD_Y_TRIGGER)
        {
            zuo1++;
        }
        else
        {
            zuo1 = 0;
        }
        if(openmvdata[9] > CAR_OPENMV_BOARD_Y_TRIGGER)
        {
            you1++;
        }
        else
        {
            you1 = 0;
        }
        if(zuo1 >= CAR_OPENMV_BOARD_DEBOUNCE || you1 >= CAR_OPENMV_BOARD_DEBOUNCE)
        {
            detect_board();
        }
    }
    else
    {
        zuo1 = 0;
        you1 = 0;
    }
}

u8 USART3_RX_BUF[CAR_OPENMV_RX_BUF_SIZE]={0};
void USART3_IRQHandler(void)
{
    uint8_t res;
    uint8_t clear = 0;
    static uint8_t Rx_Sta = 1;
    static uint8_t Rx_Overflow = 0;
    static uint8_t Rx_Stream_Delivered = 0;
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        res =USART3->DR;
#if CAR_OPENMV_USART3_ENABLED
        if(Rx_Overflow == 0)
        {
            if(Rx_Sta < sizeof(USART3_RX_BUF))
            {
                u8 wire[CAR_OPENMV_FRAME_SIZE];
                USART3_RX_BUF[Rx_Sta++] = res;
                if(CarProtocol_StreamFeedByte(&openmv_usart3_stream, res, CAR_OPENMV_MAGIC0, CAR_OPENMV_MAGIC1, CAR_OPENMV_FRAME_SIZE, CAR_OPENMV_TAIL, 0u, (u8)(CAR_OPENMV_FRAME_SIZE - 2u), (u8)(CAR_OPENMV_FRAME_SIZE - 2u), wire))
                {
                    openmv_copy_wire_frame_to_rxbuf(USART3_RX_BUF, wire);
                    Rx_Stream_Delivered = 1u;
                    GXT_DATA_DEAL();
                    Rx_Sta = 1u;
                }
            }
            else
            {
                Rx_Overflow = 1;
                Rx_Sta = 1;
                USART3_RX_BUF[0] = 0;
            }
        }
#else
        (void)res;
#endif
    }
    else if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
    {
        clear = USART3->SR;
        clear = USART3->DR;
        (void)clear;
#if CAR_OPENMV_USART3_ENABLED
        if(Rx_Overflow == 0)
        {
            USART3_RX_BUF[0] = Rx_Sta - 1;
            if(Rx_Stream_Delivered == 0u && USART3_RX_BUF[0] != 0u)
            {
                GXT_DATA_DEAL();
            }
        }
        else
#endif
        {
            USART3_RX_BUF[0] = 0;
            Rx_Overflow = 0;
        }
        Rx_Stream_Delivered = 0u;
        Rx_Sta = 1;
#if !CAR_OPENMV_USART3_ENABLED
        (void)Rx_Sta;
        (void)Rx_Overflow;
        (void)Rx_Stream_Delivered;
#endif
    }
}

void GXT_DATA_DEAL(void)
{
#if CAR_OPENMV_USART3_ENABLED
    openmv_apply_frame(USART3_RX_BUF);
#else
    USART3_RX_BUF[0] = 0;
#endif
}

u8 zuo1,you1;
int openmvdata[10];
u8 black_num;
u8 USART4_RX_BUF[CAR_OPENMV_RX_BUF_SIZE]={0};
void UART4_IRQHandler(void)
{
    uint8_t res;
    uint8_t clear = 0;
    static uint8_t Rx_Sta = 1;
    static uint8_t Rx_Overflow = 0;
    static uint8_t Rx_Stream_Delivered = 0;
    if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)
    {
        res =UART4->DR;
        if(Rx_Overflow == 0)
        {
            if(Rx_Sta < sizeof(USART4_RX_BUF))
            {
                u8 wire[CAR_OPENMV_FRAME_SIZE];
                USART4_RX_BUF[Rx_Sta++] = res;
                if(CarProtocol_StreamFeedByte(&openmv_uart4_stream, res, CAR_OPENMV_MAGIC0, CAR_OPENMV_MAGIC1, CAR_OPENMV_FRAME_SIZE, CAR_OPENMV_TAIL, 0u, (u8)(CAR_OPENMV_FRAME_SIZE - 2u), (u8)(CAR_OPENMV_FRAME_SIZE - 2u), wire))
                {
                    openmv_copy_wire_frame_to_rxbuf(USART4_RX_BUF, wire);
                    Rx_Stream_Delivered = 1u;
#if CAR_OPENMV_UART4_ENABLED
                    openmv_apply_frame(USART4_RX_BUF);
#endif
                    Rx_Sta = 1u;
                }
            }
            else
            {
                Rx_Overflow = 1;
                Rx_Sta = 1;
                USART4_RX_BUF[0] = 0;
            }
        }
    }
    else if(USART_GetITStatus(UART4, USART_IT_IDLE) != RESET)
    {
        clear = UART4->SR;
        clear = UART4->DR;
        (void)clear;
        if(Rx_Overflow == 0)
        {
            USART4_RX_BUF[0] = Rx_Sta - 1;
            if(Rx_Stream_Delivered == 0u && USART4_RX_BUF[0] != 0u)
            {
#if CAR_OPENMV_UART4_ENABLED
                openmv_apply_frame(USART4_RX_BUF);
#endif
            }
        }
        else
        {
            USART4_RX_BUF[0] = 0;
            Rx_Overflow = 0;
        }
        Rx_Stream_Delivered = 0u;
        Rx_Sta = 1;
    }
}

void detect_board(void)
{
    board = 1;
    time_up = CAR_OPENMV_BOARD_HOLD_MS;
}

u8 OpenMV_HasValidFrame(void)
{
    return (u8)(openmv_has_valid_frame != 0u);
}

openmv_status_t OpenMV_GetStatus(u32 timeout_ms)
{
    if(CAR_OPENMV_REQUIRE_FIRST_FRAME != 0u && openmv_has_valid_frame == 0u)
    {
        CarDiag_RecordThrottled(CAR_DIAG_OPENMV_NO_FRAME, 0u, 0u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
        return OPENMV_STATUS_NO_FRAME;
    }
    if((u32)(system_time - openmv_last_rx_time) > timeout_ms)
    {
        CarDiag_RecordThrottled(CAR_DIAG_OPENMV_STALE, (u8)(timeout_ms & 0xFFu), 0u, 0u, CAR_DIAG_FAULT_THROTTLE_MS);
        return OPENMV_STATUS_STALE;
    }
    return OPENMV_STATUS_FRESH;
}

u8 OpenMV_IsFresh(u32 timeout_ms)
{
    return (u8)(OpenMV_GetStatus(timeout_ms) == OPENMV_STATUS_FRESH);
}

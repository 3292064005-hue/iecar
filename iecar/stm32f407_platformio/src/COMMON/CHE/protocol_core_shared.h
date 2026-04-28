#ifndef PROTOCOL_CORE_SHARED_H__
#define PROTOCOL_CORE_SHARED_H__

static inline u8 CarProtocol_Checksum8(const u8 *data, u8 count);

typedef struct
{
    u8 valid;
    u8 is_v2;
    u8 mask;
    u8 confidence;
    u16 x;
    u16 y;
    u8 seq;
} car_protocol_k210_frame_t;

typedef struct
{
    u8 valid;
    u8 is_v2;
    s16 center_x;
    s16 east_y;
    s16 west_y;
    u8 black_num;
} car_protocol_openmv_frame_t;

#ifndef CAR_PROTOCOL_STREAM_MAX_FRAME_SIZE
#define CAR_PROTOCOL_STREAM_MAX_FRAME_SIZE 16u
#endif

typedef struct
{
    u8 count;
    u16 resync_count;
    u16 bad_tail_count;
    u16 bad_checksum_count;
    u16 overflow_count;
    u8 buffer[CAR_PROTOCOL_STREAM_MAX_FRAME_SIZE];
} car_protocol_stream_parser_t;

static inline void CarProtocol_StreamInit(car_protocol_stream_parser_t *parser)
{
    if(parser != 0)
    {
        u8 i;
        parser->count = 0u;
        parser->resync_count = 0u;
        parser->bad_tail_count = 0u;
        parser->bad_checksum_count = 0u;
        parser->overflow_count = 0u;
        for(i = 0u; i < CAR_PROTOCOL_STREAM_MAX_FRAME_SIZE; ++i)
        {
            parser->buffer[i] = 0u;
        }
    }
}

/*
 * 功能: 在一帧失败后从已缓存窗口内部滑动寻找下一组 magic，保留可能属于下一帧的后缀。
 * 入参: parser 为流式解析状态；magic0/magic1 为帧头；frame_size 为当前固定帧长。
 * 出参: 无，parser->count 被更新为可继续接收的后缀长度。
 * 异常: 找不到可用帧头时清空状态；统计 resync_count。
 * 边界: 只保留完整 magic0/magic1 后缀，或末字节单独 magic0；不产生业务帧。
 */
static inline void CarProtocol_StreamResyncFromWindow(car_protocol_stream_parser_t *parser,
                                                      u8 magic0,
                                                      u8 magic1,
                                                      u8 frame_size)
{
    u8 i;
    u8 j;
    u8 start = 0xFFu;
    u8 new_count = 0u;

    if(parser == 0 || frame_size == 0u || frame_size > CAR_PROTOCOL_STREAM_MAX_FRAME_SIZE)
    {
        return;
    }

    for(i = 1u; (u8)(i + 1u) < frame_size; ++i)
    {
        if(parser->buffer[i] == magic0 && parser->buffer[(u8)(i + 1u)] == magic1)
        {
            start = i;
            break;
        }
    }
    if(start == 0xFFu && parser->buffer[(u8)(frame_size - 1u)] == magic0)
    {
        start = (u8)(frame_size - 1u);
    }

    if(start != 0xFFu)
    {
        new_count = (u8)(frame_size - start);
        for(j = 0u; j < new_count; ++j)
        {
            parser->buffer[j] = parser->buffer[(u8)(start + j)];
        }
        parser->count = new_count;
    }
    else
    {
        parser->count = 0u;
    }
    parser->resync_count++;
}

/*
 * 功能: 对固定长度串口 wire frame 做流式重同步解析，支持噪声前缀、半包和粘包。
 * 入参: parser 为持久解析状态；byte 为新字节；magic/tail/checksum 参数描述帧契约；out_frame 输出 wire frame。
 * 出参: 返回 1 表示 out_frame 获得一帧已通过 tail/checksum 的完整帧，返回 0 表示仍在等待或丢弃坏帧。
 * 异常: 参数非法、长度越界、tail/checksum 错误均不阻塞，只更新 parser 计数并尝试从后续 magic 重同步。
 * 边界: 只负责 wire frame 边界与校验，不解释业务字段；固定长度帧大小不得超过 CAR_PROTOCOL_STREAM_MAX_FRAME_SIZE。
 */
static inline u8 CarProtocol_StreamFeedByte(car_protocol_stream_parser_t *parser,
                                            u8 byte,
                                            u8 magic0,
                                            u8 magic1,
                                            u8 frame_size,
                                            u8 tail,
                                            u8 checksum_start,
                                            u8 checksum_count,
                                            u8 checksum_index,
                                            u8 *out_frame)
{
    u8 i;
    if(parser == 0 || out_frame == 0 || frame_size == 0u || frame_size > CAR_PROTOCOL_STREAM_MAX_FRAME_SIZE)
    {
        return 0u;
    }

    if(parser->count == 0u)
    {
        if(byte == magic0)
        {
            parser->buffer[0] = byte;
            parser->count = 1u;
        }
        return 0u;
    }

    if(parser->count == 1u)
    {
        if(byte == magic1)
        {
            parser->buffer[1] = byte;
            parser->count = 2u;
            return 0u;
        }
        if(byte == magic0)
        {
            parser->resync_count++;
            parser->buffer[0] = byte;
            parser->count = 1u;
            return 0u;
        }
        parser->resync_count++;
        parser->count = 0u;
        return 0u;
    }

    if(parser->count >= frame_size)
    {
        parser->overflow_count++;
        parser->count = 0u;
        return 0u;
    }

    parser->buffer[parser->count++] = byte;
    if(parser->count < frame_size)
    {
        return 0u;
    }

    if(parser->buffer[(u8)(frame_size - 1u)] != tail)
    {
        parser->bad_tail_count++;
        CarProtocol_StreamResyncFromWindow(parser, magic0, magic1, frame_size);
        return 0u;
    }
    if(checksum_count != 0u)
    {
        if(checksum_start >= frame_size || checksum_index >= frame_size || ((u16)checksum_start + (u16)checksum_count) > (u16)frame_size)
        {
            parser->overflow_count++;
            CarProtocol_StreamResyncFromWindow(parser, magic0, magic1, frame_size);
            return 0u;
        }
        if(CarProtocol_Checksum8(&parser->buffer[checksum_start], checksum_count) != parser->buffer[checksum_index])
        {
            parser->bad_checksum_count++;
            CarProtocol_StreamResyncFromWindow(parser, magic0, magic1, frame_size);
            return 0u;
        }
    }
    for(i = 0u; i < frame_size; ++i)
    {
        out_frame[i] = parser->buffer[i];
    }
    parser->count = 0u;
    return 1u;
}

static inline u8 CarProtocol_Checksum8(const u8 *data, u8 count)
{
    u16 sum = 0;
    u8 i;
    for(i = 0; i < count; ++i)
    {
        sum = (u16)(sum + data[i]);
    }
    return (u8)sum;
}

static inline u8 CarProtocol_Checksum8Volatile(volatile u8 *data, u8 count)
{
    u16 sum = 0;
    u8 i;
    for(i = 0; i < count; ++i)
    {
        sum = (u16)(sum + data[i]);
    }
    return (u8)sum;
}

static inline u8 CarProtocol_VisualVersionIsValid(u8 version_byte)
{
    if(CAR_PROTOCOL_REJECT_VERSION_MISMATCH == 0u)
    {
        return 1u;
    }
    return (u8)(version_byte == CAR_PROTOCOL_VERSION_BYTE);
}

/*
 * 功能: 解析 STM32 本地 K210 RX 缓冲区。buf[0] 是 ISR 写入的线缆帧长度，不是线缆字节。
 * 入参: buf 指向 RX 缓冲区，支持 v2 版本化 wire frame；可按配置兼容 v1 无版本帧。
 * 出参: 返回填充后的 car_protocol_k210_frame_t，valid=1 表示通过 magic/version/checksum/tail。
 * 异常: 入参空或 checksum/version/长度错误时 valid=0，各字段置零。
 * 边界: 不做置信度、坐标、类别业务过滤；只负责帧契约解析。
 */
static inline car_protocol_k210_frame_t CarProtocol_ParseK210RxBuffer(volatile u8 *buf)
{
    car_protocol_k210_frame_t out;
    out.valid = 0u;
    out.is_v2 = 0u;
    out.mask = 0u;
    out.confidence = 0u;
    out.x = 0u;
    out.y = 0u;
    out.seq = 0u;

    if(buf == 0)
    {
        return out;
    }
    if(buf[0] == CAR_K210_FRAME_SIZE &&
       buf[1] == CAR_K210_MAGIC0 && buf[2] == CAR_K210_MAGIC1 &&
       buf[CAR_K210_FRAME_SIZE] == CAR_K210_TAIL &&
       CarProtocol_VisualVersionIsValid(buf[3]) &&
       CarProtocol_Checksum8Volatile(&buf[1], (u8)(CAR_K210_FRAME_SIZE - 2u)) == buf[CAR_K210_FRAME_SIZE - 1u])
    {
        out.valid = 1u;
        out.is_v2 = 1u;
        out.mask = buf[4];
        out.confidence = buf[5];
        out.x = (u16)(((u16)buf[6] << 8) | buf[7]);
        out.y = (u16)(((u16)buf[8] << 8) | buf[9]);
        out.seq = buf[10];
        return out;
    }
#if CAR_VISUAL_PROTOCOL_COMPAT_V1
    if(buf[0] == 11u &&
       buf[1] == CAR_K210_MAGIC0 && buf[2] == CAR_K210_MAGIC1 && buf[11] == CAR_K210_TAIL &&
       CarProtocol_Checksum8Volatile(&buf[1], 9u) == buf[10])
    {
        out.valid = 1u;
        out.is_v2 = 0u;
        out.mask = buf[3];
        out.confidence = buf[4];
        out.x = (u16)(((u16)buf[5] << 8) | buf[6]);
        out.y = (u16)(((u16)buf[7] << 8) | buf[8]);
        out.seq = buf[9];
        return out;
    }
#endif
    return out;
}

/*
 * 功能: 解析 STM32 本地 OpenMV RX 缓冲区。buf[0] 是 ISR 写入的线缆帧长度，不是线缆字节。
 * 入参: buf 指向 RX 缓冲区，支持 v2 版本化 wire frame；可按配置兼容 v1 无版本帧。
 * 出参: 返回填充后的 car_protocol_openmv_frame_t，valid=1 表示通过帧契约。
 * 异常: 入参空或 checksum/version/长度错误时 valid=0，各字段置零。
 * 边界: 不做巡线业务裁剪；只负责协议解析。
 */
static inline car_protocol_openmv_frame_t CarProtocol_ParseOpenMVRxBuffer(volatile u8 *buf)
{
    car_protocol_openmv_frame_t out;
    out.valid = 0u;
    out.is_v2 = 0u;
    out.center_x = 0;
    out.east_y = 0;
    out.west_y = 0;
    out.black_num = 0u;

    if(buf == 0)
    {
        return out;
    }
    if(buf[0] == CAR_OPENMV_FRAME_SIZE &&
       buf[1] == CAR_OPENMV_MAGIC0 && buf[2] == CAR_OPENMV_MAGIC1 &&
       buf[CAR_OPENMV_FRAME_SIZE] == CAR_OPENMV_TAIL &&
       CarProtocol_VisualVersionIsValid(buf[3]) &&
       CarProtocol_Checksum8Volatile(&buf[1], (u8)(CAR_OPENMV_FRAME_SIZE - 2u)) == buf[CAR_OPENMV_FRAME_SIZE - 1u])
    {
        out.valid = 1u;
        out.is_v2 = 1u;
        out.center_x = (s16)(((u16)buf[4] << 8) | buf[5]);
        out.east_y = (s16)(((u16)buf[6] << 8) | buf[7]);
        out.west_y = (s16)(((u16)buf[8] << 8) | buf[9]);
        out.black_num = buf[10];
        return out;
    }
#if CAR_VISUAL_PROTOCOL_COMPAT_V1
    if(buf[0] == 11u &&
       buf[1] == CAR_OPENMV_MAGIC0 && buf[2] == CAR_OPENMV_MAGIC1 && buf[11] == CAR_OPENMV_TAIL &&
       CarProtocol_Checksum8Volatile(&buf[1], 9u) == buf[10])
    {
        out.valid = 1u;
        out.is_v2 = 0u;
        out.center_x = (s16)(((u16)buf[3] << 8) | buf[4]);
        out.east_y = (s16)(((u16)buf[5] << 8) | buf[6]);
        out.west_y = (s16)(((u16)buf[7] << 8) | buf[8]);
        out.black_num = buf[9];
        return out;
    }
#endif
    return out;
}

static inline u8 CarProtocol_CarLinkChecksum(const u8 *frame)
{
    return CarProtocol_Checksum8(frame, 8u);
}

static inline u8 CarProtocol_CarLinkFrameIsLegacy(const u8 *frame)
{
    return (u8)(frame != 0 &&
                frame[0] == CAR_LINK_MAGIC0 && frame[1] == CAR_LINK_MAGIC1 &&
                frame[10] == CAR_LINK_TAIL && frame[8] == 0u && frame[9] == 0u);
}

static inline u8 CarProtocol_CarLinkVersionIsValid(const u8 *frame)
{
    if(CAR_PROTOCOL_REJECT_VERSION_MISMATCH == 0u)
    {
        return 1u;
    }
    return (u8)(frame != 0 && frame[6] == CAR_PROTOCOL_VERSION_MAJOR);
}

static inline u8 CarProtocol_CarLinkFrameIsValid(const u8 *frame, u8 len)
{
    if(frame == 0 || len != CAR_LINK_FRAME_SIZE)
    {
        return 0u;
    }
    if(frame[0] != CAR_LINK_MAGIC0 || frame[1] != CAR_LINK_MAGIC1 || frame[10] != CAR_LINK_TAIL)
    {
        return 0u;
    }
#if CAR_PROTOCOL_COMPAT_V1
    if(CarProtocol_CarLinkFrameIsLegacy(frame))
    {
        return 1u;
    }
#endif
    if(CarProtocol_CarLinkChecksum(frame) == frame[8])
    {
        return CarProtocol_CarLinkVersionIsValid(frame);
    }
    return 0u;
}


/*
 * 功能: CarLink 专用流式解析器，兼容 v2 checksum 帧与 legacy no-checksum 帧。
 * 入参: parser 为持久解析状态；byte 为新字节；out_frame 输出完整 wire frame。
 * 出参: 返回 1 表示输出一帧通过 CarLink 固定校验/legacy 兼容校验的帧，返回 0 表示继续等待或丢弃坏帧。
 * 异常: tail、checksum、版本或长度异常时会从坏帧内部滑动重同步，不阻塞后续帧。
 * 边界: 本函数只做 CarLink wire-frame 边界与兼容校验，不投递业务事件、不发送 ACK。
 */
static inline u8 CarProtocol_StreamFeedCarLinkByte(car_protocol_stream_parser_t *parser,
                                                   u8 byte,
                                                   u8 *out_frame)
{
    u8 i;
    if(parser == 0 || out_frame == 0 || CAR_LINK_FRAME_SIZE > CAR_PROTOCOL_STREAM_MAX_FRAME_SIZE)
    {
        return 0u;
    }

    if(parser->count == 0u)
    {
        if(byte == CAR_LINK_MAGIC0)
        {
            parser->buffer[0] = byte;
            parser->count = 1u;
        }
        return 0u;
    }

    if(parser->count == 1u)
    {
        if(byte == CAR_LINK_MAGIC1)
        {
            parser->buffer[1] = byte;
            parser->count = 2u;
            return 0u;
        }
        if(byte == CAR_LINK_MAGIC0)
        {
            parser->resync_count++;
            parser->buffer[0] = byte;
            parser->count = 1u;
            return 0u;
        }
        parser->resync_count++;
        parser->count = 0u;
        return 0u;
    }

    if(parser->count >= CAR_LINK_FRAME_SIZE)
    {
        parser->overflow_count++;
        parser->count = 0u;
        return 0u;
    }

    parser->buffer[parser->count++] = byte;
    if(parser->count < CAR_LINK_FRAME_SIZE)
    {
        return 0u;
    }

    if(parser->buffer[(u8)(CAR_LINK_FRAME_SIZE - 1u)] != CAR_LINK_TAIL)
    {
        parser->bad_tail_count++;
        CarProtocol_StreamResyncFromWindow(parser, CAR_LINK_MAGIC0, CAR_LINK_MAGIC1, CAR_LINK_FRAME_SIZE);
        return 0u;
    }

    if(!CarProtocol_CarLinkFrameIsValid(parser->buffer, CAR_LINK_FRAME_SIZE))
    {
        parser->bad_checksum_count++;
        CarProtocol_StreamResyncFromWindow(parser, CAR_LINK_MAGIC0, CAR_LINK_MAGIC1, CAR_LINK_FRAME_SIZE);
        return 0u;
    }

    for(i = 0u; i < CAR_LINK_FRAME_SIZE; ++i)
    {
        out_frame[i] = parser->buffer[i];
    }
    parser->count = 0u;
    return 1u;
}

static inline u8 CarProtocol_ExpectedPeerRole(u8 car_role_id)
{
    if(car_role_id == 1u)
    {
        return 2u;
    }
    if(car_role_id == 2u)
    {
        return 1u;
    }
    return 0u;
}

static inline u8 CarProtocol_NormalizeCarLinkSenderRole(const u8 *frame, u8 legacy, u8 car_role_id, u8 car_task)
{
    u8 expected_peer = CarProtocol_ExpectedPeerRole(car_role_id);
    if(frame == 0)
    {
        return 0u;
    }
    if(legacy)
    {
        return expected_peer;
    }
    if(expected_peer != 0u && frame[5] == expected_peer)
    {
        return expected_peer;
    }
#if CAR_LINK_COMPAT_TASK_FIELD
    if(expected_peer != 0u && frame[5] == car_task)
    {
        return expected_peer;
    }
#endif
    return frame[5];
}

#endif /* PROTOCOL_CORE_SHARED_H__ */

#ifndef PROTOCOL_CORE_SHARED_H__
#define PROTOCOL_CORE_SHARED_H__

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
    if(CarProtocol_CarLinkChecksum(frame) == frame[8])
    {
        return CarProtocol_CarLinkVersionIsValid(frame);
    }
#if CAR_PROTOCOL_COMPAT_V1
    if(CarProtocol_CarLinkFrameIsLegacy(frame))
    {
        return 1u;
    }
#endif
    return 0u;
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

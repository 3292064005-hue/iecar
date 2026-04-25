#ifndef HOST_K210_H__
#define HOST_K210_H__

#include "host_platform.h"

typedef enum
{
    K210_CAMERA_LEFT = 1,
    K210_CAMERA_RIGHT = 2
} k210_camera_id_t;

typedef struct
{
    u8 camera_id;
    u8 valid_mask;
    u8 best_target;
    int stable_votes;
    u8 confidence;
    u16 x;
    u16 y;
    u8 seq;
    u8 fresh;
    u8 confidence_ok;
    u8 position_ok;
    u8 reliable;
    u8 has_valid_frame;
    long long int rx_time_ms;
} k210_observation_t;

u8 K210_GetObservation(u8 camera_id, k210_observation_t *out);

#endif /* HOST_K210_H__ */

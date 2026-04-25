#include <string.h>

car_diag_record_t car_diag_ring[CAR_DIAG_RING_SIZE];
volatile u8 car_diag_write_index = 0;
volatile u16 car_diag_overrun_count = 0;

#ifndef CAR_DIAG_THROTTLE_SLOTS
#define CAR_DIAG_THROTTLE_SLOTS 8u
#endif

static car_diag_record_t car_diag_throttle_slots[CAR_DIAG_THROTTLE_SLOTS];
static u8 car_diag_throttle_used[CAR_DIAG_THROTTLE_SLOTS];

void CarDiag_Record(u8 event, u8 arg0, u8 arg1, u8 arg2)
{
    u8 index = car_diag_write_index;
    car_diag_ring[index].time_ms = system_time;
    car_diag_ring[index].event = event;
    car_diag_ring[index].arg0 = arg0;
    car_diag_ring[index].arg1 = arg1;
    car_diag_ring[index].arg2 = arg2;
    index++;
    if(index >= CAR_DIAG_RING_SIZE)
    {
        index = 0;
        car_diag_overrun_count++;
    }
    car_diag_write_index = index;
}

u8 CarDiag_RecordThrottled(u8 event, u8 arg0, u8 arg1, u8 arg2, u32 min_interval_ms)
{
    u8 i;
    u8 victim = 0u;
    u32 oldest_age = 0u;

    for(i = 0u; i < CAR_DIAG_THROTTLE_SLOTS; ++i)
    {
        if(car_diag_throttle_used[i] != 0u &&
           car_diag_throttle_slots[i].event == event &&
           car_diag_throttle_slots[i].arg0 == arg0 &&
           car_diag_throttle_slots[i].arg1 == arg1 &&
           car_diag_throttle_slots[i].arg2 == arg2)
        {
            if((u32)(system_time - car_diag_throttle_slots[i].time_ms) < min_interval_ms)
            {
                return 0u;
            }
            car_diag_throttle_slots[i].time_ms = system_time;
            CarDiag_Record(event, arg0, arg1, arg2);
            return 1u;
        }
    }

    for(i = 0u; i < CAR_DIAG_THROTTLE_SLOTS; ++i)
    {
        u32 age;
        if(car_diag_throttle_used[i] == 0u)
        {
            victim = i;
            break;
        }
        age = (u32)(system_time - car_diag_throttle_slots[i].time_ms);
        if(age >= oldest_age)
        {
            oldest_age = age;
            victim = i;
        }
    }

    car_diag_throttle_used[victim] = 1u;
    car_diag_throttle_slots[victim].time_ms = system_time;
    car_diag_throttle_slots[victim].event = event;
    car_diag_throttle_slots[victim].arg0 = arg0;
    car_diag_throttle_slots[victim].arg1 = arg1;
    car_diag_throttle_slots[victim].arg2 = arg2;
    CarDiag_Record(event, arg0, arg1, arg2);
    return 1u;
}

void CarDiag_Clear(void)
{
    memset(car_diag_ring, 0, sizeof(car_diag_ring));
    car_diag_write_index = 0;
    car_diag_overrun_count = 0;
    memset(car_diag_throttle_slots, 0, sizeof(car_diag_throttle_slots));
    memset(car_diag_throttle_used, 0, sizeof(car_diag_throttle_used));
}

u8 CarDiag_GetLatest(car_diag_record_t *out)
{
    u8 index;
    if(out == 0)
    {
        return 0u;
    }
    if(car_diag_write_index == 0u && car_diag_overrun_count == 0u && car_diag_ring[0].event == CAR_DIAG_NONE)
    {
        return 0u;
    }
    index = (car_diag_write_index == 0u) ? (u8)(CAR_DIAG_RING_SIZE - 1u) : (u8)(car_diag_write_index - 1u);
    *out = car_diag_ring[index];
    return 1u;
}

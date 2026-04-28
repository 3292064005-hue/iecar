#include "car_diag.h"
#include "car_mainchain.h"
#include <stdio.h>
#include <string.h>

car_diag_record_t car_diag_ring[CAR_DIAG_RING_SIZE];
volatile u8 car_diag_write_index = 0;
volatile u16 car_diag_overrun_count = 0;

#ifndef CAR_DIAG_THROTTLE_SLOTS
#define CAR_DIAG_THROTTLE_SLOTS 8u
#endif

static car_diag_record_t car_diag_throttle_slots[CAR_DIAG_THROTTLE_SLOTS];
static u8 car_diag_throttle_used[CAR_DIAG_THROTTLE_SLOTS];

static u32 car_diag_irq_save(void)
{
    u32 primask = __get_PRIMASK();
    INTX_DISABLE();
    return primask;
}

static void car_diag_irq_restore(u32 primask)
{
    __set_PRIMASK(primask);
}

void CarDiag_Record(u8 event, u8 arg0, u8 arg1, u8 arg2)
{
    u8 index;
    u32 irq_state = car_diag_irq_save();
    index = car_diag_write_index;
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
    car_diag_irq_restore(irq_state);
}

u8 CarDiag_RecordThrottled(u8 event, u8 arg0, u8 arg1, u8 arg2, u32 min_interval_ms)
{
    u8 i;
    u8 victim = 0u;
    u32 oldest_age = 0u;
    u8 should_record = 0u;
    u32 irq_state = car_diag_irq_save();
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
                car_diag_irq_restore(irq_state);
                return 0u;
            }
            car_diag_throttle_slots[i].time_ms = system_time;
            should_record = 1u;
            car_diag_irq_restore(irq_state);
            CarDiag_Record(event, arg0, arg1, arg2);
            return should_record;
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
    car_diag_irq_restore(irq_state);
    CarDiag_Record(event, arg0, arg1, arg2);
    return 1u;
}

void CarDiag_Clear(void)
{
    u32 irq_state = car_diag_irq_save();
    memset(car_diag_ring, 0, sizeof(car_diag_ring));
    car_diag_write_index = 0;
    car_diag_overrun_count = 0;
    memset(car_diag_throttle_slots, 0, sizeof(car_diag_throttle_slots));
    memset(car_diag_throttle_used, 0, sizeof(car_diag_throttle_used));
    car_diag_irq_restore(irq_state);
}

u8 CarDiag_GetLatest(car_diag_record_t *out)
{
    u8 index;
    if(out == 0)
    {
        return 0u;
    }
    u32 irq_state = car_diag_irq_save();
    if(car_diag_write_index == 0u && car_diag_overrun_count == 0u && car_diag_ring[0].event == CAR_DIAG_NONE)
    {
        car_diag_irq_restore(irq_state);
        return 0u;
    }
    index = (car_diag_write_index == 0u) ? (u8)(CAR_DIAG_RING_SIZE - 1u) : (u8)(car_diag_write_index - 1u);
    *out = car_diag_ring[index];
    car_diag_irq_restore(irq_state);
    return 1u;
}


u8 CarDiag_CopyRecent(car_diag_record_t *out, u8 capacity)
{
    u8 available;
    u8 count;
    u8 start;
    u8 i;
    if(out == 0 || capacity == 0u)
    {
        return 0u;
    }
    u32 irq_state = car_diag_irq_save();
    available = (car_diag_overrun_count != 0u) ? CAR_DIAG_RING_SIZE : car_diag_write_index;
    if(available > CAR_DIAG_RING_SIZE)
    {
        available = CAR_DIAG_RING_SIZE;
    }
    count = (available < capacity) ? available : capacity;
    if(count == 0u)
    {
        car_diag_irq_restore(irq_state);
        return 0u;
    }
    start = (u8)((car_diag_write_index + CAR_DIAG_RING_SIZE - count) % CAR_DIAG_RING_SIZE);
    for(i = 0u; i < count; ++i)
    {
        out[i] = car_diag_ring[(u8)((start + i) % CAR_DIAG_RING_SIZE)];
    }
    car_diag_irq_restore(irq_state);
    return count;
}

u8 CarDiag_FormatRecord(const car_diag_record_t *record, char *out, u8 capacity)
{
    if(record == 0 || out == 0 || capacity == 0u)
    {
        return 0u;
    }
    if(capacity < 2u)
    {
        out[0] = '\0';
        return 0u;
    }
    snprintf(out, capacity, "D%u:%u,%u,%u", record->event, record->arg0, record->arg1, record->arg2);
    out[capacity - 1u] = '\0';
    return 1u;
}

u8 CarDiag_ShowLatestSummary(u8 y)
{
    car_diag_record_t latest;
    if(!CarDiag_GetLatest(&latest))
    {
        return 0u;
    }
    if(!CarDiag_FormatRecord(&latest, sss, 20u))
    {
        return 0u;
    }
    OLED_ShowString(0, y, sss, 16);
    return 1u;
}

u8 openmv_num=0;
int detect_num=0;
u8 car_sta=0;
int sr04=0;
int vel;
float Kp_qwq = CAR_LINE_PID_KP;

u8 board;

u8 ss1,ss2,ss3,ss4,ss5;
u8 car_stop=0;
int time_up=0;

int Speed = CAR_DEFAULT_SPEED;
int Speed1;
int Speed2;

void Car_SetLineSpeed(int new_speed)
{
    Speed = new_speed;
}

int Car_ComputeLineCorrection(int center_x)
{
    int error = center_x - CAR_OPENMV_TARGET_X;
    int correction = (int)(error * Kp_qwq);
    if(correction > CAR_LINE_MAX_CORRECTION)
    {
        correction = CAR_LINE_MAX_CORRECTION;
    }
    if(correction < -CAR_LINE_MAX_CORRECTION)
    {
        correction = -CAR_LINE_MAX_CORRECTION;
    }
    return correction;
}

u8 Car_BuildLineOutput(car_line_output_t *out)
{
    if(out == 0)
    {
        return 0u;
    }

    memset(out, 0, sizeof(*out));
    {
        openmv_status_t status = OpenMV_GetStatus(CAR_OPENMV_FRESHNESS_MS);
        out->fresh = (u8)(status == OPENMV_STATUS_FRESH);
        if(status == OPENMV_STATUS_NO_FRAME)
        {
            out->event = CAR_LINE_EVENT_OPENMV_NO_FRAME;
            return 1u;
        }
        if(status == OPENMV_STATUS_STALE)
        {
            out->event = CAR_LINE_EVENT_OPENMV_STALE;
            return 1u;
        }
    }

    if(openmvdata[0] != 0)
    {
        vel = Car_ComputeLineCorrection(openmvdata[0]);
        Speed1 = Speed + vel;
        Speed2 = Speed - vel;
        out->left_speed = Speed1;
        out->right_speed = Speed2;
        out->line_seen = 1u;
        out->event = CAR_LINE_EVENT_NONE;
        return 1u;
    }

    if(black_num >= CAR_OPENMV_BLACK_TRIGGER)
    {
        out->event = CAR_LINE_EVENT_STATION;
        return 1u;
    }

    out->left_speed = CAR_OPENMV_LINE_LOST_SPEED;
    out->right_speed = CAR_OPENMV_LINE_LOST_SPEED;
    out->event = CAR_LINE_EVENT_LINE_LOST;
    return 1u;
}

void Car_ApplyLineOutput(const car_line_output_t *out)
{
    if(out == 0)
    {
        CAR_STOP();
        return;
    }
    if(out->event == CAR_LINE_EVENT_STATION || Car_LineEventIsVisionFault(out->event))
    {
        CAR_STOP();
        return;
    }
    MOTOR_ANTI(out->left_speed, out->right_speed);
}

u8 Car_LineEventIsVisionFault(car_line_event_t event)
{
    return (u8)(event == CAR_LINE_EVENT_OPENMV_NO_FRAME || event == CAR_LINE_EVENT_OPENMV_STALE);
}

void CAR_xunji(void)
{
    car_line_output_t line_output;
    if(!Car_BuildLineOutput(&line_output))
    {
        CAR_STOP();
        return;
    }
    Car_ApplyLineOutput(&line_output);
}

void TIM7_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM7,TIM_IT_Update)==1)
    {
        TIM_ClearITPendingBit(TIM7,TIM_IT_Update);
        static int blink_cnt;
        static int ttt_20hz;
        static u8 speed_phase=0;
        if(blink_cnt>=50)
        {
            blink_cnt=0;
            MY_LED_407_TOG;
        }
        blink_cnt++;
        if(speed_phase==0)
        {
            SPEED_GET();
        }
        speed_phase=!speed_phase;
        if(ttt_20hz>=5)
        {
            ttt_20hz=0;
        }
        ttt_20hz++;
    }
}

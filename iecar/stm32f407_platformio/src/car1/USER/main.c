#include "../SYSTEM/PLATFORM/car_mainchain.h"
#include "../SYSTEM/STRATEGY/common/task_bootstrap_common.h"

long long int system_time=0;
char sss[20];
void JumpToBootloader(void);

int main(void)
{
    NVIC_PriorityGroupConfig(2);
    delay_init(168);
    RCC_ClocksTypeDef get_rcc_clock;
    RCC_GetClocksFreq(&get_rcc_clock);

    OLED_Init();
    sprintf(sss,"%d",get_rcc_clock.SYSCLK_Frequency);
    OLED_ShowString(0,0,sss,8);
#if CAR_ROLE_ID == 1u
    OLED_ShowString(0,2,"ROLE:CAR1",8);
#elif CAR_ROLE_ID == 2u
    OLED_ShowString(0,2,"ROLE:CAR2",8);
#else
    OLED_ShowString(0,2,"ROLE:UNK",8);
#endif
    OLED_ShowString(0,4,"k210_1: 0",8);
    OLED_ShowString(0,6,"k210_2: 0",8);
    delay_ms(500);

    TIM6_INIT(999,83);
    TIM7_INIT(9999,83);
    KEY_Init();
    CAR_KAIGUAN_INIT();
    led_my_407();
    CAR_Driver_Init();
    PAout(4)=0;
    delay_ms(100);
    PAout(4)=1;
    MOTOR_PID();
    ENCODER_2_INIT();
    ENCODER_3_INIT();
    maiche_init();
    RGB_EN(YELLOW);
    delay_ms(200);
    RGB_EN(BLUE);
    delay_ms(200);
    RGB_EN(DARK);
    tim8_init(999,83);

    usart_1_init(115200);
    SBUS_Configuration();
#if CAR_OPENMV_USART3_ENABLED
    CAR_UART3_INIT(115200);
#endif
#if CAR_OPENMV_UART4_ENABLED
    CAR_UART4_INIT(115200);
#endif
    CAR_UART5_INIT(115200);
    CAR_UART6_INIT(115200);

    car_task = TaskBootstrap_ReadSelector();
    sprintf(sss,"T-%d",car_task);
    OLED_ShowString(80,0,sss,8);
    TaskBootstrap_DispatchTaskWithContract(car_task, TASK1_GO, TASK2_GO, TASK3_GO);
    while(1);
}

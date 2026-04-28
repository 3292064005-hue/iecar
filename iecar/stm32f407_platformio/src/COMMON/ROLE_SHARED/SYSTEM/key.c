#include "key.h"



void KEY_Init(void)//PE2,PE3,PA15exti
{
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//	GPIO_Init(GPIOE,&GPIO_InitStructure);
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);						 //ʹSYSCFGʱ
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource15);
	
	/* EXTI_Line2 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line15;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;		//ж¼
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //½ش
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;				//жʹ
	EXTI_Init(&EXTI_InitStructure);							//
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01; //ռȼ3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;		 //ȼ2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				 //ʹⲿжͨ
	NVIC_Init(&NVIC_InitStructure);								 //
}


//ⲿж15
void EXTI15_10_IRQHandler(void)
{
	static long long int key_trig_t=0;
	if (EXTI_GetITStatus(EXTI_Line15) == 1)
	{
		EXTI_ClearITPendingBit(EXTI_Line15); //LINE2ϵжϱ־λ
		if(system_time-key_trig_t>=200)
		{
			MY_LED_407_TOG;
			key_trig_t=system_time;
			flag_go=!flag_go;
		}
	}
}


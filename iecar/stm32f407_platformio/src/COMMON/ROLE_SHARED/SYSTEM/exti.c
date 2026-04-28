#include "exti.h"

void exti_init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE, ENABLE); //ʹGPIOA,GPIOEʱ
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);						 //ʹSYSCFGʱ

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //KEY0 KEY1 KEY2Ӧ
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;		   //ͨģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	   //100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;		   //
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource2);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource3);

	/* EXTI_Line2 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line2 | EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;		//ж¼
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //½ش
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;				//жʹ
	EXTI_Init(&EXTI_InitStructure);							//

	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;			 //ⲿж2
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01; //ռȼ3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;		 //ȼ2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				 //ʹⲿжͨ
	NVIC_Init(&NVIC_InitStructure);								 //
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;			 //ⲿж2
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01; //ռȼ3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;		 //ȼ2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				 //ʹⲿжͨ
	NVIC_Init(&NVIC_InitStructure);								 //
}

//ⲿж2
void EXTI2_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line2) == 1)
	{
		EXTI_ClearITPendingBit(EXTI_Line2); //LINE2ϵжϱ־λ
		//LED0_TOG;
		delay_ms(200);//
	}
}

//ⲿж3
void EXTI3_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line3) == 1)
	{
		EXTI_ClearITPendingBit(EXTI_Line3); //LINE2ϵжϱ־λ
		LED1_TOG;
		delay_ms(200); //
	}
}

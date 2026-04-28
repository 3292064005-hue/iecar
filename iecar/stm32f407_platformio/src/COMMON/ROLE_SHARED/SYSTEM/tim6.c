#include "tim6.h"

extern int time_up;
extern int ttt;

void TIM9_INIT(u32 arr,u32 psc)
{		 					 
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9,ENABLE);  
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE); 	//ʹPORTFʱ	
	
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource5,GPIO_AF_TIM9); 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;           
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//ٶ100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;        //
	GPIO_Init(GPIOE,&GPIO_InitStructure);              //ʼPF9
	
	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //ʱƵ
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //ϼģʽ
	TIM_TimeBaseStructure.TIM_Period=arr;   //Զװֵ
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM9,&TIM_TimeBaseStructure);//ʼʱ14
	
	//ʼTIM14 Channel1 PWMģʽ	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //ѡʱģʽ:TIMȵģʽ2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //Ƚʹ
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //:TIMȽϼԵ
	TIM_OC1Init(TIM9, &TIM_OCInitStructure);  //TָĲʼTIM1 4OC1
	TIM_OC1PreloadConfig(TIM9, TIM_OCPreload_Enable);  //ʹTIM14CCR1ϵԤװؼĴ

  	TIM_ARRPreloadConfig(TIM9,ENABLE);//ARPEʹ 
	
	TIM_Cmd(TIM9, ENABLE);  //ʹTIM14
}

void TIM6_INIT(u32 arr,u32 psc)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6,ENABLE);  	//TIM14ʱʹ    	
	  
	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //ʱƵ
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //ϼģʽ
	TIM_TimeBaseStructure.TIM_Period=arr;   //Զװֵ
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM6,&TIM_TimeBaseStructure);
	
  TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE);

	TIM_Cmd(TIM6, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM6_DAC_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x00;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


void TIM6_DAC_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM6,TIM_IT_Update)==1) //ж
	{
		TIM_ClearITPendingBit(TIM6,TIM_IT_Update);  //жϱ־λ
		system_time++;
		if(time_up>0)time_up--;
		if(ttt>0)ttt--;
	}
}



void TIM7_INIT(u32 arr,u32 psc)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE);  	//TIM14ʱʹ    	
	
	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //ʱƵ
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //ϼģʽ
	TIM_TimeBaseStructure.TIM_Period=arr;   //Զװֵ
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM7,&TIM_TimeBaseStructure);
	
  TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE);

	TIM_Cmd(TIM7, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM7_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}



#include "tim14.h"


void TIM14_PWM_Init(u32 arr,u32 psc)
{		 					 
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14,ENABLE);  	//TIM14ʱʹ    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE); 	//ʹPORTFʱ	
	
	GPIO_PinAFConfig(GPIOF,GPIO_PinSource9,GPIO_AF_TIM14); //GPIOF9Ϊʱ14
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;           //GPIOF9
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//ٶ100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //
	GPIO_Init(GPIOF,&GPIO_InitStructure);              //ʼPF9
	
	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //ʱƵ
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //ϼģʽ
	TIM_TimeBaseStructure.TIM_Period=arr;   //Զװֵ
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM14,&TIM_TimeBaseStructure);//ʼʱ14
	
	//ʼTIM14 Channel1 PWMģʽ	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //ѡʱģʽ:TIMȵģʽ2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //Ƚʹ
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //:TIMȽϼԵ
	TIM_OC1Init(TIM14, &TIM_OCInitStructure);  //TָĲʼTIM1 4OC1
	TIM_OC1PreloadConfig(TIM14, TIM_OCPreload_Enable);  //ʹTIM14CCR1ϵԤװؼĴ

  TIM_ARRPreloadConfig(TIM14,ENABLE);//ARPEʹ 
	
	TIM_Cmd(TIM14, ENABLE);  //ʹTIM14
}


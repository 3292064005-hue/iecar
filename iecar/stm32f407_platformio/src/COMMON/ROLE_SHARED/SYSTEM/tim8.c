#include "tim8.h"

void tim8_init(u32 arr,u32 psc)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8,ENABLE);  
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); 	//ʹPORTFʱ	
	
//  GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_TIM8); 
//  GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_TIM8); 
  GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_TIM8); 
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_TIM8); 
	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;  
GPIO_InitStructure.GPIO_Pin =GPIO_Pin_8|GPIO_Pin_9;      	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//ٶ100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;        //
	GPIO_Init(GPIOC,&GPIO_InitStructure);              //ʼPF9
	
	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //ʱƵ
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //ϼģʽ
	TIM_TimeBaseStructure.TIM_Period=arr;   //Զװֵ
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM8,&TIM_TimeBaseStructure);//ʼʱ14
	
	//ʼTIM14 Channel1 PWMģʽ	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //ѡʱģʽ:TIMȵģʽ2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //Ƚʹ
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //:TIMȽϼԵ
//	TIM_OC1Init(TIM8, &TIM_OCInitStructure);  //TָĲʼTIM1 4OC1
//  TIM_OC2Init(TIM8, &TIM_OCInitStructure);  //TָĲʼTIM1 4OC1
  TIM_OC3Init(TIM8, &TIM_OCInitStructure);  //TָĲʼTIM1 4OC1
  TIM_OC4Init(TIM8, &TIM_OCInitStructure);  //TָĲʼTIM1 4OC1

//  TIM_OC1PreloadConfig(TIM8, TIM_OCPreload_Enable);  //ʹTIM14CCR1ϵԤװؼĴ
//  TIM_OC2PreloadConfig(TIM8, TIM_OCPreload_Enable);  //ʹTIM14CCR1ϵԤװؼĴ
  TIM_OC3PreloadConfig(TIM8, TIM_OCPreload_Enable);  //ʹTIM14CCR1ϵԤװؼĴ
	TIM_OC4PreloadConfig(TIM8, TIM_OCPreload_Enable);  //ʹTIM14CCR1ϵԤװؼĴ

  TIM_ARRPreloadConfig(TIM8,ENABLE);//ARPEʹ
	TIM_CtrlPWMOutputs(TIM8, ENABLE);
	TIM_Cmd(TIM8, ENABLE);  //ʹTIM14
//	TIM_SetCompare1(TIM8,0);
//	TIM_SetCompare2(TIM8,0);
	TIM_SetCompare3(TIM8,0);
	TIM_SetCompare4(TIM8,0);
}


void maiche_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;        
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;        //ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//ٶ100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;      
	GPIO_Init(GPIOE,&GPIO_InitStructure);              //ʼPF9
	PEout(7)=0;
	PEout(8)=0;
	PEout(9)=0;
	PEout(10)=0;
	PEout(11)=0;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;        
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;      
	GPIO_Init(GPIOE,&GPIO_InitStructure);
    PEout(12)=0;
    PEout(13)=0;
    PEout(14)=0;
    PEout(15)=0;
}


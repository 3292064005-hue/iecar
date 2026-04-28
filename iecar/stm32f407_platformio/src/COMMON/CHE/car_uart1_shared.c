#include "car_uart1.h"
void CAR_UART3_INIT(u32 bound)//PD8,PD9
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE); //ʹGPIOAʱ
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//ʹUSART1ʱ
	
	//1ӦŸӳ
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_USART3); 
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_USART3); 
	
	//USART1˿
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//ٶ50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //
	GPIO_Init(GPIOD,&GPIO_InitStructure); //ʼPA9PA10

  //USART1 ʼ
	USART_InitStructure.USART_BaudRate = bound;//
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//ֳΪ8λݸʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//żУλ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//Ӳ
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//շģʽ
  USART_Init(USART3, &USART_InitStructure); //ʼ1
	
  USART_Cmd(USART3, ENABLE);  //ʹܴ1 	
	
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//1жͨ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;//ռȼ3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =1;		//ȼ3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨʹ
	NVIC_Init(&NVIC_InitStructure);	//ָĲʼVICĴ
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//ж
	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);//ж
}

void CAR_UART4_INIT(u32 bound)//PC10,PC11
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //ʹGPIOAʱ
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//ʹUSART1ʱ
	
	//1ӦŸӳ
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4); 
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART4); 
	
	//USART1˿
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//ٶ50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //
	GPIO_Init(GPIOC,&GPIO_InitStructure); //ʼPA9PA10

  //USART1 ʼ
	USART_InitStructure.USART_BaudRate = bound;//
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//ֳΪ8λݸʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//żУλ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//Ӳ
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//շģʽ
  USART_Init(UART4, &USART_InitStructure); //ʼ1
	
  USART_Cmd(UART4, ENABLE);  //ʹܴ1 	
	
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;//1жͨ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;//ռȼ3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//ȼ3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨʹ
	NVIC_Init(&NVIC_InitStructure);	//ָĲʼVICĴ
	
	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//ж
	USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);//ж
}

void CAR_UART5_INIT(u32 bound)//PC10,PC11
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE); //ʹGPIOAʱ
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5,ENABLE);//ʹUSART1ʱ
	
	//1ӦŸӳ
	//GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4); 
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource2,GPIO_AF_UART5); 
	
	//USART1˿
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//ٶ50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //
	GPIO_Init(GPIOD,&GPIO_InitStructure); //ʼPA9PA10

  //USART1 ʼ
	USART_InitStructure.USART_BaudRate = bound;//
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//ֳΪ8λݸʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//żУλ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//Ӳ
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//շģʽ
  USART_Init(UART5, &USART_InitStructure); //ʼ1
	
  USART_Cmd(UART5, ENABLE);  //ʹܴ1 	
	
  NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;//1жͨ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//ռȼ3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//ȼ3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨʹ
	NVIC_Init(&NVIC_InitStructure);	//ָĲʼVICĴ
	
	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);//ж
	USART_ITConfig(UART5, USART_IT_IDLE, ENABLE);//ж
}

void CAR_UART6_INIT(u32 bound)//PC10,PC11
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //ʹGPIOAʱ
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6,ENABLE);//ʹUSART1ʱ
	
	//1ӦŸӳ
	//GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_USART6);
	
	//USART1˿
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//ٶ50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //
	GPIO_Init(GPIOC,&GPIO_InitStructure); //ʼPA9PA10

  //USART1 ʼ
	USART_InitStructure.USART_BaudRate = bound;//
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//ֳΪ8λݸʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//żУλ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//Ӳ
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//շģʽ
  USART_Init(USART6, &USART_InitStructure); //ʼ1
	
  USART_Cmd(USART6, ENABLE);  //ʹܴ1 	
	
  NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;//1жͨ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//ռȼ3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =1;		//ȼ3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨʹ
	NVIC_Init(&NVIC_InitStructure);	//ָĲʼVICĴ
	
	USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);//ж
	USART_ITConfig(USART6, USART_IT_IDLE, ENABLE);//ж
}

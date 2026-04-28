#include "usart1.h"
#include "ctrl_num.h"
#include "protocol_core_shared.h"

static car_protocol_stream_parser_t car2_usart1_debug_stream;

void usart_1_init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //ʹGPIOAʱ
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//ʹUSART1ʱ
	
	//1ӦŸӳ
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9ΪUSART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10ΪUSART1
	
	//USART1˿
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//ٶ50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //
	GPIO_Init(GPIOA,&GPIO_InitStructure); //ʼPA9PA10

  //USART1 ʼ
	USART_InitStructure.USART_BaudRate = bound;//
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//ֳΪ8λݸʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//żУλ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//Ӳ
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//շģʽ
  USART_Init(USART1, &USART_InitStructure); //ʼ1

	USART_Cmd(USART1, ENABLE);  //ʹܴ1
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//ж
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//ж
	
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//1жͨ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;	//IRQͨʹ
	NVIC_Init(&NVIC_InitStructure);	//ָĲʼVICĴ
}

u8 USART1_RX_BUF[CAR_COMPAT_RX_BUF_SIZE]={0};
void USART1_IRQHandler(void)
{
	uint8_t res;
	uint8_t clear = 0;
	static uint8_t Rx_Sta = 1;
	static uint8_t Rx_Overflow = 0;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		res =USART1->DR;
		if(Rx_Overflow == 0)
		{
			if(Rx_Sta < sizeof(USART1_RX_BUF))
			{
				USART1_RX_BUF[Rx_Sta++] = res;
			}
			else
			{
				Rx_Overflow = 1;
				Rx_Sta = 1;
				USART1_RX_BUF[0] = 0;
			}
		}
		//USART_SendData(USART1,res);
	}
	if(USART_GetITStatus(USART1, USART_IT_ORE_RX) != RESET)
	{USART_ClearFlag(USART1,USART_IT_ORE_RX);}
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		clear = USART1->SR;
		clear = USART1->DR;
		(void)clear;
		if(Rx_Overflow == 0)
		{
			USART1_RX_BUF[0] = Rx_Sta - 1;
#if CAR_LINK_USART1_DEBUG_RX_ENABLED
			/* Intentional debug compatibility inlet: ISR only frames and enqueues; no ACK or business parsing is done here. */
			{
				u8 i;
				u8 wire[CAR_LINK_FRAME_SIZE];
				for(i = 0u; i < USART1_RX_BUF[0]; ++i)
				{
					if(CarProtocol_StreamFeedCarLinkByte(&car2_usart1_debug_stream,
					                                 USART1_RX_BUF[(u8)(i + 1u)],
					                                 wire))
					{
						(void)CarLink_SubmitRxFrameFromIsrEx(wire, CAR_LINK_FRAME_SIZE, 0u);
					}
				}
			}
#endif
		}
		else
		{
			USART1_RX_BUF[0] = 0;
			Rx_Overflow = 0;
		}
		Rx_Sta = 1;
	}
}



//´,֧printf,Ҫѡuse MicroLIB	  
#if defined(__CC_ARM)
#pragma import(__use_no_semihosting)
#endif
//׼Ҫֺ֧                 
struct __FILE 
{ 
	int handle; 
}; 
FILE __stdout;       
//_sys_exit()Աʹðģʽ    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//ضfputc 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//ѭ,ֱ   
	USART1->DR = (u8) ch;
	return ch;
}

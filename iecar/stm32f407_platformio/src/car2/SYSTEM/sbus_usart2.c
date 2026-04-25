#include "sbus_usart2.h"
void SBUS_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //ʹGPIOAʱ
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//ʹUSART1ʱ
	
	//1ӦŸӳ
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2); //GPIOA9ΪUSART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2); //GPIOA10ΪUSART1
	
	//USART1˿
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //GPIOA9GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//ù
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//ٶ50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //츴
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //
	GPIO_Init(GPIOA,&GPIO_InitStructure); //ʼPA9PA10

  //USART1 ʼ
	USART_InitStructure.USART_BaudRate = 115200;//
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//ֳΪ8λݸʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//żУλ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//Ӳ
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//շģʽ
  USART_Init(USART2, &USART_InitStructure); //ʼ1

	USART_Cmd(USART2, ENABLE);  //ʹܴ1
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//ж
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);//ж
	
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;//1жͨ
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =1;		
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;	//IRQͨʹ
	NVIC_Init(&NVIC_InitStructure);	//ָĲʼVICĴ
}

uint8_t USART2_RX_BUF[CAR_LINK_RX_BUF_SIZE]={0};

/**
  * @name   USART2_IRQHandler
  * @brief  This function handles USART2 Handler
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
	uint8_t res;
	uint8_t clear = 0;
	static uint8_t Rx_Sta = 1;
	static uint8_t Rx_Overflow = 0;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		res =USART2->DR;
		if(Rx_Overflow == 0)
		{
			if(Rx_Sta < sizeof(USART2_RX_BUF))
			{
				USART2_RX_BUF[Rx_Sta++] = res;
			}
			else
			{
				Rx_Overflow = 1;
				Rx_Sta = 1;
				USART2_RX_BUF[0] = 0;
			}
		}
		//USART_SendData(USART2,res);
	}
	else if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		clear = USART2->SR;
		clear = USART2->DR;
		if(Rx_Overflow == 0)
		{
			USART2_RX_BUF[0] = Rx_Sta - 1;
		}
		else
		{
			USART2_RX_BUF[0] = 0;
			Rx_Overflow = 0;
		}
		Rx_Sta = 1;
		
		//ݴ
		U2_DATA_DEAL();
	}
}

uint16_t CH[18]={0};  // ֵͨ
#define ch_val_change 0
u8 rc_flag=0;
void Sbus_Data_Count(uint8_t *buf)
{
	CH[0]=(buf[24]==0x0C)?0:1;//жǷյңźţյΪ1Ϊ0
	if(ch_val_change==0)
	{
		CH[ 1] = ((int16_t)buf[ 2] >> 0 | ((int16_t)buf[ 3] << 8 )) & 0x07FF;
		CH[ 2] = ((int16_t)buf[ 3] >> 3 | ((int16_t)buf[ 4] << 5 )) & 0x07FF;
		CH[ 3] = ((int16_t)buf[ 4] >> 6 | ((int16_t)buf[ 5] << 2 )  | (int16_t)buf[ 6] << 10 ) & 0x07FF;
		CH[ 4] = ((int16_t)buf[ 6] >> 1 | ((int16_t)buf[ 7] << 7 )) & 0x07FF;
		CH[ 5] = ((int16_t)buf[ 7] >> 4 | ((int16_t)buf[ 8] << 4 )) & 0x07FF;
		CH[ 6] = ((int16_t)buf[ 8] >> 7 | ((int16_t)buf[ 9] << 1 )  | (int16_t)buf[10] <<  9 ) & 0x07FF;
		CH[ 7] = ((int16_t)buf[10] >> 2 | ((int16_t)buf[11] << 6 )) & 0x07FF;
		CH[ 8] = ((int16_t)buf[11] >> 5 | ((int16_t)buf[12] << 3 )) & 0x07FF;
		CH[ 9] = ((int16_t)buf[13] << 0 | ((int16_t)buf[14] << 8 )) & 0x07FF;
		CH[10] = ((int16_t)buf[14] >> 3 | ((int16_t)buf[15] << 5 )) & 0x07FF;
		CH[11] = ((int16_t)buf[15] >> 6 | ((int16_t)buf[16] << 2 )  | (int16_t)buf[17] << 10 ) & 0x07FF;
		CH[12] = ((int16_t)buf[17] >> 1 | ((int16_t)buf[18] << 7 )) & 0x07FF;
		CH[13] = ((int16_t)buf[18] >> 4 | ((int16_t)buf[19] << 4 )) & 0x07FF;
		CH[14] = ((int16_t)buf[19] >> 7 | ((int16_t)buf[20] << 1 )  | (int16_t)buf[21] <<  9 ) & 0x07FF;
		CH[15] = ((int16_t)buf[21] >> 2 | ((int16_t)buf[22] << 6 )) & 0x07FF;
		CH[16] = ((int16_t)buf[22] >> 5 | ((int16_t)buf[23] << 3 )) & 0x07FF;
	}
	else
	{
		;
	}
	if (CH[0]==1&&CH[3]!=0&&CH[3]!=1700)	rc_flag=1;
	else	rc_flag=0;
}


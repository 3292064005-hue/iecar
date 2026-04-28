#include "qy3806_abs.h"


/*ʹʾ----ĬSTM32F407-SPI1-PB3,4,5,CSƬѡѡPB6
QY3806_init();//ʼ

SSIReadAngle(&qy_state,&qy_crc8,&qy_readdata);//ȡݵУѶֱ仰
if(qy_state==16)//һݿʹʱstateΪ0x10(16)Ϊ3031ˣݴڱqy_readdata
{
    sprintf(sss,"%d",qy_readdata);
    OLED_ShowString(0,2,sss,8);
    delay_ms(50);//עʱʹʱԸĵ
}
//עһ¶qy_readdataֵΧ0/1-1024ʹýǶȿ*3601024
*/

u8 qy_state;
u8 qy_crc8;
u16 qy_readdata;
void QY3806_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;
	
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	
  	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹGPIOBʱ
  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);//ʹSPI1ʱ
 
	//GPIOB3,4,5ʼ
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;//PB3~5ù	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//ù
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//
	GPIO_Init(GPIOB, &GPIO_InitStructure);//ʼ
	
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource3,GPIO_AF_SPI1); //PB3Ϊ SPI1
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource4,GPIO_AF_SPI1); //PB4Ϊ SPI1
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource5,GPIO_AF_SPI1); //PB5Ϊ SPI1
 
	//ֻSPIڳʼ
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,ENABLE);//λSPI1
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,DISABLE);//ֹͣλSPI1


    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex ;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRCֵĶʽ
	SPI_Init(SPI1, &SPI_InitStructure);  //SPI_InitStructָĲʼSPIxĴ
	SPI_Cmd(SPI1, ENABLE); //ʹSPI
}


u8 QY3806_Send_Data(u8 TxData)
{		 			 
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET){}//ȴ  
    SPI_I2S_SendData(SPI1, TxData); //ͨSPIxһbyte    
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET){} //ȴһbyte  
    return SPI_I2S_ReceiveData(SPI1); //ͨSPIxյ		    
}


void delay_us_rj(u16 time)
{    
   u16 i=0;  
   while(time--)
   {
      i=18;  //Լ
      while(i--) ;    
   }
}

uint8_t SSIReadAngle(uint8_t *state,uint8_t *CRC8, uint16_t* ReadRegData)
{
    uint8_t *rddata,Parity;
    uint16_t SSIAngle;
    rddata = (uint8_t*)&SSIAngle;
    SPI1_CS_OFF; //bit[0]: Module _spi_ena
    delay_us_rj(1);
    rddata[1] = QY3806_Send_Data(0); //SPI read first MSB data.
    rddata[0] = QY3806_Send_Data(0); //SPI read second LSB data.
    delay_us_rj(1);
    SPI1_CS_ON; //bit[0]: Disable SPI function
    Parity = Even_parity(SSIAngle>>1,15); // data 1--15BIT parity check
    *CRC8 = Parity;
    if((SSIAngle&0x01) == Parity)
    {
        *ReadRegData = ((SSIAngle>>6)&0x0fff);
        *state = (SSIAngle>>1) & 0x1f;
        return 1;
    }
    return 0;
}

int Even_parity(int value, int n_bit)
{
    int parity = 0;
    while(n_bit-- > 0)
    {
    parity += value & 1;
    value >>= 1;
    }
    return (parity % 2);
}


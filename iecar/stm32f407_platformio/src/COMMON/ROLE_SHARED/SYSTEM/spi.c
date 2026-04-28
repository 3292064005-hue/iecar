#include "spi.h"

//SPIģĳʼ룬óģʽ 						  
//SPIڳʼ
//ǶSPI1ĳʼ
void SPI1_Init(void)
{	 
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹGPIOBʱ
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);//ʹSPI1ʱ
 
  //GPIOFB3,4,5ʼ
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

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //SPI˫ģʽ:SPIΪ˫˫ȫ˫
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//SPIģʽ:ΪSPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//SPIݴС:SPIͽ8λ֡ṹ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;		//ͬʱӵĿ״̬Ϊߵƽ
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	//ͬʱӵĵڶأ½ݱ
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSSźӲNSSܽţʹSSIλ:ڲNSSźSSIλ
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;		//岨ԤƵֵ:ԤƵֵΪ256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//ָݴMSBλLSBλʼ:ݴMSBλʼ
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRCֵĶʽ
	SPI_Init(SPI1, &SPI_InitStructure);  //SPI_InitStructָĲʼSPIxĴ
 
	SPI_Cmd(SPI1, ENABLE); //ʹSPI

	SPI1_ReadWriteByte(0xff);//		 
}   
//SPI1ٶú
//SPIٶ=fAPB2/Ƶϵ
//@ref SPI_BaudRate_Prescaler:SPI_BaudRatePrescaler_2~SPI_BaudRatePrescaler_256  
//fAPB2ʱһΪ84Mhz
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
{
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));//жЧ
	SPI1->CR1&=0XFFC7;//λ3-5㣬ò
	SPI1->CR1|=SPI_BaudRatePrescaler;	//SPI1ٶ 
	SPI_Cmd(SPI1,ENABLE); //ʹSPI1
} 
//SPI1 дһֽ
//TxData:Ҫдֽ
//ֵ:ȡֽ
u8 SPI1_ReadWriteByte(u8 TxData)
{		 			 
 
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET){}//ȴ  
	
	SPI_I2S_SendData(SPI1, TxData); //ͨSPIxһbyte  
		
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET){} //ȴһbyte  
 
	return SPI_I2S_ReceiveData(SPI1); //ͨSPIxյ	
 		    
}









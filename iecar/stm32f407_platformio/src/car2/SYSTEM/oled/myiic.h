#ifndef __MYIIC_H
#define __MYIIC_H
#include "../PLATFORM/car_platform.h"
//////////////////////////////////////////////////////////////////////////////////	 
//ֻѧϰʹãδɣκ;
//ALIENTEKսSTM32
//IIC 	   
//ԭ@ALIENTEK
//̳:www.openedv.com
//޸:2012/9/9
//汾V1.0
//ȨУؾ
//Copyright(C) ӿƼ޹˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

//IO
 
#define SDA_IN()  {GPIOE->MODER&=~(3<<(1*2));GPIOE->MODER|=0<<1*2;}
#define SDA_OUT() {GPIOE->MODER&=~(3<<(1*2));GPIOE->MODER|=1<<1*2;}

//IO
#define IIC_SCL    PEout(0) //SCL
#define IIC_SDA    PEout(1) //SDA	 
#define READ_SDA   PEin(1)  //SDA 

//IICв
void IIC_Init(void);                //ʼIICIO				 
void IIC_Start(void);				//IICʼź
void IIC_Stop(void);	  			//IICֹͣź
void IIC_Send_Byte(u8 txd);			//IICһֽ
u8 IIC_Read_Byte(unsigned char ack);//IICȡһֽ
u8 IIC_Wait_Ack(void); 				//IICȴACKź
void IIC_Ack(void);					//IICACKź
void IIC_NAck(void);				//IICACKź

void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_Read_One_Byte(u8 daddr,u8 addr);	  
#endif

















#ifndef __SPI_H
#define __SPI_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//ֻѧϰʹãδɣκ;
//ALIENTEK STM32F407
//SPI 	   
//ԭ@ALIENTEK
//̳:www.openedv.com
//:2014/5/6
//汾V1.0
//ȨУؾ
//Copyright(C) ӿƼ޹˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
 	    													  
void SPI1_Init(void);			 //ʼSPI1
void SPI1_SetSpeed(u8 SpeedSet); //SPI1ٶ   
u8 SPI1_ReadWriteByte(u8 TxData);//SPI1߶дһֽ
		 
#endif


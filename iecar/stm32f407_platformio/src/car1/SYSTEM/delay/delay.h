#ifndef __DELAY_H
#define __DELAY_H 			   
#include "../PLATFORM/car_platform.h"	  
//////////////////////////////////////////////////////////////////////////////////  
//ֻѧϰʹãδɣκ;
//ALIENTEK STM32F407
//ʹSysTickͨģʽӳٽй(֧ucosii)
//delay_us,delay_ms
//ԭ@ALIENTEK
//̳:www.openedv.com
//޸:2014/5/2
//汾V1.0
//ȨУؾ
//Copyright(C) ӿƼ޹˾ 2014-2024
//All rights reserved
//********************************************************************************
//޸˵
//
////////////////////////////////////////////////////////////////////////////////// 	 
void delay_init(u8 SYSCLK);
void delay_ms(u16 nms);
void delay_us(u32 nus);

#endif






























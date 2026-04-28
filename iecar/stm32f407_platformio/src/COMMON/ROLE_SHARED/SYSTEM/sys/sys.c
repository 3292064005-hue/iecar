#include "sys.h"  
//////////////////////////////////////////////////////////////////////////////////	 
//ֻѧϰʹãδɣκ;
//ALIENTEK STM32F407
//ϵͳʱӳʼ	
//ʱ/жϹ/GPIOõ
//ԭ@ALIENTEK
//̳:www.openedv.com
//:2014/5/2
//汾V1.0
//ȨУؾ
//Copyright(C) ӿƼ޹˾ 2014-2024
//All rights reserved
//********************************************************************************
//޸˵
//
//////////////////////////////////////////////////////////////////////////////////  


//THUMBָֻ֧
//·ʵִлָWFI  
void WFI_SET(void)
{
	__WFI();
}
void INTX_DISABLE(void)
{
	__disable_irq();
}
void INTX_ENABLE(void)
{
	__enable_irq();
}
void MSR_MSP(u32 addr)
{
	__set_MSP(addr);
}

int gxt_abs(int a)
{
	return a>0?a:(-a);
}













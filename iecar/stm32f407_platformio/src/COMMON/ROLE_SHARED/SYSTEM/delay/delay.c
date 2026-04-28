#include "delay.h"
#include "sys.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//ʹOS,ͷļucosΪ.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//֧OSʱʹ	  
#endif
//////////////////////////////////////////////////////////////////////////////////  
//ֻѧϰʹãδɣκ;
//ALIENTEK STM32F407
//ʹSysTickͨģʽӳٽй(֧OS)
//delay_us,delay_ms
//ԭ@ALIENTEK
//̳:www.openedv.com
//:2014/5/2
//汾V1.3
//ȨУؾ
//Copyright(C) ӿƼ޹˾ 2014-2024
//All rights reserved
//********************************************************************************
//޸˵
//V1.1 20140803 
//1,delay_us,Ӳ0ж,0,ֱ˳. 
//2,޸ucosii,delay_ms,OSLockNestingж,ڽжϺ,Ҳ׼ȷʱ.
//V1.2 20150411  
//޸OSַ֧ʽ,֧OS(UCOSIIUCOSIII,OS֧)
//:delay_osrunning/delay_ostickspersec/delay_osintnesting궨
//:delay_osschedlock/delay_osschedunlock/delay_ostimedly
//V1.3 20150521
//UCOSIII֧ʱ2bug
//delay_tickspersecΪdelay_ostickspersec
//delay_intnestingΪdelay_osintnesting
////////////////////////////////////////////////////////////////////////////////// 

static u8  fac_us=0;							//usʱ			   
static u16 fac_ms=0;							//msʱ,os,ÿĵms
	
#if SYSTEM_SUPPORT_OS							//SYSTEM_SUPPORT_OS,˵Ҫ֧OS(UCOS).
//delay_us/delay_msҪ֧OSʱҪOSصĺ궨ͺ֧
//3궨:
//    delay_osrunning:ڱʾOSǰǷ,ԾǷʹغ
//delay_ostickspersec:ڱʾOS趨ʱӽ,delay_initʼsystick
// delay_osintnesting:ڱʾOSжǶ׼,Ϊж治Ե,delay_msʹøò
//Ȼ3:
//  delay_osschedlock:OS,ֹ
//delay_osschedunlock:ڽOS,¿
//    delay_ostimedly:OSʱ,.

//̽UCOSIIUCOSIII֧,OS,вοֲ
//֧UCOSII
#ifdef 	OS_CRITICAL_METHOD						//OS_CRITICAL_METHOD,˵Ҫ֧UCOSII				
#define delay_osrunning		OSRunning			//OSǷб,0,;1,
#define delay_ostickspersec	OS_TICKS_PER_SEC	//OSʱӽ,ÿȴ
#define delay_osintnesting 	OSIntNesting		//жǶ׼,жǶ״
#endif

//֧UCOSIII
#ifdef 	CPU_CFG_CRITICAL_METHOD					//CPU_CFG_CRITICAL_METHOD,˵Ҫ֧UCOSIII	
#define delay_osrunning		OSRunning			//OSǷб,0,;1,
#define delay_ostickspersec	OSCfg_TickRate_Hz	//OSʱӽ,ÿȴ
#define delay_osintnesting 	OSIntNestingCtr		//жǶ׼,жǶ״
#endif


//usʱʱ,ر(ֹusӳ)
void delay_osschedlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD   			//ʹUCOSIII
	OS_ERR err; 
	OSSchedLock(&err);						//UCOSIIIķʽ,ֹȣֹusʱ
#else										//UCOSII
	OSSchedLock();							//UCOSIIķʽ,ֹȣֹusʱ
#endif
}

//usʱʱ,ָ
void delay_osschedunlock(void)
{	
#ifdef CPU_CFG_CRITICAL_METHOD   			//ʹUCOSIII
	OS_ERR err; 
	OSSchedUnlock(&err);					//UCOSIIIķʽ,ָ
#else										//UCOSII
	OSSchedUnlock();						//UCOSIIķʽ,ָ
#endif
}

//OSԴʱʱ
//ticks:ʱĽ
void delay_ostimedly(u32 ticks)
{
#ifdef CPU_CFG_CRITICAL_METHOD
	OS_ERR err; 
	OSTimeDly(ticks,OS_OPT_TIME_PERIODIC,&err);//UCOSIIIʱģʽ
#else
	OSTimeDly(ticks);						//UCOSIIʱ
#endif 
}
 
//systickжϷ,ʹOSʱõ
void SysTick_Handler(void)
{	
	if(delay_osrunning==1)					//OSʼ,ִĵȴ
	{
		OSIntEnter();						//ж
		OSTimeTick();       				//ucosʱӷ               
		OSIntExit();       	 				//лж
	}
}
#endif
			   
//ʼӳٺ
//ʹOSʱ,˺ʼOSʱӽ
//SYSTICKʱӹ̶ΪAHBʱӵ1/8
//SYSCLK:ϵͳʱƵ
void delay_init(u8 SYSCLK)
{
#if SYSTEM_SUPPORT_OS 						//Ҫ֧OS.
	u32 reload;
#endif
 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8); 
	fac_us=SYSCLK/8;						//ǷʹOS,fac_usҪʹ
#if SYSTEM_SUPPORT_OS 						//Ҫ֧OS.
	reload=SYSCLK/8;						//ÿӵļ λΪM	   
	reload*=1000000/delay_ostickspersec;	//delay_ostickspersec趨ʱ
											//reloadΪ24λĴ,ֵ:16777216,168M,Լ0.7989s	
	fac_ms=1000/delay_ostickspersec;		//OSʱٵλ	   
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//SYSTICKж
	SysTick->LOAD=reload; 					//ÿ1/delay_ostickspersecжһ	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk; 	//SYSTICK    
#else
	fac_ms=(u16)fac_us*1000;				//OS,ÿmsҪsystickʱ   
#endif
}								    

#if SYSTEM_SUPPORT_OS 						//Ҫ֧OS.
//ʱnus
//nus:Ҫʱus.	
//nus:0~204522252(ֵ2^32/fac_us@fac_us=21)	    								   
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				//LOADֵ	    	 
	ticks=nus*fac_us; 						//ҪĽ 
	delay_osschedlock();					//ֹOSȣֹusʱ
	told=SysTick->VAL;        				//սʱļֵ
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	//עһSYSTICKһݼļͿ.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			//ʱ䳬/Ҫӳٵʱ,˳.
		}  
	};
	delay_osschedunlock();					//ָOS											    
}  
//ʱnms
//nms:Ҫʱms
//nms:0~65535
void delay_ms(u16 nms)
{	
	if(delay_osrunning&&delay_osintnesting==0)//OSѾ,Ҳж(ж治)	    
	{		 
		if(nms>=fac_ms)						//ʱʱOSʱ 
		{ 
   			delay_ostimedly(nms/fac_ms);	//OSʱ
		}
		nms%=fac_ms;						//OSѾ޷ṩôСʱ,ͨʽʱ    
	}
	delay_us((u32)(nms*1000));				//ͨʽʱ
}
#else  //ucosʱ
//ʱnus
//nusΪҪʱus.	
//ע:nusֵ,Ҫ798915us(ֵ2^24/fac_us@fac_us=21)
void delay_us(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD=nus*fac_us; 				//ʱ	  		 
	SysTick->VAL=0x00;        				//ռ
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ; //ʼ 	 
	do
	{
		temp=SysTick->CTRL;
	}while((temp&0x01)&&!(temp&(1<<16)));	//ȴʱ䵽   
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk; //رռ
	SysTick->VAL =0X00;       				//ռ 
}
//ʱnms
//עnmsķΧ
//SysTick->LOADΪ24λĴ,,ʱΪ:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLKλΪHz,nmsλΪms
//168M,nms<=798ms 
void delay_xms(u16 nms)
{	 		  	  
	u32 temp;		   
	SysTick->LOAD=(u32)nms*fac_ms;			//ʱ(SysTick->LOADΪ24bit)
	SysTick->VAL =0x00;           			//ռ
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //ʼ 
	do
	{
		temp=SysTick->CTRL;
	}while((temp&0x01)&&!(temp&(1<<16)));	//ȴʱ䵽   
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //رռ
	SysTick->VAL =0X00;     		  		//ռ	  	    
} 
//ʱnms 
//nms:0~65535
void delay_ms(u16 nms)
{	 	 
	u8 repeat=nms/540;						//540,ǿǵĳЩͻܳƵʹ,
											//糬Ƶ248Mʱ,delay_xmsֻʱ541ms
	u16 remain=nms%540;
	while(repeat)
	{
		delay_xms(540);
		repeat--;
	}
	if(remain)delay_xms(remain);
} 
#endif
			 




































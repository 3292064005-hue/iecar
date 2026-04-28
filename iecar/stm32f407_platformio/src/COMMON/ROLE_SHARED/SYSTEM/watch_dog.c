#include "watch_dog.h"



//ʼŹ
//prer:Ƶ:0~7(ֻе3λЧ!)
//rlr:Զװֵ,0~0XFFF.
//Ƶ=4*2^prer.ֵֻ256!
//rlr:װؼĴֵ:11λЧ.
//ʱ():Tout=((4*2^prer)*rlr)/32 (ms).
//IWDG_Init(3,9);//Źʼ,ӦҪС9msҪιһι
void IWDG_Init(u8 prer,u16 rlr)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); //ʹܶIWDG->PR IWDG->RLRд
	IWDG_SetPrescaler(prer); //IWDGƵϵ
	IWDG_SetReload(rlr);   //IWDGװֵ
	IWDG_ReloadCounter(); //reload
	IWDG_Enable();       //ʹܿŹ
}

//ιŹ
void IWDG_Feed(void)
{
	IWDG_ReloadCounter();//reload
}


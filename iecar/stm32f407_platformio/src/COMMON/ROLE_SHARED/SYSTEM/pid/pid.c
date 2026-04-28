#include "pid.h"

PID_wz wz_text;
PID_zl zl_text;


void  pid_Init(void)
{    
	wz_text.kp=0.5;
	wz_text.ki=0.01;
	wz_text.kd=0;
	zl_text.kp=0;
	zl_text.ki=0.001;
	zl_text.kd=0;
}

// λʽPID
float PID_WZ_TEST(float pid_now,float pid_anti)
{
    wz_text.err =pid_anti-pid_now; // 㵱ǰ
    if(1) //
		{wz_text.integral += wz_text.err;      //ƫĻ
		if(0)//޷
			{if(wz_text.integral >= wz_text.itl_lim_up){wz_text.integral =  wz_text.itl_lim_up;}//޷
			else if(wz_text.integral <= wz_text.itl_lim_dn){wz_text.integral =  wz_text.itl_lim_dn;}}
		}
			else wz_text.integral=0;//ֵ֣Ϊ0
		
		wz_text.dif=wz_text.err-wz_text.err_last;
    wz_text.out = wz_text.kp*wz_text.err + wz_text.ki*wz_text.integral + wz_text.kd*wz_text.dif;   //λʽPID
    wz_text.err_last = wz_text.err;   //һƫ 
		
		if(0){//Ƿ޷
    if(wz_text.out >= wz_text.limit_up){wz_text.out =  wz_text.limit_up;}//޷
		else if(wz_text.out <= wz_text.limit_dn){wz_text.out =  wz_text.limit_dn;}}
		return wz_text.out;
}


// ʽPID
float PID_ZL_TEST(float pid_now,float pid_anti)
{
    zl_text.err =pid_anti-pid_now; // 㵱ǰ
		zl_text.dif=zl_text.err-zl_text.err_last;//΢
		zl_text.dif_two=zl_text.err-2*zl_text.err_last+zl_text.err_pre;//΢
    zl_text.out = zl_text.kp*zl_text.dif + zl_text.ki*zl_text.err + zl_text.kd*zl_text.dif_two;   //λʽPID
    zl_text.err_pre = zl_text.err_last;   //һƫ 
		zl_text.err_last = zl_text.err;   //һƫ 
		if(0){//Ƿ޷
    if(zl_text.out >= zl_text.limit_up){zl_text.out =  zl_text.limit_up;}//޷
		else if(zl_text.out <= zl_text.limit_dn){zl_text.out =  zl_text.limit_dn;}}
		return zl_text.out;
}

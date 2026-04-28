#include "MOTOR_CTRL.h"
#include "car_mainchain.h"
PID_wz M_SP1;
PID_wz M_SP2;

#define max_sp 80
void MOTOR_ANTI(int sp1,int sp2)
{
	if(sp1>max_sp) sp1=max_sp;
	else if(sp1<-max_sp) sp1=-max_sp;
	if(sp2>max_sp) sp2=max_sp;
	else if(sp2<-max_sp) sp2=-max_sp;
	PID_WZ_M1(SPEED_1,sp1);
	PID_WZ_M2(SPEED_2,sp2);
	CAR_SPEED_SET((int)M_SP1.out,(int)M_SP2.out);
}
void MOTOR_PID(void)
{
	M_SP1.kp=26;
	M_SP1.ki=0;
	M_SP1.kd=1;
	M_SP1.itl_lim_up=1000;
	M_SP1.itl_lim_dn=-1000;
	M_SP1.limit_up=800;
	M_SP1.limit_dn=-800;
	
	M_SP2.kp=M_SP1.kp;
	M_SP2.ki=M_SP1.ki;
	M_SP2.kd=M_SP1.kd;
	M_SP2.itl_lim_up=M_SP1.itl_lim_up;
	M_SP2.itl_lim_dn=M_SP1.itl_lim_dn;
	M_SP2.limit_up=M_SP1.limit_up;
	M_SP2.limit_dn=M_SP1.limit_dn;
}


// λʽPID
float PID_WZ_M1(float pid_now,float pid_anti)
{
    M_SP1.err =pid_anti-pid_now; // 㵱ǰ
    if(1) //
		{M_SP1.integral += M_SP1.err;      //ƫĻ
		if(0)//޷
			{if(M_SP1.integral >= M_SP1.itl_lim_up){M_SP1.integral =  M_SP1.itl_lim_up;}//޷
			else if(M_SP1.integral <= M_SP1.itl_lim_dn){M_SP1.integral =  M_SP1.itl_lim_dn;}}
		}
			else M_SP1.integral=0;//ֵ֣Ϊ0
		
		M_SP1.dif=M_SP1.err-M_SP1.err_last;
    M_SP1.out = M_SP1.kp*M_SP1.err + M_SP1.ki*M_SP1.integral + M_SP1.kd*M_SP1.dif;   //λʽPID
    M_SP1.err_last = M_SP1.err;   //һƫ 
		
		
		if(1){//Ƿ޷
    if(M_SP1.out >= M_SP1.limit_up){M_SP1.out =  M_SP1.limit_up;}//޷
		else if(M_SP1.out <= M_SP1.limit_dn){M_SP1.out =  M_SP1.limit_dn;}}
		return M_SP1.out;
}
// λʽPID
float PID_WZ_M2(float pid_now,float pid_anti)
{
    M_SP2.err =pid_anti-pid_now; // 㵱ǰ
    if(1) //
		{M_SP2.integral += M_SP2.err;      //ƫĻ
		if(0)//޷
			{if(M_SP2.integral >= M_SP2.itl_lim_up){M_SP2.integral =  M_SP2.itl_lim_up;}//޷
			else if(M_SP2.integral <= M_SP2.itl_lim_dn){M_SP2.integral =  M_SP2.itl_lim_dn;}}
		}
			else M_SP2.integral=0;//ֵ֣Ϊ0
		
		M_SP2.dif=M_SP2.err-M_SP2.err_last;
    M_SP2.out = M_SP2.kp*M_SP2.err + M_SP2.ki*M_SP2.integral + M_SP2.kd*M_SP2.dif;   //λʽPID
    M_SP2.err_last = M_SP2.err;   //һƫ 
		
		
		if(1){//Ƿ޷
    if(M_SP2.out >= M_SP2.limit_up){M_SP2.out =  M_SP2.limit_up;}//޷
		else if(M_SP2.out <= M_SP2.limit_dn){M_SP2.out =  M_SP2.limit_dn;}}
		return M_SP2.out;
}

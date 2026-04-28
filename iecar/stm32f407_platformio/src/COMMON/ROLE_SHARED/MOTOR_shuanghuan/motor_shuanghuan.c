#include "motor_shuanghuan.h"
int m_exp_val=0;
int m_enc_val;
int m_pwm_out;
int enc_sum=0;
int zjbl=0;
float test;
PID_wz m_speed;
PID_wz m_position;
void motor_pid_init(void)
{
	m_speed.kp=0.5;
	m_speed.ki=0;
	m_speed.kd=0;
	m_speed.itl_lim_up=3000;
	m_speed.itl_lim_dn=-3000;
	m_speed.limit_up=500;
	m_speed.limit_dn=-500;
	
	m_position.kp=0.04;
	m_position.ki=0.01;
	m_position.kd=0;
	m_speed.itl_lim_up=60000;
	m_speed.itl_lim_dn=-60000;
	m_position.limit_up=500;
	m_position.limit_dn=-500;
}

// λʽPID
float PID_WZ_M_TEST(float pid_now,float pid_anti)
{
    m_speed.err =pid_anti-pid_now; // 㵱ǰ
    if(1) //
		{m_speed.integral += m_speed.err;      //ƫĻ
		if(1)//޷
			{if(m_speed.integral >= m_speed.itl_lim_up){m_speed.integral =  m_speed.itl_lim_up;}//޷
			else if(m_speed.integral <= m_speed.itl_lim_dn){m_speed.integral =  m_speed.itl_lim_dn;}}
		}
			else m_speed.integral=0;//ֵ֣Ϊ0
		
		m_speed.dif=m_speed.err-m_speed.err_last;
    m_speed.out = m_speed.kp*m_speed.err + m_speed.ki*m_speed.integral + m_speed.kd*m_speed.dif;   //λʽPID
    m_speed.err_last = m_speed.err;   //һƫ 
		
		
		if(1){//Ƿ޷
    if(m_speed.out >= m_speed.limit_up){m_speed.out =  m_speed.limit_up;}//޷
		else if(m_speed.out <= m_speed.limit_dn){m_speed.out =  m_speed.limit_dn;}}
		return m_speed.out;
}

// ʽPID
float PID_ZL_M_TEST(float pid_now,float pid_anti)
{
	m_position.err =pid_anti-pid_now; // 㵱ǰ
	if(PEin(7)==1) //
	{m_position.integral += m_position.err;      //ƫĻ
	if(1)//޷
		{if(m_position.integral >= m_position.itl_lim_up){m_position.integral =  m_position.itl_lim_up;}//޷
		else if(m_position.integral <= m_position.itl_lim_dn){m_position.integral =  m_position.itl_lim_dn;}}
	}
		else m_position.integral=0;//ֵ֣Ϊ0
	if(m_position.err<1500&&m_position.err>-1500) m_position.integral=0;//ַ
	m_position.dif=m_position.err-m_position.err_last;
	m_position.out = m_position.kp*m_position.err + m_position.ki*m_position.integral + m_position.kd*m_position.dif;   //λʽPID
	m_position.err_last = m_position.err;   //һƫ 
	
	
	if(1){//Ƿ޷
	if(m_position.out >= m_position.limit_up){m_position.out =  m_position.limit_up;}//޷
	else if(m_position.out <= m_position.limit_dn){m_position.out =  m_position.limit_dn;}}
	return m_position.out;
}

void PWM_REA_OUT(int a)
{
	if(a>=999) a=999;
	else if(a<=-999) a=-999;
	if(a>0) {PEout(14)=1;PEout(15)=0;}
	else {PEout(14)=0;PEout(15)=1;}
	TIM_SetCompare4(TIM8,gxt_abs(a));
}





void motor_init(u32 arr,u32 psc)
{
		GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_14|GPIO_Pin_15;        
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;        //ù
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//ٶ100MHz
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //츴
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;      
		GPIO_Init(GPIOE,&GPIO_InitStructure);     
		
		PEout(7)=0;
		PEout(14)=0;
		PEout(15)=0;
	
		TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
		TIM_OCInitTypeDef  TIM_OCInitStructure;
	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8,ENABLE);  
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	
		GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_TIM8); 
	
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;           
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //ù
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//ٶ100MHz
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //츴
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;        //
		GPIO_Init(GPIOC,&GPIO_InitStructure);              //ʼPF9
	
		TIM_TimeBaseStructure.TIM_Prescaler=psc;  //ʱƵ
		TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //ϼģʽ
		TIM_TimeBaseStructure.TIM_Period=arr;   //Զװֵ
		TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;
		TIM_TimeBaseInit(TIM8,&TIM_TimeBaseStructure);//ʼʱ14
	
		//ʼTIM14 Channel1 PWMģʽ	 
		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //ѡʱģʽ:TIMȵģʽ2
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //Ƚʹ
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //:TIMȽϼԵ
		TIM_OC4Init(TIM8, &TIM_OCInitStructure);  //TָĲʼTIM1 4OC1

 
		TIM_OC4PreloadConfig(TIM8, TIM_OCPreload_Enable);  //ʹTIM14CCR1ϵԤװؼĴ

		TIM_ARRPreloadConfig(TIM8,ENABLE);//ARPEʹ
		TIM_CtrlPWMOutputs(TIM8, ENABLE);
		TIM_Cmd(TIM8, ENABLE);  //ʹTIM14
		TIM_SetCompare4(TIM8,300);	
}


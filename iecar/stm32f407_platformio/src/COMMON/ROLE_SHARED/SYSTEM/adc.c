#include "adc.h"

#define SR04_threshold 300
#define SR04_T 3
u8 SR04_VAL=0;
u8 CAR_SR04(void)
{
	int adc_v;
	u8 i=0;
	for(i=0;i<SR04_T;i++)
	{
		adc_v=Get_Adc_Average(ADC_Channel_5,5,1);
		if(adc_v>=SR04_threshold) break;
	}
	if(i<SR04_T) {SR04_VAL=0;return 0;}
	else {SR04_VAL=1;return 1;}
}

void  Adc_Init(void)
{    
  GPIO_InitTypeDef  GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef       ADC_InitStructure;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹGPIOAʱ
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); //ʹADC1ʱ

  //ȳʼADC1ͨ5 IO
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;//PA5 ͨ5
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//ģ
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;//
  GPIO_Init(GPIOA, &GPIO_InitStructure);//ʼ  
 
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,ENABLE);	  //ADC1λ
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,DISABLE);	//λ	 
	
	
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;//ģʽ
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;//׶֮ӳ5ʱ
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //DMAʧ
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;//ԤƵ4ƵADCCLK=PCLK2/4=84/4=21Mhz,ADCʱòҪ36Mhz 
  ADC_CommonInit(&ADC_CommonInitStructure);//ʼ
	
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;//12λģʽ
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;//ɨģʽ	
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;//رת
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//ֹ⣬ʹ
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//Ҷ	
  ADC_InitStructure.ADC_NbrOfConversion = 1;//1תڹ Ҳֻת1 
  ADC_Init(ADC1, &ADC_InitStructure);//ADCʼ
	
 
	ADC_Cmd(ADC1, ENABLE);//ADת	

}	

//ADCֵ
//ch: @ref ADC_channels 
//ֵͨ 0~16ȡֵΧΪADC_Channel_0~ADC_Channel_16
//ֵ:ת
u16 Get_Adc(u8 ch)   
{
	  	//ָADCĹͨһУʱ
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles );	//ADC1,ADCͨ,480,߲ʱ߾ȷ			    
  
	ADC_SoftwareStartConv(ADC1);		//ʹָADC1ת	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//ȴת

	return ADC_GetConversionValue(ADC1);	//һADC1ת
}
//ȡͨchתֵȡtimes,Ȼƽ 
//ch:ͨ
//times:ȡ
//ֵ:ͨchtimesתƽֵ
u16 Get_Adc_Average(u8 ch,u8 times,u8 delay_t)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_us(delay_t*200);
	}
	return temp_val/times;
} 
	 

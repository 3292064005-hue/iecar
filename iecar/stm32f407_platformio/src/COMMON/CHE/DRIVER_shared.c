#include "DRIVER.h"
#include "car_mainchain.h"
void CAR_Driver_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
}

/*
 * Configure the task selection switches on PE2/PE3.
 * Input pull-ups are enabled so an open switch reads logic high.
 */
void CAR_KAIGUAN_INIT(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

/*
 * Set the onboard RGB state.
 * a: color selector defined in DRIVER.h.
 */
void RGB_EN(u8 a)
{
	switch(a)
	{
		case RED:    RED_EN;  GREEN_DIS; BLUE_DIS; break;
		case WHITE:  RED_EN;  GREEN_EN;  BLUE_EN;  break;
		case DARK:   RED_DIS; GREEN_DIS; BLUE_DIS; break;
		case GREEN:  RED_DIS; GREEN_EN;  BLUE_DIS; break;
		case YELLOW: RED_EN;  GREEN_EN;  BLUE_DIS; break;
		case BLUE:   RED_DIS; GREEN_DIS; BLUE_EN;  break;
		case PURPLE: RED_EN;  GREEN_DIS; BLUE_EN;  break;
		case CYAN:   RED_DIS; GREEN_EN;  BLUE_EN;  break;
		default:     RED_DIS; GREEN_DIS; BLUE_DIS; break;
	}
}

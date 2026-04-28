#ifndef ws2812_h__
#define ws2812_h__
#include "sys.h"

#define ws_time 40

//û޸Ĵ
#define WS_TIMES 6//оƬƵйأstm32f4076Լ£ʾ 
#define TH	PEout(4)=1;
#define TL	PEout(4)=0;

//RGB--˼׼rgbֵ 
#define WS_DARK 	0,0,0
#define WS_WHITE 	255,255,255
#define WS_RED 		255,0,0
#define WS_GREEN 	0,255,0
#define WS_BLUE 	0,0,255
#define WS_YELLO 	255,255,0
#define WS_PURPLE   255,0,255
#define WS_CYAN 	0,255,255

//extern u8 ws_data[];Ҫطõ飬Ͳע 

void ws2812_init(void);
void ws2812_rgb(u8 ws_i,u8 ws_r,u8 ws_g,u8 ws_b);
void ws2812_rgb_all(u8 ws_i,u8 ws_r,u8 ws_g,u8 ws_b);
void ws2812_refresh(u8 ws_i);
void ws2812_reset(u8 ws_set);


void delay_250ns(void);
void send_0(void);
void send_1(void);
void send_res(void);

#endif //ws2812_h__


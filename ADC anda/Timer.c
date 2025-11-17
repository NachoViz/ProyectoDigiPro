// SEÑAL CUADRADA QUE ALTERNA CADA 1 ms

*/

#include "LPC17xx.h"

#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define ADC_CONV_RATE 200000

void configADCPin(void);
void configADC_Drivers(void);

#define	OUTPUT	    (uint8_t) 1
#define INPUT	    (uint8_t) 0

#define PIN_22		((uint32_t)(1<<22))
#define PORT_ZERO	(uint8_t)	0
#define PORT_ONE	(uint8_t)	1
#define PORT_TWO	(uint8_t)	2
#define PORT_THREE	(uint8_t)	3

void config_GPIO(void);
void config_timer(void);

void TIMER0_IRQHandler(void){


    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)){

        ADC_StartCmd(LPC_ADC, ADC_START_NOW);   //adc se activa cada 1ms
    	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    }
}

int main(void) {

	config_GPIO();
	config_timer();
	GPIO_SetValue(PORT_ZERO,PIN_22);

    LPC_SC->PCLKSEL0 |= (3 << 24);
    
    configADCPin();
    configADC_Drivers();


	while(1) {
		
	}
    return 0 ;
}

void config_GPIO(){
	PINSEL_CFG_Type pin_configuration;

	pin_configuration.Portnum 	=	PINSEL_PORT_0;
	pin_configuration.Pinnum	=	PINSEL_PIN_22;
	pin_configuration.Pinmode	=	PINSEL_PINMODE_PULLUP;
	pin_configuration.Funcnum	= 	PINSEL_FUNC_0;
	pin_configuration.OpenDrain	=	0;

	PINSEL_ConfigPin(&pin_configuration);

	GPIO_SetDir( PORT_ZERO , PIN_22 , OUTPUT );

	return;
}

void configADCPin(void) {
    PINSEL_CFG_Type PinCfg;
    
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 25;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    
    LPC_PINCON->PINMODE1 |= (3 << 18);
}

void configADC_Drivers(void) {
    
    ADC_Init(LPC_ADC, ADC_CONV_RATE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE); 
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN2, ENABLE);
    ADC_BurstCmd(LPC_ADC, DISABLE);
    NVIC_EnableIRQ(ADC_IRQn);
}

void ADC_IRQHandler(void) {
    uint32_t resultado;
    
    if (ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)) {
        
        resultado = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
        
        if(resultado>=0 && resultado < 1365){
            GPIO_SetValue(0, (1 << 22));
            GPIO_ClearValue(3 , ( 1 << 25));
            GPIO_ClearValue(3 , ( 1 << 26));
        }else if(resultado>=1365 && resultado<2730){
            GPIO_SetValue(3, (1 << 25));
            GPIO_ClearValue(0 , ( 1 << 22));
            GPIO_ClearValue(3 , ( 1 << 26));
        }else if(resultado>=2730 && resultado<4095){
            GPIO_SetValue(3, (1 << 26));
            GPIO_ClearValue(0 , ( 1 << 22));
            GPIO_ClearValue(3 , ( 1 << 25));
        }
    }
    
}

void config_timer(){
	TIM_TIMERCFG_Type	struct_config;
	TIM_MATCHCFG_Type	struct_match;

	struct_config.PrescaleOption	=	TIM_PRESCALE_USVAL;
	struct_config.PrescaleValue		=	1;

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &struct_config);
	
    struct_match.MatchChannel		=	0;
	struct_match.IntOnMatch			=	ENABLE;
	struct_match.ResetOnMatch		=	ENABLE;
	struct_match.StopOnMatch		=	DISABLE;
	struct_match.ExtMatchOutputType	=	TIM_EXTMATCH_NOTHING;
	struct_match.MatchValue			=	1000;


	TIM_ConfigMatch(LPC_TIM0, &struct_match);

	NVIC_EnableIRQ(TIMER0_IRQn);
	
	TIM_Cmd(LPC_TIM0, ENABLE);

	return;
}
/*	Drivers
 *  Created on: 14 october 2025
 *      Author: jnach
 *      
 *      
 */
#include "lpc17xx_gpio.h"
#include "lpc17xx_nvic.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_uart.h"

void configPotenciometro();

uint32_t adc_value;
uint32_t valor_match_pwm;

int main(){
    confiGIO();  
    configTimer();

    while(1) {
    }

    return 0;
}

void configGPIO(){

    LPC_GPIO0->FIODIR |= (1 << 6 ); 

}

void configTimer(void){
    GPIO_SetDir(1 , (1 << 28), 0);
    PINSEL_CFG_Type pinty = {PINSEL_PORT_1, PINSEL_PIN_28, PINSEL_FUN_2, PINSEL_PINMODE_TRISTATE , PINSEL_PINMODE_NORMAL};
    PINSEL_ConfigPin(&pinty);

    GPIO_SetDir(1 , (1 << 29), 0);
    PINSEL_CFG_Type pinty1 = {PINSEL_PORT_1, PINSEL_PIN_29, PINSEL_FUN_2, PINSEL_PINMODE_TRISTATE , PINSEL_PINMODE_NORMAL};
    PINSEL_ConfigPin(&pinty1);
    

    TIM_TIMERCFG_Type timty = {TIM_PRESCALE_USVAL , 1};
    TIM_MATCHCFG_Type matchty = {0 , ENABLE , DISABLE , ENABLE , TIM_EXTMATCH_NOTHING , 20000};

    TIM_MATCHCFG_Type matchty1 = {1 , ENABLE , DISABLE , DISABLE , TIM_EXTMATCH_NOTHING , 1};

    TIM_Init(LPC_TIM0 , TIM_TIMER , &timty);
    
    TIM_ConfigMatch(LPC_TIM0 , &matchty);
    TIM_ConfigMatch(LPC_TIM0 , &matchty1);


    TIM_Cmd(LPC_TIM0, ENABLE);

    NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT) == 1) {

        LPC_GPIO0->FIOSET  |= (1 << 6); 

        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    }
}
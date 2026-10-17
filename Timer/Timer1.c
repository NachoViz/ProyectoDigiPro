/*	Drivers
 *  Created on: 14 october 2025
 *      Author: jnach
 *      
 *      
 */
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

void configGPIO(){

    PINSEL_CFG_Type pinPWM;
    pinPWM.PortNum = PINSEL_PORT_1;
    pinPWM.PinNum = PINSEL_PIN_27; // Pin P1.27
    pinPWM.FuncNum = PINSEL_FUNC_3; // Función 3: TIM0_MAT1
    pinPWM.PinMode = PINSEL_PINMODE_PULLUP;
    pinPWM.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pinPWM);

}


void configTimer(void){
    TIM_TIMERCFG_Type timerCfg;
    TIM_MATCHCFG_Type matchCfg;
    TIM_MATCHCFG_Type matchCfg1;


    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    // Configurar MR0 para el período de la señal: 20ms (frecuencia de 50Hz)
    matchCfg.MatchChannel = 0;
    matchCfg.MatchValue = 20000;      // 20000 µs = 20 ms
    matchCfg.ResetOnMatch = ENABLE;   // Resetear el contador para repetir el ciclo
    matchCfg.IntOnMatch = DISABLE;    // No necesitamos interrupción
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    // Configurar MR1 para el ancho del pulso inicial (ej. 90 grados = 1.5ms)
    matchCfg1.MatchChannel = 1;
    matchCfg1.MatchValue = 1500;       // 1500 µs = 1.5 ms
    matchCfg1.ResetOnMatch = DISABLE;
    matchCfg1.IntOnMatch = DISABLE;    // No necesitamos interrupción
    matchCfg1.StopOnMatch = DISABLE;
    matchCfg1.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    TIM_ConfigMatch(LPC_TIM0, &matchCfg1);

    TIM_Cmd(LPC_TIM0, ENABLE);

}


int main(){
    configGPIO();
    configTimer();
    while(1);
    return 0;
}   
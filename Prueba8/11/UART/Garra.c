/*
    //SOLAMENTE INCORPORA TIMER0 Y INTERRUPCION EXTERNA PARA EL BOTON
 */

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_nvic.h"
#include "lpc17xx_uart.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define SERVO_GARRA   12     // P2.12

#define SERVO_MIN_US 1000
#define SERVO_MAX_US 2000
volatile uint8_t contadorBoton; 

volatile uint32_t pulsoGARRA = 0;

// ---------------- GPIO ----------------
void ConfGPIO(void) {
    PINSEL_CFG_Type PinCfg;
    PinCfg.Portnum = 2;
    PinCfg.Pinnum = SERVO_GARRA;
    PinCfg.Funcnum = 0;
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PinCfg.OpenDrain = 0;
    PINSEL_ConfigPin(&PinCfg);

    //BOTON CONFIGURADO POR BAJO(LO APRETO Y DA UN LOW)
    // PONERLO A MASA Y AL TENER LA PULLUP SIEMPRE VA A ESTAR EN 1 HASTA QUE LO APRETE
    //HABILITO INTERRUPCION EXT
    PinCfg.Pinnum = 11;
    PinCfg.Funcnum = 1;
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PINSEL_ConfigPin(&PinCfg);

    EXTI_Init();


    EXTI_InitTypeDef EXTI_Cfg = {0};
    EXTI_Cfg.EXTI_Line = 1;
    EXTI_Cfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    EXTI_Cfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
    EXTI_Config(EXTI_Cfg);
    NVIC_EnableIRQ(EXTI1_IRQn);

    GPIO_SetDir(0, (1 << SERVO_GARRA), 1);
}

// ---------------- EXTI1_HANDLER ----------------
void EXTI1_IRQHandler(void){
    EXTI_ClearEXTIFlag(EXTI_EINT1);
    contadorBoton++;
    if(contadorBoton = 1){
        TIM_UpdateMatchValue(LPC_TIM0, 1, 2000);
    }else{
        contadorBoton = 0; 
        TIM_UpdateMatchValue(LPC_TIM0, 1, 1000);

    }
}

// ---------------- TIMER0 ----------------
void ConfTIMER0(void) {
    TIM_TIMERCFG_Type timerCfg;
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1;

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    TIM_MATCHCFG_Type matchy0 = {0};
    matchy0.MatchChannel = 0;
    matchy0.IntOnMatch = ENABLE;
    matchy0.ResetOnMatch = ENABLE;
    matchy0.MatchValue = 20000;
    TIM_ConfigMatch(LPC_TIM0, &matchy0);
    
    TIM_MATCHCFG_Type match1 = {0};
    match1.MatchChannel = 1;
    match1.IntOnMatch = ENABLE;
    match1.MatchValue = 1000;
    TIM_ConfigMatch(LPC_TIM0, &match1);

    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Cmd(LPC_TIM0, ENABLE);
}

// ---------------- TIMER2 IRQ ----------------
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {

        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        GPIO_SetValue(0, (1 << SERVO_GARRA));

    }
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT)) {
        TIM_ClearIntPending(LPC_TIM2, TIM_MR1_INT);
        GPIO_ClearValue(0, (1 << SERVO_GARRA));
    }
}

// ---------------- MAIN ----------------
int main(void) {
    ConfGPIO();
    ConfTIMER0();

    while (1);

}
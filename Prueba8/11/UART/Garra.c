#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_nvic.h"
#include <stdint.h>

// ------------------------------
// Pines
// ------------------------------
#define SERVO_PORT 2
#define SERVO_PIN  12    // P2.12

#define BOTON_PORT 2
#define BOTON_PIN  10    // P2.10

volatile uint8_t estado = 0;

// ------------------------------
// GPIO: Servo como salida
// ------------------------------
void ConfGPIO(void) {
    PINSEL_CFG_Type p;

    // P2.12 como GPIO (función 0)
    p.Portnum = 2;
    p.Pinnum = SERVO_PIN;
    p.Funcnum = 0;
    p.Pinmode = PINSEL_PINMODE_PULLUP;
    p.OpenDrain = 0;
    PINSEL_ConfigPin(&p);

    GPIO_SetDir(2, (1 << SERVO_PIN), 1); // salida

    // ------------------------
    // Botón en P2.10 como EINT0
    // ------------------------
    p.Pinnum = BOTON_PIN;
    p.Funcnum = 1; // EINT0
    p.Pinmode = PINSEL_PINMODE_PULLUP;
    PINSEL_ConfigPin(&p);

    EXTI_Init();

    EXTI_InitTypeDef exti;
    exti.EXTI_Line = 0;  // EINT0
    exti.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    exti.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
    EXTI_Config(&exti);

    NVIC_EnableIRQ(EINT0_IRQn);
}

// ------------------------------
// TIMER0: genera PWM en P2.12
// ------------------------------
void ConfTimer0(void) {
    TIM_TIMERCFG_Type cfg;
    cfg.PrescaleOption = TIM_PRESCALE_USVAL; // 1 tick = 1us
    cfg.PrescaleValue = 1;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfg);

    // MATCH0 = periodo = 20 ms
    TIM_MATCHCFG_Type m0;
    m0.MatchChannel = 0;
    m0.IntOnMatch = ENABLE;
    m0.ResetOnMatch = ENABLE;
    m0.StopOnMatch = DISABLE;
    m0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    m0.MatchValue = 20000;
    TIM_ConfigMatch(LPC_TIM0, &m0);

    // MATCH1 = pulso del servo (inicial 1500)
    TIM_MATCHCFG_Type m1;
    m1.MatchChannel = 1;
    m1.IntOnMatch = ENABLE;
    m1.ResetOnMatch = DISABLE;
    m1.StopOnMatch = DISABLE;
    m1.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    m1.MatchValue = 1500;
    TIM_ConfigMatch(LPC_TIM0, &m1);

    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Cmd(LPC_TIM0, ENABLE);
}

// ------------------------------
// ISR TIMER0
// ------------------------------
void TIMER0_IRQHandler(void) {
    // Inicio del ciclo → poner servo en alto
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        GPIO_SetValue(SERVO_PORT, (1 << SERVO_PIN));
    }

    // Fin del pulso → poner servo en bajo
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);
        GPIO_ClearValue(SERVO_PORT, (1 << SERVO_PIN));
    }
}

// ------------------------------
// ISR BOTÓN
// ------------------------------
void EINT0_IRQHandler(void) {
    EXTI_ClearEXTIFlag(EXTI_EINT0);

    estado ^= 1; // alternar

    if (estado == 1)
        TIM_UpdateMatchValue(LPC_TIM0, 1, 2000); // ABRE
    else
        TIM_UpdateMatchValue(LPC_TIM0, 1, 1000); // CIERRA
}

// ------------------------------
// MAIN
// ------------------------------
int main(void) {
    ConfGPIO();
    ConfTimer0();
    while (1) { }
}

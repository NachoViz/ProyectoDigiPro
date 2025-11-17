/* Drivers
 * Created on: 14 october 2025
 * Author: jnach
 */

// Includes necesarios
#include "lpc17xx_gpio.h"
#include "lpc17xx_nvic.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"     // <-- CORREGIDO: Faltaba el include de UART
#include <string.h>         // <-- CORREGIDO: Faltaba el include para strlen()

const char mensaje[] = "Holamundo";

// Prototipos de funciones
void configGPIO(void);
void configTimer(void);     // <-- CORREGIDO: El prototipo era configST()
void configUART(void);

int main() {
    configGPIO();
    configTimer();
    configUART();

    while(1) {
        // El bucle principal queda vacío, todo se maneja por interrupción.
    }
    return 0; // <-- NOTA: Buena práctica
}

void configGPIO() {
    // Configuración de P0.2 como TXD0 (Función 1)
    PINSEL_CFG_Type psel_conf;
    psel_conf.Portnum = 0;
    psel_conf.Pinnum = 2;
    psel_conf.Pinmode = PINSEL_PINMODE_TRISTATE; // Sin pull-up/pull-down
    psel_conf.Funcnum = 1;                     // <-- NOTA: 1 es TXD0
    psel_conf.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&psel_conf);

    // Configuración de P0.3 como RXD0 (Función 1)
    psel_conf.Portnum = 0;
    psel_conf.Pinnum = 3;
    psel_conf.Pinmode = PINSEL_PINMODE_TRISTATE; // Sin pull-up/pull-down
    psel_conf.Funcnum = 1;                     // <-- NOTA: 1 es RXD0
    psel_conf.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&psel_conf);

    // <-- NOTA: Los GPIO_SetDir() no son necesarios
    // cuando configuras el PINSEL en una función que no es GPIO.
}

void configUART() {
    UART_CFG_Type uart_cfg = {0};
    UART_FIFO_CFG_Type fifo_cfg = {0};

    // Configuración estándar: 9600, 8N1
    UART_ConfigStructInit(&uart_cfg); // Inicializa con valores por defecto (9600 8N1)
    uart_cfg.Baud_rate = 9600;

    // Inicializar la UART0
    UART_Init(LPC_UART0, &uart_cfg);

    // Configurar la FIFO
    UART_FIFOConfigStructInit(&fifo_cfg); // Inicializa con valores por defecto

    // <-- NOTA: Tu configuración manual estaba bien,
    // pero usar la función StructInit es más seguro.
    /*
    fifo_cfg.FIFO_ResetRxBuf = ENABLE;
    fifo_cfg.FIFO_ResetTxBuf = ENABLE;
    fifo_cfg.FIFO_DMAMODE = DISABLE;
    fifo_cfg.FIFO_Level = UART_FIFO_TRGLEV3;
    */

    UART_FIFOConfig(LPC_UART0, &fifo_cfg); // <-- CORREGIDO: Usar LPC_UART0

    // Habilitar la transmisión
    UART_TxCmd(LPC_UART0, ENABLE);
}

void configTimer() {
    TIM_TIMERCFG_Type tim_cfg;
    TIM_MATCHCFG_Type match_cfg;

    // Configurar el Timer 0
    tim_cfg.PrescaleOption = TIM_PRESCALE_USVAL;
    tim_cfg.PrescaleValue = 1; // <-- CORREGIDO: 1 para ticks de 1us (asumiendo PCLK=25MHz)
                               // Si querías 1MHz, tu valor de 1000000 era incorrecto.

    // Configurar el Match 0 para 5 segundos
    match_cfg.MatchChannel = 0;
    match_cfg.IntOnMatch = ENABLE;
    match_cfg.ResetOnMatch = ENABLE;  // <-- CORREGIDO: Debe reiniciarse para ser periódico
    match_cfg.StopOnMatch = DISABLE;  // <-- CORREGIDO: No debe parar
    match_cfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match_cfg.MatchValue = 5000000;   // <-- CORREGIDO: 5,000,000 us = 5 segundos

    // Inicializar Timer y Match
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &tim_cfg);
    TIM_ConfigMatch(LPC_TIM0, &match_cfg);

    // Habilitar el Timer
    TIM_Cmd(LPC_TIM0, ENABLE);

    // Habilitar la interrupción del Timer0 en el NVIC
    NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler(void) {
    // 1. Limpiar la bandera de interrupción DEL PERIFÉRICO (Timer)
    // Esto es fundamental para que no se llame a la ISR en bucle.
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT); // <-- CORREGIDO: ¡Muy importante!

    // 2. Enviar el mensaje por UART
    UART_Send(LPC_UART0, (uint8_t *)mensaje, strlen(mensaje), BLOCKING);
    // <-- CORREGIDO: (uint8_t *) cast, y quitado el "- 1" de strlen

    // 3. NO es necesario limpiar el NVIC manualmente aquí.
    // NVIC_ClearPendingIRQ(TIMER0_IRQn); // <-- CORREGIDO: Borrar esta línea.
}

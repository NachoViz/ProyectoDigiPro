/*
        IMPORTANTISISISISISMO

        EL ECHO DEVUELVE UN PULSO DE 5V OSEA QUE VA A QUEMAR LA PLACA 

        NECESITO PONERLE UN DIVISION DE FRECUENCIA

            El Resumen de Pines
    VCC (Sensor): ENTRADA de alimentación (aquí sí conectás tu fuente externa de 5V).

    GND (Sensor): ENTRADA de tierra (conectar a la fuente externa Y al GND del LPC).

    Trig (Sensor): ENTRADA de señal (recibe el pulso de 3.3V desde el LPC).

    Echo (Sensor): SALIDA de señal (envía un pulso de 5V hacia el LPC).
 */

 #include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"
#include <stdio.h>
#include <string.h>

// --- Pines ---
#define TRIGGER_PORT 0
#define TRIGGER_PIN  (1 << 22) // P0.22 (LED de la placa)
#define ECHO_PORT    1
#define ECHO_PIN     (1 << 26) // P1.26 (CAP0.0)

// --- Variables Globales para la Interrupción ---
static uint32_t startTime = 0;
static uint8_t  state = 0; // 0 = esperando flanco de subida, 1 = esperando flanco de bajada
volatile uint32_t pulseDuration_us = 0;
volatile int measurement_ready = 0;

// --- Prototipos ---
void configTriggerPin(void);
void configEchoCapture(void);
void configUART_simple(void);
void sendTriggerPulse(void);
void delay_ms(uint32_t ms);

// -----------------------------------------------------------------
// FUNCIÓN PRINCIPAL
// -----------------------------------------------------------------
int main(void) {
    configTriggerPin();
    configEchoCapture();
    configUART_simple();

    while(1) {
        // 1. Enviar el pulso de 10us para iniciar la medición
        sendTriggerPulse();
        
        // 2. Esperar a que la interrupción del Timer termine de medir
        while (!measurement_ready);
        measurement_ready = 0; // Resetear la bandera

        // 3. Calcular la distancia
        // Distancia (cm) = (Duración en µs) / 58
        uint32_t distance_cm = pulseDuration_us / 58;

        // 4. Enviar el resultado por UART
        char buffer[64];
        sprintf(buffer, "Distancia: %d cm\r\n", distance_cm);
        UART_Send((LPC_UART_TypeDef *)LPC_UART0, (uint8_t *)buffer, strlen(buffer), BLOCKING);

        // 5. Esperar antes de la próxima medición
        delay_ms(500);
    }
    return 0;
}

// -----------------------------------------------------------------
// HANDLER DE INTERRUPCIÓN DEL TIMER0 (AQUÍ OCURRE LA MEDICIÓN)
// -----------------------------------------------------------------
void TIMER0_IRQHandler(void) {
    // Verificar si la interrupción fue por Captura en el Canal 0
    if (TIM_GetIntStatus(LPC_TIM0, TIM_CR0_INT) == SET) {
        
        if (state == 0) {
            // --- 1. Se capturó el FLANCO DE SUBIDA (Inicio del pulso) ---
            
            // Guardar el tiempo de inicio
            startTime = LPC_TIM0->CR0;
            
            // Reconfigurar el Timer para capturar el FLANCO DE BAJADA
            LPC_TIM0->CCR = (1 << 1) | (1 << 2); // CAP0.0 Falling Edge + Interrupt
            
            state = 1; // Mover al siguiente estado
            
        } else if (state == 1) {
            // --- 2. Se capturó el FLANCO DE BAJADA (Fin del pulso) ---
            
            uint32_t endTime = LPC_TIM0->CR0;
            
            // Calcular la duración (considerando desbordamiento del timer)
            if (endTime > startTime) {
                pulseDuration_us = endTime - startTime;
            } else {
                pulseDuration_us = (0xFFFFFFFF - startTime) + endTime;
            }
            
            // Indicar al main() que la medición está lista
            measurement_ready = 1;
            
            // Reconfigurar el Timer para el FLANCO DE SUBIDA (para la próxima vez)
            LPC_TIM0->CCR = (1 << 0) | (1 << 2); // CAP0.0 Rising Edge + Interrupt
            
            state = 0; // Volver al estado inicial
        }
        
        // Limpiar la bandera de interrupción de captura
        TIM_ClearIntPending(LPC_TIM0, TIM_CR0_INT);
    }
}

// -----------------------------------------------------------------
// CONFIGURACIONES
// -----------------------------------------------------------------

void configTriggerPin(void) {
    // Configurar P0.22 como salida GPIO
    PINSEL_CFG_Type PinCfg;
    PinCfg.Portnum = TRIGGER_PORT;
    PinCfg.Pinnum = 22;
    PinCfg.Funcnum = 0; // Función GPIO
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PinCfg.OpenDrain = 0;
    PINSEL_ConfigPin(&PinCfg);
    
    GPIO_SetDir(TRIGGER_PORT, TRIGGER_PIN, 1); // 1 = Salida
    GPIO_ClearValue(TRIGGER_PORT, TRIGGER_PIN); // Asegurarse que empiece en BAJO
}

void configEchoCapture(void) {
    // --- Configurar el Pin P1.26 ---
    PINSEL_CFG_Type PinCfg;
    PinCfg.Portnum = ECHO_PORT;
    PinCfg.Pinnum = 26;
    PinCfg.Funcnum = 3; // Función CAP0.0
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE; // El pin de Echo no necesita pull-up/down
    PinCfg.OpenDrain = 0;
    PINSEL_ConfigPin(&PinCfg);
    
    // --- Configurar el Timer0 ---
    TIM_TIMERCFG_Type timerCfg;
    // Queremos que el tick del timer sea de 1 microsegundo (µs)
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);
    
    // --- Configurar la Captura ---
    // Iniciar esperando el flanco de subida
    LPC_TIM0->CCR = (1 << 0) | (1 << 2); // CAP0.0 Rising Edge + Interrupt
    
    // Habilitar la interrupción del Timer0 en el NVIC
    NVIC_EnableIRQ(TIMER0_IRQn);
    
    // Iniciar el Timer
    TIM_Cmd(LPC_TIM0, ENABLE);
}

void sendTriggerPulse(void) {
    // Crear un pulso ALTO de 10µs en el pin de Trigger
    GPIO_SetValue(TRIGGER_PORT, TRIGGER_PIN);
    
    // Pequeño retardo (busy-wait) para 10µs. 
    // Asumiendo 100MHz CCLK, ~1000 iteraciones.
    volatile uint32_t i;
    for(i=0; i < 1000; i++); 
    
    GPIO_ClearValue(TRIGGER_PORT, TRIGGER_PIN);
}

void configUART_simple(void) {
    PINSEL_CFG_Type psel_conf;
    psel_conf.Portnum = 0;
    psel_conf.Pinnum = 2; // TXD0
    psel_conf.Funcnum = 1;
    PINSEL_ConfigPin(&psel_conf);
    
    psel_conf.Pinnum = 3; // RXD0
    PINSEL_ConfigPin(&psel_conf);

    UART_CFG_Type uart_cfg;
    UART_ConfigStructInit(&uart_cfg); // 9600 8N1
    
    UART_Init((LPC_UART_TypeDef *)LPC_UART0, &uart_cfg);
    UART_TxCmd((LPC_UART_TypeDef *)LPC_UART0, ENABLE);
}

void delay_ms(uint32_t ms) {
    volatile uint32_t i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 7000; j++); // Bucle de retardo aproximado
    }
}
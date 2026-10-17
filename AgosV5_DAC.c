/*
 * Proyecto: Control de 3 servomotores con 3 potenciómetros - LPC1769
 *
 * Descripción:
 * Control de tres servos mediante tres potenciómetros usando ADC en modo burst
 * y generación de PWM por software con el Timer2.
 *
 * *** ADEMÁS: Muestreo de Pot 1 (AD0.4) por TIMER0 y salida por DAC usando DMA ***
 *
 * Conexiones:
 * - Potenciómetro 1 → P1.30 (AD0.4) → (Controla Servo 1 Y se muestrea al DAC)
 * - Potenciómetro 2 → P1.31 (AD0.5) → (Controla Servo 2)
 * - Potenciómetro 3 → P0.25 (AD0.2) → (Controla Servo 3)
 * - Servo 1 → P0.9
 * - Servo 2 → P0.8
 * - Servo 3 → P0.7
 * - Salida DAC → P0.26 (AOUT)
 */

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_DAC.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_nvic.h"
#include "lpc17xx_uart.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>


#define NUM_SINE_SAMPLE 128  // Mismo tamaño
#define SINE_FREQ_IN_HZ 50   // Frecuencia de actualización del DAC
#define PCLK_DAC_IN_MHZ 25   // CCLK/4
// Definiciones
#define SERVO_PIN   9   // P0.9  -> Servo 1
#define SERVO_PIN2  8   // P0.8  -> Servo 2
#define SERVO_PIN3  7   // P0.7  -> Servo 3

#define ADC_CH      4   // AD0.4 -> P1.30 -> Potenciómetro 1
#define ADC_CH5     5   // AD0.5 -> P1.31 -> Potenciómetro 2
#define ADC_CH2     2   // AD0.2 -> P0.25 -> Potenciómetro 3

// Variables globales
volatile uint16_t valorADC1 = 0;
volatile uint16_t valorADC2 = 0;
volatile uint16_t valorADC3 = 0;

volatile uint32_t pulso = 1500;
volatile uint32_t pulso2 = 1500;
volatile uint32_t pulso3 = 1500;

volatile uint32_t counter = 0;

// ✔ CAMBIO: Se necesita alinear los buffers para el DMA
volatile uint32_t bufferEntrada[128] __attribute__ ((aligned (4)));
volatile uint32_t bufferSalida[128] __attribute__ ((aligned (4)));

// ✔ CAMBIO: Se necesita una estructura LLI para el DMA circular
volatile GPDMA_LLI_Type dma_lli_dac;

volatile uint32_t i = 0;
volatile uint32_t tmp;

// Timer0 para muestreo/grabación
void ConfTIMER0(void)
{
    TIM_TIMERCFG_Type timerCfg;
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1; // Timer corre a 1MHz (1us)
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    TIM_MATCHCFG_Type matchCfg;
    matchCfg.MatchChannel = 0;
    matchCfg.IntOnMatch = ENABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    // Frecuencia de muestreo = 1MHz / 25 = 40kHz
    matchCfg.MatchValue = 1000;
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Cmd(LPC_TIM0, ENABLE);
}

void confDac(void){
    uint32_t tmp;
    DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;

    // ✔ CAMBIO: Configurar pin AOUT P0.26
    PINSEL_CFG_Type PinCfg;
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Funcnum = 2; // Función AOUT
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&PinCfg);

    DAC_ConverterConfigStruct.CNT_ENA = SET;
    DAC_ConverterConfigStruct.DMA_ENA = SET;
    DAC_Init(LPC_DAC);

    // Cálculo del timeout para la frecuencia deseada
    // Esto establece la velocidad a la que el DMA transfiere CADA muestra
    tmp = (PCLK_DAC_IN_MHZ * 1000000) / (SINE_FREQ_IN_HZ * NUM_SINE_SAMPLE);

    // Si queremos que la "onda" de 128 muestras se repita 50 veces por seg,
    // la frecuencia de actualización de cada muestra es 50 * 128 = 6400 Hz
    // Timeout = 25,000,000 / 6400 = 3906
    // El cálculo original es correcto.

    DAC_SetDMATimeOut(LPC_DAC, tmp);
    DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);
}

void TIMER0_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT))
    {
        // 1. Guarda la muestra actual (leída por el ADC_IRQHandler)
        bufferEntrada[i] = valorADC1;

        // 2. Al llenar el buffer, procesa y copia a la salida
        if(i == 127)
        {
            //GPIO_SetValue(0,1<<22); // Opcional para debug

            for (int j = 0; j < 128; j++) {
                uint32_t raw = bufferEntrada[j];

                // ✔ CAMBIO: 'raw' YA es el valor de 12 bits (0-4095).
                // No hay que hacer (raw >> 4).
                uint32_t val_adc = raw & 0xFFF;

                // Escala de 12 bits (ADC) a 10 bits (DAC)
                uint32_t val_dac = val_adc >> 2;

                // Prepara el valor para el registro DACR (bits 15:6)
                bufferSalida[j] = (val_dac << 6);
            }

            GPIO_ClearValue(0,1<<22); // Opcional para debug
        }

        i = (i + 1) % 128; // Avanza el índice del buffer de entrada

        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    }
}


void DMA_IRQHandler(void) {

    // ✔ CAMBIO: La interrupción para el canal 1 (DAC) ya no es necesaria.
    // El LLI lo hace circular automáticamente por hardware.
    /*
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 1)) {
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 1);
        GPDMA_ChannelCmd(1, ENABLE); // Esto estaba mal, solo reiniciaba
    }
    */

    // Canal 0 no se usa, pero lo dejas por si acaso
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
        GPIO_ClearValue(0,1<<22);
    }

    if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
    }
     if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 1)) { // Agregado por si hay error en Ch1
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 1);
    }
}

void confDMA_DAC(void){

	GPDMA_Init();

    // ✔ CAMBIO: Configurar el LLI (Linked List Item) para DMA circular
    dma_lli_dac.SrcAddr = (uint32_t)bufferSalida;
    dma_lli_dac.DstAddr = 0; // Se ignora, usamos DstConn = GPDMA_CONN_DAC
    dma_lli_dac.NextLLI = (uint32_t)&dma_lli_dac; // <-- ¡La clave! Apunta a sí mismo
    dma_lli_dac.Control = (128) // TransferSize
                        | (GPDMA_BSIZE_1 << 12)     // SrcBurstSize
                        | (GPDMA_BSIZE_1 << 15)     // DstBurstSize
                        | (GPDMA_WIDTH_WORD << 18)  // SrcWidth = 32 bit
                        | (GPDMA_WIDTH_WORD << 21)  // DstWidth = 32 bit
                        | (1 << 26)                 // SrcIncrement = SÍ
                        | (0 << 27)                 // DstIncrement = NO
                        | (0 << 31);                // Terminal count interrupt = NO

	GPDMA_Channel_CFG_Type GPDMACfg;
    // Configurar Canal 1
    GPDMACfg.ChannelNum = 1;
    GPDMACfg.SrcMemAddr = 0; // Se ignora, se usa LLI
    GPDMACfg.DstMemAddr = 0; // Se ignora, se usa LLI
    GPDMACfg.TransferSize = 128; // Se ignora, se usa LLI
    GPDMACfg.TransferWidth = 0; // Se ignora, se usa LLI
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P; // Memoria a Periférico
    GPDMACfg.SrcConn = 0;
    GPDMACfg.DstConn = GPDMA_CONN_DAC; // Destino: DAC
    GPDMACfg.DMALLI = (uint32_t)&dma_lli_dac; // Apunta al LLI

    GPDMA_Setup(&GPDMACfg);
    GPDMA_ChannelCmd(1, ENABLE);

    // Habilitar interrupciones DMA (solo para errores, ya no para TC)
    NVIC_EnableIRQ(DMA_IRQn);
}


// Config UART
void configGPIO_UART(void) {
    PINSEL_CFG_Type psel_conf;
    psel_conf.Portnum = 0;
    psel_conf.Pinmode = PINSEL_PINMODE_TRISTATE;
    psel_conf.OpenDrain = PINSEL_PINMODE_NORMAL;
    psel_conf.Pinnum = 2; // TXD0
    psel_conf.Funcnum = 1;
    PINSEL_ConfigPin(&psel_conf);
    psel_conf.Pinnum = 3; // RXD0
    PINSEL_ConfigPin(&psel_conf);
}

void configUART(void) {
    UART_CFG_Type uart_cfg;
    UART_FIFO_CFG_Type fifo_cfg;
    UART_ConfigStructInit(&uart_cfg);
    UART_Init((LPC_UART_TypeDef *) LPC_UART0, &uart_cfg);
    UART_FIFOConfigStructInit(&fifo_cfg);
    UART_FIFOConfig((LPC_UART_TypeDef *) LPC_UART0, &fifo_cfg);
    UART_TxCmd((LPC_UART_TypeDef *) LPC_UART0, ENABLE);
}

// Configuración GPIO
void ConfGPIO(void)
{
    PINSEL_CFG_Type PinCfg;
    PinCfg.Portnum = 0;
    PinCfg.Funcnum = 0; // GPIO
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PinCfg.Pinnum = SERVO_PIN;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = SERVO_PIN2;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = SERVO_PIN3;
    PINSEL_ConfigPin(&PinCfg);
    GPIO_SetDir(0, (1<<SERVO_PIN) | (1<<SERVO_PIN2) | (1<<SERVO_PIN3), 1);

    GPIO_SetDir(0,1<<22,1);
    GPIO_SetDir(0,1<<22,1);
}

// Configuración ADC
void ConfADC(void)
{
    PINSEL_CFG_Type PinCfg;

    // Potenciómetro 1 -> AD0.4 (P1.30)
    PinCfg.Portnum = 1;
    PinCfg.Pinnum = 30;
    PinCfg.Funcnum = 3;
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PinCfg.OpenDrain = 0;
    PINSEL_ConfigPin(&PinCfg);

    // Potenciómetro 2 -> AD0.5 (P1.31)
    PinCfg.Pinnum = 31;
    PINSEL_ConfigPin(&PinCfg);

    // Potenciómetro 3 -> AD0.2 (P0.25)
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 25;
    PinCfg.Funcnum = 1;
    PINSEL_ConfigPin(&PinCfg);

    ADC_Init(LPC_ADC, 1000); // Frecuencia de muestreo baja, ya que está en Burst
    ADC_BurstCmd(LPC_ADC, ENABLE);

    // Habilita los 3 canales para los servos (y el DAC usará el CH4)
    ADC_ChannelCmd(LPC_ADC, ADC_CH, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CH5, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CH2, ENABLE);

    // Habilita interrupción para CADA canal
    ADC_IntConfig(LPC_ADC, ADC_CH, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_CH5, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_CH2, ENABLE);

    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUOUS);
    NVIC_EnableIRQ(ADC_IRQn);
}

// Interrupción ADC (Controla los servos Y actualiza 'valorADC1' para el DAC)
void ADC_IRQHandler(void)
{
    if (ADC_ChannelGetStatus(LPC_ADC, ADC_CH, 1))
    {
        // Esta variable la usan tanto el servo como el TIMER0
        valorADC1 = ADC_ChannelGetData(LPC_ADC, ADC_CH);

        // Lógica del Servo 1
        if (valorADC1 >= 1948 && valorADC1 < 2147)
            pulso = 1500;
        else if (valorADC1 < 1948)
            pulso = 1000 + (uint32_t)(((float)valorADC1 / 1947.0f) * 500.0f);
        else
            pulso = 1500 + (uint32_t)(((float)(valorADC1 - 2147) / 1948.0f) * 500.0f);

        TIM_UpdateMatchValue(LPC_TIM2, 1, pulso);
    }

    if (ADC_ChannelGetStatus(LPC_ADC, ADC_CH5, 1))
    {
        valorADC2 = ADC_ChannelGetData(LPC_ADC, ADC_CH5);

        // Lógica del Servo 2
        if (valorADC2 >= 1948 && valorADC2 < 2147)
            pulso2 = 1500;
        else if (valorADC2 < 1948)
            pulso2 = 1000 + (uint32_t)(((float)valorADC2 / 1947.0f) * 500.0f);
        else
            pulso2 = 1500 + (uint32_t)(((float)(valorADC2 - 2147) / 1948.0f) * 500.0f);

        TIM_UpdateMatchValue(LPC_TIM2, 2, pulso2);
    }

    if (ADC_ChannelGetStatus(LPC_ADC, ADC_CH2, 1))
    {
        valorADC3 = ADC_ChannelGetData(LPC_ADC, ADC_CH2);

        // Lógica del Servo 3
        if (valorADC3 >= 1948 && valorADC3 < 2147)
            pulso3 = 1500;
        else if (valorADC3 < 1948)
            pulso3 = 1000 + (uint32_t)(((float)valorADC3 / 1947.0f) * 500.0f);
        else
            pulso3 = 1500 + (uint32_t)(((float)(valorADC3 - 2147) / 1948.0f) * 500.0f);

        TIM_UpdateMatchValue(LPC_TIM2, 3, pulso3);
    }
}

// Timer2 PWM software (Sin cambios)
void ConfTIMER2(void)
{
    TIM_TIMERCFG_Type timerCfg;
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1;
    TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &timerCfg);

    TIM_MATCHCFG_Type match0;
    match0.MatchChannel = 0;
    match0.IntOnMatch = ENABLE;
    match0.ResetOnMatch = ENABLE;
    match0.StopOnMatch = DISABLE;
    match0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match0.MatchValue = 20000;
    TIM_ConfigMatch(LPC_TIM2, &match0);

    TIM_MATCHCFG_Type match1;
    match1.MatchChannel = 1;
    match1.IntOnMatch = ENABLE;
    match1.ResetOnMatch = DISABLE;
    match1.StopOnMatch = DISABLE;
    match1.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match1.MatchValue = 1500;
    TIM_ConfigMatch(LPC_TIM2, &match1);

    TIM_MATCHCFG_Type match2;
    match2.MatchChannel = 2;
    match2.IntOnMatch = ENABLE;
    match2.ResetOnMatch = DISABLE;
    match2.StopOnMatch = DISABLE;
    match2.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match2.MatchValue = 1500;
    TIM_ConfigMatch(LPC_TIM2, &match2);

    TIM_MATCHCFG_Type match3;
    match3.MatchChannel = 3;
    match3.IntOnMatch = ENABLE;
    match3.ResetOnMatch = DISABLE;
    match3.StopOnMatch = DISABLE;
    match3.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match3.MatchValue = 1500;
    TIM_ConfigMatch(LPC_TIM2, &match3);

    NVIC_EnableIRQ(TIMER2_IRQn);
    TIM_Cmd(LPC_TIM2, ENABLE);
}

// Interrupción Timer2 (Sin cambios)
void TIMER2_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT))
    {
        counter++;
        if (counter >= 50)
        {
            char mensaje[128];
            sprintf(mensaje,
                    "Posicion de los servos : pulso=%u  pulso2=%u  pulso3=%u\r\n",
                    pulso, pulso2, pulso3);
            UART_Send((LPC_UART_TypeDef *)LPC_UART0, (uint8_t *)mensaje,strlen(mensaje),
                      NONE_BLOCKING);
            counter = 0;
        }
        TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
        GPIO_SetValue(0, (1<<SERVO_PIN));
        GPIO_SetValue(0, (1<<SERVO_PIN2));
        GPIO_SetValue(0, (1<<SERVO_PIN3));
    }
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR1_INT))
    {
        TIM_ClearIntPending(LPC_TIM2, TIM_MR1_INT);
        GPIO_ClearValue(0, (1<<SERVO_PIN));
    }
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR2_INT))
    {
        TIM_ClearIntPending(LPC_TIM2, TIM_MR2_INT);
        GPIO_ClearValue(0, (1<<SERVO_PIN2));
    }
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR3_INT))
    {
        TIM_ClearIntPending(LPC_TIM2, TIM_MR3_INT);
        GPIO_ClearValue(0, (1<<SERVO_PIN3));
    }
}

// Main
int main(void)
{
    ConfGPIO();
    ConfADC();
    ConfTIMER0(); // Para muestrear al DAC
    ConfTIMER2(); // Para los servos
    configGPIO_UART();
    configUART();
    confDac();
    confDMA_DAC(); // Inicia el DMA circular

    while (1)
    {
        // Todo se maneja por interrupciones
    }
}
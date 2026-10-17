#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_nvic.h"
#include "lpc17xx_exti.h"

// --- Definiciones ---
#define SERVO_GARRA_PIN (1 << 12) // P2.12
#define BOTON_PIN       (1 << 11) // P2.11

#define DMA_SIZE        500
#define PCLK_DAC_IN_MHZ 25        // CCLK/4 (100/4 = 25MHz)
#define UPDATE_FREQ_HZ  50        // Frecuencia de actualización deseada

// --- Variables Globales ---
uint32_t bufferEntrada[DMA_SIZE]; // Buffer crudo del ADC
uint32_t bufferSalida[DMA_SIZE];  // Buffer formateado para DAC

volatile uint8_t contadorBoton = 0;

// --- Prototipos ---
void ConfGPIO(void);
void ConfTIMER0(void);
void confDMA_ADC(void);
void confDMA_DAC(void);
void confDac(void);

// -----------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------
int main(void) {
    ConfGPIO();
    ConfTIMER0();
    confDac();
    confDMA_ADC(); // Configura y arranca el ADC+DMA

    while (1) {
        // El trabajo se realiza en las interrupciones
    }
    return 0;
}

// -----------------------------------------------------------------
// CONFIGURACIÓN GPIO (Servo y Botón)
// -----------------------------------------------------------------
void ConfGPIO(void) {
    PINSEL_CFG_Type PinCfg;

    // --- Servo (P2.12) ---
    PinCfg.Portnum = 2;
    PinCfg.Pinnum = 12;
    PinCfg.Funcnum = 0; // GPIO
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PinCfg.OpenDrain = 0;
    PINSEL_ConfigPin(&PinCfg);
    
    GPIO_SetDir(2, SERVO_GARRA_PIN, 1); // Salida

    // --- Botón (P2.11) para EINT1 ---
    PinCfg.Portnum = 2;
    PinCfg.Pinnum = 11;
    PinCfg.Funcnum = 1; // EINT1
    PINSEL_ConfigPin(&PinCfg);

    // Configurar Interrupción Externa 1
    EXTI_Init();
    EXTI_InitTypeDef EXTI_Cfg;
    EXTI_Cfg.EXTI_Line = EXTI_EINT1;
    EXTI_Cfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    EXTI_Cfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
    EXTI_Config(&EXTI_Cfg);
    
    NVIC_EnableIRQ(EINT1_IRQn);

    // --- DAC (P0.26) ---
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Funcnum = 2; // AOUT
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&PinCfg);
    
    // --- ADC (P0.23 - AD0.0) ---
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 23;
    PinCfg.Funcnum = 1; // AD0.0
    PINSEL_ConfigPin(&PinCfg);
}

// -----------------------------------------------------------------
// HANDLER BOTÓN (EINT1)
// -----------------------------------------------------------------
void EINT1_IRQHandler(void){
    EXTI_ClearEXTIFlag(EXTI_EINT1);
    contadorBoton++;
    
    if(contadorBoton == 1){
        TIM_UpdateMatchValue(LPC_TIM0, 1, 2000); // 2ms
    } else {
        contadorBoton = 0; 
        TIM_UpdateMatchValue(LPC_TIM0, 1, 1000); // 1ms
    }
}

// -----------------------------------------------------------------
// CONFIGURACIÓN TIMER0 (PWM por Software)
// -----------------------------------------------------------------
void ConfTIMER0(void) {
    TIM_TIMERCFG_Type timerCfg;
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1; // 1 us
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    // MR0: Periodo (20ms)
    TIM_MATCHCFG_Type match0;
    match0.MatchChannel = 0;
    match0.IntOnMatch = ENABLE;
    match0.ResetOnMatch = ENABLE;
    match0.StopOnMatch = DISABLE;
    match0.ExtMatchAction = TIM_EXTMATCH_NOTHING;
    match0.MatchValue = 20000;
    TIM_ConfigMatch(LPC_TIM0, &match0);
    
    // MR1: Ancho de Pulso
    TIM_MATCHCFG_Type match1;
    match1.MatchChannel = 1;
    match1.IntOnMatch = ENABLE;
    match1.ResetOnMatch = DISABLE;
    match1.StopOnMatch = DISABLE;
    match1.ExtMatchAction = TIM_EXTMATCH_NOTHING;
    match1.MatchValue = 1000;
    TIM_ConfigMatch(LPC_TIM0, &match1);

    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Cmd(LPC_TIM0, ENABLE);
}

// -----------------------------------------------------------------
// HANDLER TIMER0 (Generación PWM Software)
// -----------------------------------------------------------------
void TIMER0_IRQHandler(void) {
    // Inicio del periodo (Poner en Alto)
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        GPIO_SetValue(2, SERVO_GARRA_PIN);
    }
    // Fin del pulso (Poner en Bajo)
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);
        GPIO_ClearValue(2, SERVO_GARRA_PIN);
    }
}

// -----------------------------------------------------------------
// CONFIGURACIÓN DMA LECTURA (ADC -> Memoria)
// -----------------------------------------------------------------

void confDMA_ADC(void){
    // Config ADC primero
    ADC_Init(LPC_ADC, 200000);
    ADC_BurstCmd(LPC_ADC, ENABLE); // Burst necesario para DMA request
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_ADGINTEN, DISABLE); // DMA maneja esto
    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUOUS);

    GPDMA_Channel_CFG_Type dma_cfg;
    GPDMA_Init();

    dma_cfg.ChannelNum = 0; 
    dma_cfg.SrcConn = GPDMA_CONN_ADC; // Corregido mayúscula
    dma_cfg.DstMemAddr = (uint32_t)bufferEntrada;
    dma_cfg.TransferSize = DMA_SIZE;
    dma_cfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
    dma_cfg.TransferWidth = 3;      //32 bits
    dma_cfg.SrcMemAddr = 0; 
    dma_cfg.DstConn = 0;
    dma_cfg.DMALLI = 0; 

    GPDMA_Setup(&dma_cfg);
    
    // Habilitar interrupción TC y el Controlador
    LPC_GPDMACH0->DMACCControl |= (1 << 31); 
    NVIC_EnableIRQ(DMA_IRQn);
    LPC_GPDMA->DMACConfig = 1;

    GPDMA_ChannelCmd(0, ENABLE);
}

// -----------------------------------------------------------------
// CONFIGURACIÓN DMA ESCRITURA (Memoria -> DAC)
// -----------------------------------------------------------------
void confDMA_DAC(void) {
    GPDMA_Channel_CFG_Type dma_cfg;

    dma_cfg.ChannelNum = 1; 
    dma_cfg.DstConn = GPDMA_CONN_DAC; 
    dma_cfg.SrcMemAddr = (uint32_t)bufferSalida;
    dma_cfg.TransferSize = DMA_SIZE;
    dma_cfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
    dma_cfg.TransferWidth = GPDMA_WIDTH_WORD;
    dma_cfg.DstMemAddr = 0;
    dma_cfg.SrcConn = 0;
    dma_cfg.DMALLI = 0;

    GPDMA_Setup(&dma_cfg);
    // Se habilita en el IRQ cuando los datos están listos
}

// -----------------------------------------------------------------
// HANDLER DMA (Procesamiento y Ping-Pong)
// -----------------------------------------------------------------
void DMA_IRQHandler(void) {
    // Si terminó el ADC (Canal 0)
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, 0)) {
        
        // Procesar datos: ADC (12bit) -> DAC (10bit)
        for (int i = 0; i < DMA_SIZE; i++) {
            uint32_t raw = bufferEntrada[i];
            uint32_t val_adc = (raw >> 4) & 0xFFF; 
            uint32_t val_dac = val_adc >> 2; // Escalar a 10 bits
            
            // Formato DACR: Value en bits 6-15, Bias=0 (bit 16=0)
            bufferSalida[i] = (val_dac << 6); 
        }

        // Disparar DMA para DAC (Canal 1)
        confDMA_DAC(); 
        GPDMA_ChannelCmd(1, ENABLE);

        // Reiniciar DMA para ADC (Canal 0)
        confDMA_ADC(); 
        
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, 0);
}
    
    // Limpiar error si ocurre
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, 0)) {
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, 0);
    }
}

// -----------------------------------------------------------------
// CONFIGURACIÓN DAC (Hardware Trigger)
// -----------------------------------------------------------------
void confDac(void){
    uint32_t tmp;
    DAC_CONVERTER_CFG_Type DAC_Config;
    
    DAC_Config.CNT_ENA = SET;
    DAC_Config.DMA_ENA = SET;
    DAC_Init(LPC_DAC);
    
    // Calcular timeout para la frecuencia deseada
    // tmp = PCLK / (Freq_Salida * Muestras)
    tmp = (PCLK_DAC_IN_MHZ * 1000000) / (UPDATE_FREQ_HZ * DMA_SIZE);
    
    DAC_SetDMATimeOut(LPC_DAC, tmp);
    DAC_ConfigDAConverterControl(LPC_DAC, &DAC_Config);
}
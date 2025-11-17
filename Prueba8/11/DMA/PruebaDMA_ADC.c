#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_nvic.h"

// --- Definiciones ---
#define ADC_CH 4                // Canal ADC 4 (P1.30)
#define BLOCK_SIZE 256          // Tamaño del buffer para DMA
#define LED_PIN (1 << 22)       // P0.22 para indicar DMA activo
#define DMA_CHANNEL 0           // Canal DMA 0

// --- Buffers ---
uint16_t bufferADC[BLOCK_SIZE] __attribute__((aligned(4))); // Buffer para datos ADC
volatile uint32_t indiceADC = 0;                            // Índice actual
volatile uint8_t dmaCompleto = 0;                           // Flag de DMA completo

// --- Variables para procesamiento ---
uint32_t valorADC1 = 0;
uint32_t pulso = 1500;

// --- Prototipos ---
void ConfADC_DMA(void);
void configDMA_ADC(void);
void configGPIO_LED(void);
void procesarDatosADC(void);

// -----------------------------------------------------------------
// HANDLER DE INTERRUPCIÓN DEL DMA (Cuando se llena el buffer)
// -----------------------------------------------------------------
void DMA_IRQHandler(void) {
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, DMA_CHANNEL)) {
        
        // DMA completó la transferencia del buffer
        dmaCompleto = 1;
        LPC_GPIO0->FIOSET = LED_PIN;  // Indicar DMA completo
        
        // Limpiar bandera
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, DMA_CHANNEL);
        
        // Reiniciar DMA para siguiente transferencia
        GPDMA_ChannelCmd(DMA_CHANNEL, DISABLE);
        indiceADC = 0;  // Reiniciar índice
        GPDMA_ChannelCmd(DMA_CHANNEL, ENABLE);
    }
    
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, DMA_CHANNEL)) {
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, DMA_CHANNEL);
    }
}

// -----------------------------------------------------------------
// CONFIGURACIÓN ADC CON DMA
// -----------------------------------------------------------------
void ConfADC_DMA(void) {
    PINSEL_CFG_Type PinCfg;

    // P1.30 -> AD0.4
    PinCfg.Portnum = 1;
    PinCfg.Pinnum = 30;
    PinCfg.Funcnum = 3;
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PinCfg.OpenDrain = 0;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar ADC
    ADC_Init(LPC_ADC, 1000);
    
    // Configurar modo BURST para DMA
    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CH, ENABLE);
    
    // IMPORTANTE: Deshabilitar interrupciones por canal individual
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN4, DISABLE);
    
    // Habilitar interrupción global del ADC para DMA
    ADC_IntConfig(LPC_ADC, ADC_ADGINTEN, ENABLE);
    
    // Iniciar conversiones continuas
    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUOUS);
}

// -----------------------------------------------------------------
// CONFIGURACIÓN DMA PARA ADC
// -----------------------------------------------------------------
void configDMA_ADC(void) {
    GPDMA_Channel_CFG_Type dma_cfg;

    // Inicializar DMA
    GPDMA_Init();

    // Configurar canal DMA para transferencia Periférico->Memoria (ADC->buffer)
    dma_cfg.ChannelNum = DMA_CHANNEL;
    dma_cfg.TransferSize = BLOCK_SIZE;  // Número de transferencias
    
    // Configurar transferencia de 16 bits (resultado ADC es 12 bits en 16-bit register)
    dma_cfg.TransferWidth = 1;  // 1 = 16 bits (según PDF página 18)
    
    // Configurar burst
    dma_cfg.SrcBurstSize = 0;   // Burst de 1 transferencia
    dma_cfg.DstBurstSize = 0;   // Burst de 1 transferencia
    
    // Configurar incremento
    dma_cfg.SrcInc = 0;         // Dirección fuente constante (registro ADC)
    dma_cfg.DstInc = 1;         // Incrementar dirección destino (buffer en memoria)
    
    // Configuración P2M (Peripheral to Memory)
    dma_cfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
    
    // Dirección del registro de datos del ADC (fuente)
    dma_cfg.SrcMemAddr = (uint32_t)&LPC_ADC->ADDR4;  // Registro del canal 4
    
    // Dirección del buffer en memoria (destino)
    dma_cfg.DstMemAddr = (uint32_t)bufferADC;
    
    // Configurar periféricos
    dma_cfg.SrcConn = GPDMA_CONN_ADC;    // ADC como fuente
    dma_cfg.DstConn = 0;                 // Memoria como destino
    
    dma_cfg.DMALLI = 0;  // Sin listas enlazadas

    // Aplicar configuración
    if (GPDMA_Setup(&dma_cfg) != SUCCESS) {
        while(1); // Error en configuración
    }
    
    // Habilitar interrupciones del DMA
    GPDMA_IntConfig(DMA_CHANNEL, GPDMA_INT_TC, ENABLE);
    
    // Habilitar interrupción DMA en NVIC
    NVIC_EnableIRQ(DMA_IRQn);
}

// -----------------------------------------------------------------
// PROCESAR DATOS ADC (Se llama desde main cuando DMA está completo)
// -----------------------------------------------------------------
    
void procesarDatosADC(void) {
    if (dmaCompleto) {
        dmaCompleto = 0;
        LPC_GPIO0->FIOCLR = LED_PIN;  // Apagar LED
        
        // Procesar todos los datos del buffer
        for (int i = 0; i < BLOCK_SIZE; i++) {
            // Extraer valor ADC (12 bits en bits 15:4)
            valorADC1 = (bufferADC[i] >> 4) & 0xFFF;
            
            // Aplicar tu lógica de conversión
            if (valorADC1 >= 1948 && valorADC1 < 2147) {
                pulso = 1500;
            } else if (valorADC1 < 1948) {
                pulso = 1000 + (uint32_t)(((float)valorADC1 / 1947.0f) * 500.0f);
            } else {
                pulso = 1500 + (uint32_t)(((float)(valorADC1 - 2147) / 1948.0f) * 500.0f);
            }
            
            // Actualizar PWM (puedes modificar esto según necesites)
            TIM_UpdateMatchValue(LPC_TIM2, 1, pulso);
        }
    }
}

// -----------------------------------------------------------------
// FUNCIÓN PRINCIPAL
// -----------------------------------------------------------------
int main(void) {
    
    // Inicializaciones
    configGPIO_LED();
    ConfADC_DMA();
    configDMA_ADC();
    
    // Iniciar DMA
    GPDMA_ChannelCmd(DMA_CHANNEL, ENABLE);

    while(1) {
        // Procesar datos cuando el DMA complete un buffer
        procesarDatosADC();
        
        // La CPU está libre para otras tareas
        // mientras el DMA se encarga de mover los datos del ADC
    }
    return 0;
}

// -----------------------------------------------------------------
// CONFIGURACIÓN GPIO (igual que antes)
// -----------------------------------------------------------------
void configGPIO_LED(void) {
    LPC_GPIO0->FIODIR |= LED_PIN; // P0.22 como salida
    LPC_GPIO0->FIOCLR = LED_PIN;  // Empezar con el LED apagado
}
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_nvic.h"


#define BLOCK_SIZE 128          // Cuántos bytes vamos a mover
#define LED_PIN    (1 << 22)    // P0.22
#define LED_PIN3    (1 << 25)    // P3.22
#define DMA_CHANNEL 0           // Usaremos el canal 0

uint8_t array_fuente[BLOCK_SIZE];
uint8_t array_destino[BLOCK_SIZE];

void configDMA_M2M(void);
void configGPIO_LED(void);

// -----------------------------------------------------------------
// HANDLER DE INTERRUPCIÓN DEL DMA
// -----------------------------------------------------------------

void DMA_IRQHandler(void) {
    
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, DMA_CHANNEL)) {
        
        // 2. ¡Terminó la copia! Encender el LED
        LPC_GPIO0->FIOSET = LED_PIN;

        GPDMA_ClearIntPending( DMA_CHANNEL, ENABLE);
    }
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, DMA_CHANNEL)) {
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTERR, DMA_CHANNEL);
    }
}

// -----------------------------------------------------------------
// FUNCIÓN PRINCIPAL
// -----------------------------------------------------------------

int main(void) {
    
    // Preparar los datos
    for (int i = 0; i < BLOCK_SIZE; i++) {
        array_fuente[i] = i & 0xFF;         // Llenar la fuente con un patrón
        array_destino[i] = 0;               // Limpiar el destino
    }

    configGPIO_LED();
    configDMA_M2M();

    
    GPDMA_ChannelCmd(DMA_CHANNEL, ENABLE);

    while(1) {
        // La CPU está libre mientras el DMA trabaja...
        // El LED se encenderá solo cuando el DMA termine.
    }
    return 0;
}

// -----------------------------------------------------------------
// CONFIGURACIONES
// -----------------------------------------------------------------
void configGPIO_LED(void) {
    LPC_GPIO0->FIODIR |= LED_PIN; // P0.22 como salida
    LPC_GPIO3->FIODIR |= LED_PIN3; // P3.22 como salida
    LPC_GPIO0->FIOCLR = LED_PIN;  // Empezar con el LED apagado
    LPC_GPIO3->FIOSET = LED_PIN3;
}

void configDMA_M2M(void) {
    GPDMA_Channel_CFG_Type dma_cfg;

    // Inicializar el periférico DMA (Esto enciende el PCONP)
    GPDMA_Init();

    // Configurar el canal
    dma_cfg.ChannelNum = DMA_CHANNEL;
    dma_cfg.TransferSize = BLOCK_SIZE;
    dma_cfg.TransferWidth = 0; // Transferencia de 8 bits (byte)
    
    // --- Configuración M2M ---
    dma_cfg.TransferType = GPDMA_TRANSFERTYPE_M2M;
    dma_cfg.SrcMemAddr = (uint32_t)array_fuente;
    dma_cfg.DstMemAddr = (uint32_t)array_destino;
    
    dma_cfg.SrcConn = 0;
    dma_cfg.DstConn = 0;
    dma_cfg.DMALLI = 0;

    // Aplicar la configuración
    GPDMA_Setup(&dma_cfg);
    
    // Habilitar la interrupción del DMA en el NVIC
    NVIC_EnableIRQ(DMA_IRQn);

    // -----------------------------------------------------------------
    // --- LÍNEAS CORREGIDAS QUE FALTABAN ---
    // -----------------------------------------------------------------
    
    // 1. Encender el controlador DMA (Bit 0 = Enable)
    LPC_GPDMA->DMACConfig = 1;

    // 2. Habilitar la interrupción de "Terminal Count" (Fin de Conteo)
    // para el Canal 0 (Bit 31 del registro de control del canal)
    LPC_GPDMACH0->DMACCControl |= (1 << 31);
    // -----------------------------------------------------------------
}
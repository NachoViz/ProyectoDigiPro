#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

// --- Definiciones ---
#define BLOCK_SIZE 128          // Cuántos bytes vamos a mover
#define LED_PIN    (1 << 22)    // P0.22
#define DMA_CHANNEL 0

// --- Buffers de Memoria ---
// (Usamos 'volatile' para que el compilador no los optimice)
volatile uint8_t array_fuente[BLOCK_SIZE];
volatile uint8_t array_destino[BLOCK_SIZE];

// --- Prototipos ---
void configDMA_M2M(void);
void configGPIO_LED(void);

// -----------------------------------------------------------------
// HANDLER DE INTERRUPCIÓN DEL DMA
// -----------------------------------------------------------------
void DMA_IRQHandler(void) {
    // 1. Verificar si la interrupción fue por Transferencia Completa (TC)
    if (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, DMA_CHANNEL)) { //

        // 2. Encender el LED
        LPC_GPIO0->FIOSET = LED_PIN;

        // 3. Limpiar la bandera de interrupción
        GPDMA_ClearIntPending(GPDMA_STATCLR_INTTC, DMA_CHANNEL); //
    }
}

// -----------------------------------------------------------------
// FUNCIÓN PRINCIPAL
// -----------------------------------------------------------------
int main(void) {

    // Preparar los datos
    for (int i = 0; i < BLOCK_SIZE; i++) {
        array_fuente[i] = 0xAA;
        array_destino[i] = 0;
    }

    configGPIO_LED();
    configDMA_M2M();

    // Habilitar el canal 0 del DMA.
    GPDMA_ChannelCmd(DMA_CHANNEL, ENABLE); //

    while(1) {
        // El LED debe encenderse casi instantáneamente si el DMA funciona.
    }
    return 0;
}

// -----------------------------------------------------------------
// CONFIGURACIÓN GPIO (LED)
// -----------------------------------------------------------------
void configGPIO_LED(void) {
    // Configurar P0.22 como salida GPIO
    PINSEL_CFG_Type PinCfg;
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 22;
    PinCfg.Funcnum = 0; // GPIO
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PinCfg.OpenDrain = 0;
    PINSEL_ConfigPin(&PinCfg);

    LPC_GPIO0->FIODIR |= LED_PIN;
    LPC_GPIO0->FIOCLR = LED_PIN;
}

// -----------------------------------------------------------------
// CONFIGURACIÓN DMA (M2M) - CORREGIDA
// -----------------------------------------------------------------
void configDMA_M2M(void) {
    GPDMA_Channel_CFG_Type dma_cfg;

    // 1. Inicializar el periférico DMA (Enciende el PCONP)
    GPDMA_Init(); //

    // 2. Configurar la estructura
    dma_cfg.ChannelNum = DMA_CHANNEL;
    dma_cfg.TransferSize = BLOCK_SIZE;
    dma_cfg.TransferWidth = 0; // 0 = 8 bits
    dma_cfg.TransferType = GPDMA_TRANSFERTYPE_M2M; //
    dma_cfg.SrcMemAddr = (uint32_t)array_fuente;
    dma_cfg.DstMemAddr = (uint32_t)array_destino;
    dma_cfg.SrcConn = 0;
    dma_cfg.DstConn = 0;
    dma_cfg.DMALLI = 0;

    // 3. Aplicar la configuración
    GPDMA_Setup(&dma_cfg); //

    // 4. Habilitar la interrupción en el NVIC
    NVIC_EnableIRQ(DMA_IRQn);

    // --- ACCESO DIRECTO A REGISTROS (Requerido según tus PDFs) ---

    // 5. Habilitar el controlador DMA (Pág 17, Reg DMACConfig, bit 0 'E')
    LPC_GPDMA->DMACConfig = 1;

    // 6. Habilitar la interrupción de 'Terminal Count' (TC)
    // (Pág 17, Reg DMACCControl, bit 31 'I')
    LPC_GPDMACH0->DMACCControl |= (1 << 31);
}

#include "lpc17xx_adc.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_nvic.h"

#define ADC_CONV_RATE 200000
#define ADC_CHANNEL_USED ADC_CHANNEL_2 

void configADC_Drivers(void);
void configADCPin(void);

int main(void) {
    LPC_SC->PCLKSEL0 |= (3 << 24);
    
    configADCPin();
    configADC_Drivers();

    // Iniciar la PRIMERA conversión para arrancar el ciclo de interrupciones
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);

    while(1) {
        // La CPU queda libre (modo de bajo consumo)
        // El ciclo de ADC se auto-mantiene por interrupciones
    }
    return 0;
}

void configADCPin(void) {
    PINSEL_CFG_Type PinCfg;
    
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 25;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    
    LPC_PINCON->PINMODE1 |= (3 << 18);
}

void configADC_Drivers(void) {
    
    ADC_Init(LPC_ADC, ADC_CONV_RATE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE); 
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN2, ENABLE);
    ADC_BurstCmd(LPC_ADC, DISABLE);
    NVIC_EnableIRQ(ADC_IRQn);
}

void ADC_IRQHandler(void) {
    uint32_t resultado;
    
    if (ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_2, ADC_DATA_DONE)) {
        
        resultado = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
        
        // Aquí usas 'resultado' para lo que necesites...+
    }
    
    // Iniciar la SIGUIENTE conversión
    // Esto crea la cadena de interrupciones
    ADC_StartCmd(LPC_ADC, ADC_START_NOW);
}
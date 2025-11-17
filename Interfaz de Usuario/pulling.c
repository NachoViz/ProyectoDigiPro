/*  ULTIMO RECURSO
 * Prueba Simple de ADC usando Polling
 */

#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"

uint32_t adc_value;

void configGPIO(void);
void configADC(void);

#define PIN_ADC (1 << 23)   // P0.23 (Canal 0)
#define LED_OUT (1 << 6)    // P2.6

int main(){
    configGPIO();
    configADC();
    
    while(1){
        // 1. Iniciar una conversión (Start)
        ADC_StartCmd(LPC_ADC, ADC_START_NOW); 
        
        // 2. Esperar a que la conversión termine (Polling)
        // La función espera (bloquea) hasta que el canal 0 esté listo (1)
        while(!ADC_ChannelGetStatus(LPC_ADC, 0, 1)); 

        // 3. Leer el valor
        adc_value = ADC_ChannelGetData(LPC_ADC, 0); 
        
        // 4. Lógica de prueba simple
        if(adc_value > 2048){
            GPIO_SetValue(2, LED_OUT);      // Encender P2.6
        } else {
            GPIO_ClearValue(2, LED_OUT);    // Apagar P2.6
        }
    }

}

void configGPIO(){
    // Configurar P2.6 como salida
    GPIO_SetDir(2, LED_OUT, 1);
}

void configADC(){
    
    // Configuración PINSEL (P0.23 -> FUNC_1 / ADC)
    PINSEL_CFG_Type pinse = {PINSEL_PORT_0, PINSEL_PIN_23, PINSEL_FUNC_1, PINSEL_PINMODE_TRISTATE, PINSEL_PINMODE_NORMAL};
    PINSEL_ConfigPin(&pinse);

    // Inicializar ADC y habilitar Canal 0
    ADC_Init(LPC_ADC, 200000);
    ADC_ChannelCmd(LPC_ADC, 0 , ENABLE);
    
    // Deshabilitar Burst y Deshabilitar Interrupciones para usar Polling
    ADC_BurstCmd(LPC_ADC, DISABLE);
    ADC_IntConfig(LPC_ADC , ADC_ADINTEN, DISABLE);
}

// Ya no necesitamos el ADC_IRQHandler()
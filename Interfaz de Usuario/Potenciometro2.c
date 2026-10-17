/*	
 * Probando facilmente 2 protenciometro con leds
 *      
 *      
 */

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_nvic.h"

/*
 * potenciometro por el pin - P0.9
 * mediante el ADC (canal AD0.4) y PWM por software con Timer2. - P1.30
 */

#define SERVO_PIN 9        // P0.9
#define ADC_CH    4        // AD0.4 (P1.30)
#define ADC_CH5    5        // AD0.5 (P1.31)
#define SERVO_MIN_US 1000            // 1.0 ms  -> ángulo mínimo
#define SERVO_MAX_US 2000            // 2.0 ms  -> ángulo máximo

volatile uint16_t valorADC1 = 0;
volatile uint16_t valorADC2 = 0;


// INTERRUPCIÓN ADC
void ADC_IRQHandler(void) {
    // Lee valor del canal 4
    valorADC1 = ADC_ChannelGetData(LPC_ADC, ADC_CH);

    while(! ADC_ChannelGetStatus(LPC_ADC,ADC_CH ,ADC_DATA_DONE));

    // Mapea 0–4095 → 1000–2000 µs
    uint32_t pulso = 1000 + ((uint32_t)valorADC * 1000 / 4095);

    // Actualiza el valor del MATCH2 (ancho de pulso)
    TIM_UpdateMatchValue(LPC_TIM2, 2, pulso);

    if(pulso>=0 && pulso < 1365){
                GPIO_SetValue(0, (1 << 22));
                GPIO_ClearValue(3 , ( 1 << 25));
                GPIO_ClearValue(3 , ( 1 << 26));
            }else if(pulso>=1365 && pulso< 2730){
                GPIO_SetValue(3, (1 << 25));
                GPIO_ClearValue(0 , ( 1 << 22));
                GPIO_ClearValue(3 , ( 1 << 26));
            }else if(pulso>=2730 && pulso<4095){
                GPIO_SetValue(3, (1 << 26));
                GPIO_ClearValue(0 , ( 1 << 22));
                GPIO_ClearValue(3 , ( 1 << 25));
            }

// Lee valor del canal 5  (ejey)
    valorADC2 = ADC_ChannelGetData(LPC_ADC, ADC_CH5);

    while(! ADC_ChannelGetStatus(LPC_ADC,ADC_CH5 ,ADC_DATA_DONE));

    // Mapea 0–4095 → 1000–2000 µs
    uint32_t pulso2 = 1000 + ((uint32_t)valorADC2 * 1000 / 4095);

    if(valorADC2>=1948 && valorADC2 < 2147){
        pulso = 1500;
            }else if(valorADC2>=0 && valorADC2< 1948){
                pulso = 1500 - ((uint32_t)valorADC2 * 1000 / 4095);
            }else if(valorADC2>=2147 && valorADC2<4096){
                pulso = 1500 + ((uint32_t)valorADC2 * 1000 / 4095);
            }
// Actualiza el valor del MATCH2 (ancho de pulso)
    
    TIM_UpdateMatchValue(LPC_TIM2, 2, pulso1);

    
}

int main(){
    configGPIO();
    configADC();
    
    while(1){

    }

}

void configGPIO(){
    GPIO_SetDir(2, P2_6, 1);   //NORTE 
    GPIO_SetDir(2, P2_7, 1);   //SUR 
    GPIO_SetDir(2, P2_8, 1);   //ESTE 
    GPIO_SetDir(2, P2_10, 1);  //OESTE

}

void configADC(){

    //Primer potenciometro configurado 
    GPIO_SetDir(0 , PIN_ADC0, 0);

    PINSEL_CFG_Type pinse1;

    pinse1.PortNum = PINSEL_PORT_0;
    pinse1.PinNum = PINSEL_PIN_23;
    pinse1.FuncNum = PINSEL_FUNC_1;
    pinse1.PinMode = PINSEL_PINMODE_TRISTATE;
    pinse1.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinse1);

    PINSEL_CFG_Type pinse2;

    pinse2.PortNum = PINSEL_PORT_0;
    pinse2.PinNum = PINSEL_PIN_24;
    pinse2.FuncNum = PINSEL_FUNC_1;
    pinse2.PinMode = PINSEL_PINMODE_TRISTATE;
    pinse2.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinse2);

    ADC_Init(LPC_ADC, 200000);

    ADC_BurstCmd(LPC_ADC, ENABLE );

    ADC_ChannelCmd(LPC_ADC, 1 , ENABLE);
    ADC_ChannelCmd(LPC_ADC, 0 , ENABLE);
    
    //ADC_InitConfig(LPC_ADC , ADC_ADINTEN0, ENABLE);
    //ADC_InitConfig(LPC_ADC , ADC_ADINTEN1, ENABLE);

    // CORRECCIÓN CRÍTICA: En multicanal/Burst, usar la Interrupción Global ADINTEN.
    // Esto asegura que la IRQ se dispare después de la conversión completa de todos los canales.
    ADC_InitConfig(LPC_ADC , ADC_ADINTEN, ENABLE);

    NVIC_EnableIRQ(ADC_IRQn);
    
}


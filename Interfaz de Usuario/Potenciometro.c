/*	
 * Probando facilmente 1 protenciometro con leds
 *      
 *      
 */

#include "lpc17xx_gpio.h"
#include "lpc17xx_nvic.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"

uint32_t adc_value;

void configGPIO(void);
void configADC(void);

#define PIN_ADC (1 << 23)   //P0.23
#define P2_6 (1 << 6)
#define P2_7 (1 << 7)
#define P2_8 (1 << 8)

int main(){
    configGPIO();
    configADC();
    
    while(1){

    }

}

void configGPIO(){
    GPIO_SetDir(2, P2_6, 1);   
    GPIO_SetDir(2, P2_7, 1);   
    GPIO_SetDir(2, P2_8, 1);


}

void configADC(){
    GPIO_SetDir(0 , PIN_ADC, 0);

    PINSEL_CFG_Type pinse;

    pinse.PortNum = PINSEL_PORT_0;
    pinse.PinNum = PINSEL_PIN_23;
    pinse.FuncNum = PINSEL_FUNC_1;
    pinse.PinMode = PINSEL_PINMODE_TRISTATE;
    pinse.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinse);

    ADC_Init(LPC_ADC, 200000);
    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUS);
    ADC_BurstCmd(LPC_ADC, 0 );
    ADC_ChannelCmd(LPC_ADC, 0 , ENABLE);
    
    ADC_InitConfig(LPC_ADC , ADC_ADINTEN0, ENABLE);

    NVIC_EnableIRQ(ADC_IRQn);
    
}

void ADC_IRQHandler(){
    if(ADC_ChannelGetStatus(LPC_ADC, 0 , 1)){
        adc_value = ADC_ChannelGetData(LPC_ADC, 0);
        
        if(adc_value <= 1365){              //Prendo P2.6
            GPIO_SetValue(2 , P2_6);
            GPIO_ClearValue(2 , P2_7);
            GPIO_ClearValue(2 , P2_8);
        }else if(adc_value <= 2730 && adc_value >= 1365){   //Prendo P2.7
            GPIO_SetValue(2 , P2_7);
            GPIO_ClearValue(2 , P2_6);
            GPIO_ClearValue(2 , P2_8);
        }else if(adc_value >= 2730 && adc_value >= 4095){   //Prendo P2.8
            GPIO_SetValue(2 , P2_8);
            GPIO_ClearValue(2 , P2_6);
            GPIO_ClearValue(2 , P2_7);
        }

    }
}


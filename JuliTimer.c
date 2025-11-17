#include "lpc17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"

#define ADC_CH    4        // AD0.4 (P1.30)

uint16_t adc_value=0;

void configPin(void);
void configADC(void);
void configUART(void);

int main (void){


	configPin();
	configADC();
	configUART();

	while(1){

	}

	return 0;
}

void configPin(void){
	PINSEL_CFG_Type PINcfg={0};

	//GPIO P0.22 pin con led
	PINcfg.Portnum=0;
	PINcfg.Pinnum=22;
	PINcfg.Funcnum=0;
	PINSEL_ConfigPin(&PINcfg);
	GPIO_SetDir(0,1<<22,1);
	GPIO_SetValue(0,1<<22);			//Prende por bajo

	//UART2 TXD2 P0.10
	PINcfg.Portnum=0;
	PINcfg.Pinnum=10;
	PINcfg.Funcnum=1;
	PINSEL_ConfigPin(&PINcfg);

	//UART2 RXD2 P0.11
	PINcfg.Portnum=0;
	PINcfg.Pinnum=11;
	PINcfg.Funcnum=1;
	PINSEL_ConfigPin(&PINcfg);
}

void configADC(void){
    // Pin P1.30 -> AD0.4
    PINSEL_CFG_Type PinCfg;
    PinCfg.Portnum = 1;
    PinCfg.Pinnum = 30;
    PinCfg.Funcnum = 3;
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PinCfg.OpenDrain = 0;
    PINSEL_ConfigPin(&PinCfg);

    ADC_Init(LPC_ADC, 1000);       // 200 kHz
    ADC_BurstCmd(LPC_ADC, ENABLE);   // Burst Mode
    ADC_ChannelCmd(LPC_ADC, ADC_CH, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_CH, ENABLE);
    ADC_StartCmd(LPC_ADC,ADC_START_CONTINUOUS);

    NVIC_EnableIRQ(ADC_IRQn);
}

void ADC_IRQHandler(void)
{
	adc_value =  ADC_ChannelGetData(LPC_ADC,ADC_CH);//limpia int

	UART_Send(LPC_UART2, (uint8_t *)&adc_value, sizeof(adc_value), BLOCKING);

}

void configUART(void){
	UART_CFG_Type cfgPinUART2;
	cfgPinUART2.Baud_rate=9600;
	cfgPinUART2.Parity=UART_PARITY_NONE;
	cfgPinUART2.Databits=UART_DATABIT_8;
	cfgPinUART2.Stopbits = UART_STOPBIT_1;


	UART_FIFO_CFG_Type cfgUART2FIFO;

//	UART_ConfigStructInit(&cfgPinUART2);         //Configuración por Defecto
	UART_Init(LPC_UART2, &cfgPinUART2);          //Inicialización de periférico UART2
	UART_FIFOConfigStructInit(&cfgUART2FIFO);    //Inicialización FIFO
	UART_FIFOConfig(LPC_UART2, &cfgUART2FIFO);   //Inicialización FIFO
	UART_IntConfig(LPC_UART2, UART_INTCFG_RBR, ENABLE); //Habilitación de interrupción por recepción por UART2
	UART_TxCmd(LPC_UART2, ENABLE);               //Habilitación de transmisión por UART2
	NVIC_EnableIRQ(UART2_IRQn);                  //Habilitación de interrupción por UART2
	return;
}

void UART2_IRQHandler(void)
{

    return;
}
/*	Drivers
 *  Created on: 14 october 2025
 *      Author: jnach
 *      
 *      
 */
#include "lpc17xx_gpio.h"
#include "lpc17xx_nvic.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_uart.h"

const char mensaje[] = "Holamundo";
void configGPIO();
void configTimer();
void configUART();

int main(){
    configGPIO();
    configTimer();
    configUART();
    
    while(1){
    
    }
}

void configGPIO(){
    GPIO_SetDir(0 , (1 << 2 ), 1);     
    GPIO_SetDir(0 , (1 << 3 ), 0);
    
	PINSEL_CFG_Type psel_conf;
	psel_conf.Portnum = 0;
	psel_conf.Pinnum = 2;
	psel_conf.Pinmode = PINSEL_PINMODE_TRISTATE; 
	psel_conf.Funcnum = 01;
	psel_conf.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&psel_conf);
    
    PINSEL_CFG_Type psel_conf1;
	psel_conf1.Portnum = 0;
	psel_conf1.Pinnum = 3;
	psel_conf1.Pinmode = PINSEL_PINMODE_TRISTATE; 
	psel_conf1.Funcnum = 01;
	psel_conf1.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&psel_conf1);

}

void configUART(){
    UART_CFG_Type uart = {0};
    uart.Baud_rate = 9600;
    uart.Parity = UART_PARITY_NONE;
    uart.Databits = UART_DATABIT_8;
    uart.Stopbits = UART_STOPBIT_1;

    UART_FIFO_CFG_Type fifo = {0};
    fifo.FIFO_ResetRxBuf = ENABLE;
    fifo.FIFO_ResetTxBuf = ENABLE;
    fifo.FIFO_DMAMODE = DISABLE;
    fifo.FIFO_Level = UART_FIFO_TRGLEV3;
    
    UART_Init((LPC_UART_TypeDef * ) LPC_UART0 , &uart);     //necesario castear ya que el UART0 presenta errores
    UART_FIFOConfig(LPC_UART, &fifo);
    UART_TxCmd(LPC_UART0, ENABLE);
    // y list no hay nvic ni interrupcion handler
    //manipulo el disparo por el timer Handler
}

void configTimer(){
    GPIO_SetDir(1 , (1 << 28), 0);
    PINSEL_CFG_Type pinty = {PINSEL_PORT_1, PINSEL_PIN_28, PINSEL_FUN_2, PINSEL_PINMODE_TRISTATE , PINSEL_PINMODE_NORMAL};
    PINSEL_ConfigPin(&pinty);

    TIM_TIMERCFG_Type timty = {TIM_PRESCALE_USVAL , 1};
    TIM_MATCHCFG_Type match0 = {0};
    match0.MatchChannel = 0;
    match0.IntOnMatch = ENABLE;							//en la interrupcion pone el pin en ALTO
    match0.ResetOnMatch = ENABLE;
    match0.StopOnMatch = DISABLE;
    match0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match0.MatchValue = 5000000;

    TIM_Init(LPC_TIM0 , TIM_TIMER , &timty);
    TIM_ConfigMatch(LPC_TIM0 , &match0);

    TIM_Cmd(LPC_TIM0, ENABLE);

    NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler(){

    UART_Send(LPC_UART0,(uint8_t *) mensaje , strlen(mensaje) - 1 , BLOCKING);

    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT); 

}

//TIMER CONIFIGURADO MATCH CADA 5 SEG
//HAY QUE QUITAR LA PARIDAD PORQUE SINO HAY QUE HACERLO EN LA PC
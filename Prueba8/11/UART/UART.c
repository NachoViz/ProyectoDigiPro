#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include <string.h>

// Mensaje a enviar. \r\n es para Salto de Línea en la terminal.
const char mensaje[] = "Holamundo\r\n";

// Prototipos
void configGPIO_UART(void);
void configUART(void);
void delay_simple(void); // Función de retardo simple

int main() {
    // 1. Configurar pines
    configGPIO_UART();
    
    // 2. Configurar periférico UART
    configUART();

    // 3. Bucle infinito
    while(1) {
        // Enviar el mensaje en modo "bloqueante"
        UART_Send(LPC_UART0, (uint8_t *)mensaje, strlen(mensaje), BLOCKING);
        
        // Esperar un poco antes de enviar el próximo
        delay_simple();
    }
    return 0; // Nunca llegará aquí
}

/**
 * @brief Configura P0.2 como TXD0 y P0.3 como RXD0
 */
void configGPIO_UART(void) {
    PINSEL_CFG_Type psel_conf;

    psel_conf.Portnum = 0;
    psel_conf.Pinmode = PINSEL_PINMODE_TRISTATE; // Modo normal, sin pull-up/down
    psel_conf.OpenDrain = PINSEL_PINMODE_NORMAL;

    // Configurar P0.2 como TXD0 (Función 01)
    psel_conf.Pinnum = 2;
    psel_conf.Funcnum = 1;
    PINSEL_ConfigPin(&psel_conf);

    // Configurar P0.3 como RXD0 (Función 01)
    psel_conf.Pinnum = 3;
    psel_conf.Funcnum = 1;
    PINSEL_ConfigPin(&psel_conf);
}

/**
 * @brief Configura la UART0 a 9600 8N1
 */
void configUART(void) {
    UART_CFG_Type uart_cfg;
    UART_FIFO_CFG_Type fifo_cfg;

    // Cargar configuración por defecto (9600 8N1)
    UART_ConfigStructInit(&uart_cfg);
    
    // Inicializar UART0
    UART_Init(LPC_UART0, &uart_cfg);

    // Cargar configuración FIFO por defecto
    UART_FIFOConfigStructInit(&fifo_cfg);

    // Inicializar FIFO
    UART_FIFOConfig(LPC_UART0, &fifo_cfg);

    // Habilitar transmisión
    UART_TxCmd(LPC_UART0, ENABLE);
}

/**
 * @brief Retardo simple por software (bloqueante)
 */
void delay_simple(void) {
    volatile uint32_t i;
    // Ajusta este número para un retardo más largo o corto
    for (i = 0; i < 5000000; i++); 
}
#include <msp430.h>
#include <stdint.h>
#include "types.h"

void uartOpen(u8 interface){
    if (interface == 1){
        UCA1CTL1 = UCSWRST;              // Reseta a interface para permitir a config.

        UCA1CTL0 = // UCPEN    |         // Habilita Paridade
                   // UCPAR    |         // PAR = 0
                   // UCMSB    |         // Manda pelo LSB primeiro, LSB até MSB, ou seja 0xAF manda o 1111 0101 padrão mundial  
                   // UC7BIT   |         // pacote de 8 bits 
                   // UCSPB    |         // Manda apenas 1 bit de STOP
                   UCMODE_0    |         // Modo UART
                   // UCSYNC   |         // uart é assincrono, então nao precisamos
                   0;

        UCA1CTL1 |=  UCSSEL__SMCLK;

        // Clock de entrada bate a 2^20 Hz
        // BAudrate = 9600 bits por segundo
        // BRW = 2^20/9600 = 109,23 > 32 (Cafe diz, quando é maior que 32 usar o oversampling)

        // --> Sem oversampling 16 
        // BRW = 109; BRS = 0,227*8 = 1,816 ~~ 2 (BRS usa sem oversampling)
        // Problema: A janela de amostragem é muito pequena

        // --> Modo de oversampling (OS16 = 1)
        // D = 2^20/ 9600 / 16 = 6,83
        // BRW = 6; BRF = 0,83 * 16 = 13,2 => 13

        UCA1BRW = 6;
        UCA1MCTL = UCBRF_13 | UCOS16;

        P4SEL |= BIT4 | BIT5;

        // e se fosse selecionado o ACLK @32768 Hz

        UCA1CTL1 &= ~UCSWRST; 
    }

};

// --> TX - UART - ´4.4
// --> TX - UART - ´4.5
void uartPrint(char * str){
    while(*str){
        while(!(UCA1IFG & UCTXIFG));    // Aguarda o buffer de transmissão ficar vazio
        UCA1TXBUF = *str++;
    }
}

#include <msp430.h>
#include <stdint.h>

volatile uint8_t rx_byte = 0;
volatile uint8_t rx_ok = 0;

char nome_tx[] = "Alan Henrique";
char nome_rx[20];

volatile int i_tx = 0;
volatile int i_rx = 0;
volatile int terminou = 0;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    // P3.3 = UCA0TXD
    // P3.4 = UCA0RXD
    P4SEL |= BIT4 | BIT5;

    // Coloca USCI_A0 em reset para configurar
    UCA0CTL1 |= UCSWRST;

    // Usa SMCLK como clock da UART
    UCA0CTL1 |= UCSSEL__SMCLK;

    // Baudrate 1200 bps com SMCLK ~= 1 MHz
    // 1000000 / 1200 = 833
    UCA0BR0 = 833 & 0xFF;   // 833 = 0x341 | 0x341 & 0xFF = 0x41
    UCA0BR1 = 833 >> 8;   // 833 = 0x341 | 0x341 = 0011 0100 0001 >> 8 = 0000 0000 0011 = 0x03
    UCA0MCTL = UCBRS_2; // modulação simples

    // Sai do reset
    UCA0CTL1 &= ~UCSWRST;

    // Habilita interrupção de recepção
    UCA0IE |= UCRXIE;

    __enable_interrupt();

    // Envia um byte qualquer
    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = nome_tx[i_tx++];

    while (1) {
        if (terminou) {
            terminou = 0;

            // Coloque breakpoint aqui e veja rx_byte
            __no_operation();
        }
    }
}

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) {
    switch (__even_in_range(UCA0IV, 4)) {
        case 0:
            break;

        case 2: // RXIFG
            nome_rx[i_rx] = UCA0RXBUF;

            if (nome_rx[i_rx] == '\0'){
              terminou = 1;
            }else {
              while (!(UCA0IFG & UCTXIFG));
              UCA0TXBUF = nome_tx[i_tx++];
            }

            i_rx++;
            break;

        case 4: // TXIFG
            break;
    }
}

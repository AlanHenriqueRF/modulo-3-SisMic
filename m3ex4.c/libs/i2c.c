#include "types.h"
#include <msp430.h>

void i2cConfig() {
  // coloca a interface em reset
  UCB0CTL1 = UCSWRST;

  UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC; // Interface é mestre, modo I2C síncrono.
  UCB0CTL1 |= UCSSEL__SMCLK;     // usa SMCLK @1MHz
  UCB0BRW = 100;                 // SCL @ SMCLK / 100 = 10KHz

  // SDA -> P3.0 --- SCL -> P3.1
  P3SEL |= BIT0 | BIT1; // P3.0 E P3.1 serão SDA e SCL respectivamente
  P3DIR &= ~(BIT0 | BIT1); // entrada (open-drain depende de pull-up)
  P3REN |= BIT0 | BIT1; // habilita resistor
  P3OUT |= BIT0 | BIT1; // resistor de pull-up

  UCB0CTL1 &= ~UCSWRST; // Zera o bit de RST para funcionamento da interface
}

// Envia 1 byte para o escravo. Retorna 0 se houve ACK, 1 se houve NACK.
u8 i2cSend(u8 addr, u8 byte) {
  UCB0IFG = 0; // Boa prática - Zerar o registro de flags antes

  UCB0I2CSA = addr; // Configura o endereço do escravo
  UCB0CTL1 |= UCTXSTT | UCTR; // Requisita o início da comunicação como Transmissor(TX)
  while (!(UCB0IFG & UCTXIFG)); // Escreve no buffer de Transmit (TX)

  UCB0TXBUF = byte;
  while (UCB0CTL1 & UCTXSTT); // Espera o ciclo de ACK/NACK acontecer

  UCB0CTL1 |= UCTXSTP; // Pede o stop
  while (UCB0CTL1 & UCTXSTP); // Espera enviar o stop

  return (UCB0IFG & UCNACKIFG);
}

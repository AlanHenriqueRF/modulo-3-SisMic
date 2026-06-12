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

// Varre o barramento. Escreve em addrs[] os enderecos de 7 bits que
// responderam com ACK e retorna a quantidade encontrada.
//
// Pula 0x00..0x07 e 0x78..0x7F porque sao enderecos reservados pela
// especificacao I2C (general call, 10-bit addressing, etc.).
// FALTA TESTE
// u8 i2cScan(uint8_t *addrs) {
//     uint8_t addr;
//     uint8_t count = 0;

//     for (addr = 0x08; addr <= 0x77; addr++) {
//         UCB0I2CSA  = addr;
//         UCB0IFG   &= ~UCNACKIFG;                  // limpa flag antes da tentativa

//         // START + STOP simultaneos como transmissor:
//         // o HW manda START, envia o byte de endereco, checa ACK/NACK e
//         // emite o STOP. Nada precisa ser escrito em TXBUF.
//         UCB0CTL1 |= UCTR | UCTXSTT | UCTXSTP;

//         // Aguarda o protocolo terminar (STOP de fato emitido)
//         while (UCB0CTL1 & UCTXSTP);

//         // Se nao houve NACK, anota o endereco
//         if (!(UCB0IFG & UCNACKIFG)) {
//             if (count < MAX_DEVS) {
//                 addrs[count] = addr;
//             }
//             count++;
//         }
//         UCB0IFG &= ~UCNACKIFG;
//     }

//     return (count > MAX_DEVS) ? MAX_DEVS : count;
// }

/* Aqui seria para configurar no USCI-B1 P4.1 e 4.2
   Sem resistor da placa, resistor fora */
// void i2cConfig_()
// {
//   UCB1CTL1  =  UCSWRST;        // Aplica o reset
//   UCB1CTL1 |=  UCSSEL__SMCLK;  // Configura o clock
//   UCB1CTL0  =  UCMODE_3 | UCMST; // Mestre I2C
//   P4SEL    |=  BIT2 | BIT1;       // Configura SDA e SCL da USCI-B1
//   UCB1BRW   =  10;               // Clock 1M/10 = @100kHz
//   UCB1CTL1 &= ~UCSWRST;
// }

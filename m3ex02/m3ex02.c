// Modulo 3 - Exercicio 2: Scanner de enderecos I2C
// Reaproveita a configuracao do exercicio 1 (USCI-B0 em P3.0/P3.1) e adiciona
// a funcao i2cScan(), que varre todos os enderecos validos de 7 bits e guarda
// no vetor passado os enderecos dos dispositivos que responderam com ACK.
//
// Tecnica usada na varredura: requisitar START e STOP ao mesmo tempo
// (UCTXSTT | UCTXSTP). O hardware envia START + endereco + STOP sem mandar
// nenhum byte de dado, ja que o TXBUF nao e tocado. Basta entao consultar
// UCNACKIFG para saber se alguem respondeu.
//
// Para visualizar o resultado sem terminal:
//   - o vetor `found` e o contador `found_count` sao globais => visiveis no
//     debugger do CCS;
//   - depois do scan o programa pisca o backlight do PRIMEIRO endereco
//     encontrado, em ~1 Hz. Se nada foi encontrado, fica idle.

#include <msp430.h>
#include <stdint.h>

#define MAX_DEVS    16   // capacidade do vetor de enderecos encontrados

// Prototipos
void    i2c_config(void);
uint8_t i2cSendesss(uint8_t addr, uint8_t data);
uint8_t i2cScan(uint8_t *addrs);
void    delay_500ms(void);

// Globais para inspecao no debugger
volatile uint8_t found[MAX_DEVS];
volatile uint8_t found_count;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    i2c_config();

    // Varre o barramento uma vez
    found_count = i2cScan((uint8_t *)found);

    // Demonstracao visual: se achou pelo menos um, pisca o backlight do primeiro
    while (1) {
        if (found_count > 0) {
            i2cSendesss(found[0], 0x08);   // backlight ON
            delay_500ms();
            i2cSendesss(found[0], 0x00);   // backlight OFF
            delay_500ms();
        }
    }
}

// Configura USCI-B0 como mestre I2C em P3.0 (SDA) / P3.1 (SCL).
void i2c_config(void) {
    UCB0CTL1 |= UCSWRST;
    UCB0CTL0  = UCMST | UCMODE_3 | UCSYNC;
    UCB0CTL1  = UCSSEL_2 | UCSWRST;     // SMCLK + reset
    UCB0BRW   = 12;                      // ~87 kHz

    P3SEL |= BIT0 | BIT1;
    P3DIR &= ~(BIT0 | BIT1);
    P3REN |= BIT0 | BIT1;
    P3OUT |= BIT0 | BIT1;

    UCB0CTL1 &= ~UCSWRST;
}

// Envia 1 byte (mesma implementacao do ex1). Retorna 0=ACK, 1=NACK.
uint8_t i2cSendesssesss(uint8_t addr, uint8_t data) {
    UCB0I2CSA = addr;
    UCB0CTL1 |= UCTR | UCTXSTT;

    while (!(UCB0IFG & UCTXIFG))
        ;
    UCB0TXBUF = data;

    while (UCB0CTL1 & UCTXSTT)
        ;

    uint8_t nack = (UCB0IFG & UCNACKIFG) ? 1 : 0;

    if (!nack) {
        while (!(UCB0IFG & UCTXIFG))
            ;
    }

    UCB0CTL1 |= UCTXSTP;
    while (UCB0CTL1 & UCTXSTP)
        ;
    UCB0IFG &= ~UCNACKIFG;
    return nack;
}

// Varre o barramento. Escreve em addrs[] os enderecos de 7 bits que
// responderam com ACK e retorna a quantidade encontrada.
//
// Pula 0x00..0x07 e 0x78..0x7F porque sao enderecos reservados pela
// especificacao I2C (general call, 10-bit addressing, etc.).
uint8_t i2cScan(uint8_t *addrs) {
    uint8_t addr;
    uint8_t count = 0;

    for (addr = 0x08; addr <= 0x77; addr++) {
        UCB0I2CSA  = addr;
        UCB0IFG   &= ~UCNACKIFG;                  // limpa flag antes da tentativa

        // START + STOP simultaneos como transmissor:
        // o HW manda START, envia o byte de endereco, checa ACK/NACK e
        // emite o STOP. Nada precisa ser escrito em TXBUF.
        UCB0CTL1 |= UCTR | UCTXSTT | UCTXSTP;

        // Aguarda o protocolo terminar (STOP de fato emitido)
        while (UCB0CTL1 & UCTXSTP)
            ;

        // Se nao houve NACK, anota o endereco
        if (!(UCB0IFG & UCNACKIFG)) {
            if (count < MAX_DEVS) {
                addrs[count] = addr;
            }
            count++;
        }
        UCB0IFG &= ~UCNACKIFG;
    }

    return (count > MAX_DEVS) ? MAX_DEVS : count;
}

void delay_500ms(void) {
    __delay_cycles(500000);
}

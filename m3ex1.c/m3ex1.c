// Modulo 3 - Exercicio 1: Teste da interface I2C
// Pisca a retroiluminacao do LCD 16x2 (modulo PCF8574) a ~1Hz.
//
// Ligacao fisica (MSP430F5529 LaunchPad -> backpack I2C do LCD):
//   LCD VCC -> 5V (TP1 do LaunchPad, ou pino 5V dos headers)
//   LCD GND -> GND
//   LCD SDA -> P3.0  (UCB0SDA)
//   LCD SCL -> P3.1  (UCB0SCL)
//
// Endereco do escravo: 0x27 (PCF8574T) ou 0x3F (PCF8574AT) -- trocar abaixo
// se o seu modulo nao responder. No mapa padrao do backpack, o bit P3 do
// expander controla o backlight: 0x08 = aceso, 0x00 = apagado.

#include <msp430.h>
#include <stdint.h>

#define SLAVE_ADDR  0x27        // tente 0x3F se nao houver ACK
#define BL_ON       0x08
#define BL_OFF      0x00

// Prototipos
void   i2c_config(void);
uint8_t i2cSend(uint8_t addr, uint8_t data);
uint8_t i2cSendN(uint8_t addr, const uint8_t *data, uint16_t nBytes); // bonus
void   delay_500ms(void);

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // desliga watchdog
    i2c_config();

    while (1) {
        i2cSend(SLAVE_ADDR, BL_ON);
        delay_500ms();
        i2cSend(SLAVE_ADDR, BL_OFF);
        delay_500ms();
    }
}

// Configura USCI-B0 como mestre I2C nos pinos P3.0/P3.1
void i2c_config(void) {
    // (1) coloca a interface em reset
    UCB0CTL1 |= UCSWRST;

    // (2) registros de controle: mestre, modo I2C, sincrono, SMCLK
    UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC;
    UCB0CTL1 = UCSSEL_2 | UCSWRST;      // SMCLK, mantem reset
    UCB0BRW  = 12;                       // SMCLK(~1MHz)/12 ~= 87 kHz (<100 kHz)

    // (3) pinos com funcao dedicada da USCI + pull-ups internos
    P3SEL |= BIT0 | BIT1;
    P3DIR &= ~(BIT0 | BIT1);             // entrada (open-drain depende de pull-up)
    P3REN |= BIT0 | BIT1;                // habilita resistor
    P3OUT |= BIT0 | BIT1;                // resistor como pull-up

    // (4) tira do reset
    UCB0CTL1 &= ~UCSWRST;
}

// Envia 1 byte para o escravo. Retorna 0 se houve ACK, 1 se houve NACK.
uint8_t i2cSend(uint8_t addr, uint8_t data) {
    // 1) endereco do escravo
    UCB0I2CSA = addr;

    // 2) transmissor + start
    UCB0CTL1 |= UCTR | UCTXSTT;

    // 3) espera o buffer de TX ficar vazio para liberar o ciclo de ACK do endereco
    while (!(UCB0IFG & UCTXIFG))
        ;
    UCB0TXBUF = data;

    // 4) espera o start ser concluido (STT volta a 0 quando o ACK do endereco chega)
    while (UCB0CTL1 & UCTXSTT)
        ;

    // 5) verifica se o escravo respondeu com ACK
    uint8_t nack = (UCB0IFG & UCNACKIFG) ? 1 : 0;

    if (!nack) {
        // 5a) espera o byte de dado ser transmitido
        while (!(UCB0IFG & UCTXIFG))
            ;
    }

    // 6) envia stop
    UCB0CTL1 |= UCTXSTP;

    // 7) espera o stop ser concluido
    while (UCB0CTL1 & UCTXSTP)
        ;

    // limpa flag de NACK para a proxima transacao
    UCB0IFG &= ~UCNACKIFG;

    // 8) retorna sinal de ACK/NACK
    return nack;
}

// Bonus: generalizacao para enviar N bytes na mesma transacao
uint8_t i2cSendN(uint8_t addr, const uint8_t *data, uint16_t nBytes) {
    if (nBytes == 0) return 0;

    UCB0I2CSA = addr;
    UCB0CTL1 |= UCTR | UCTXSTT;

    // primeiro byte destrava o ACK do endereco
    while (!(UCB0IFG & UCTXIFG))
        ;
    UCB0TXBUF = data[0];

    while (UCB0CTL1 & UCTXSTT)
        ;
    uint8_t nack = (UCB0IFG & UCNACKIFG) ? 1 : 0;

    uint16_t i = 1;
    while (!nack && i < nBytes) {
        while (!(UCB0IFG & UCTXIFG))
            ;
        UCB0TXBUF = data[i++];
        // detecta NACK no meio do envio
        if (UCB0IFG & UCNACKIFG) { nack = 1; break; }
    }

    // espera o ultimo byte sair se nao houve NACK
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

// Atraso ocupado de ~500 ms com SMCLK/DCO default ~1 MHz
void delay_500ms(void) {
    __delay_cycles(500000);
}

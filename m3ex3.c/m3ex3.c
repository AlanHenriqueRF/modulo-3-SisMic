// Modulo 3 - Exercicio 3: Escrita de 1 nibble no LCD (modo 4 bits via PCF8574)
//
// Mapa de bits do byte enviado ao PCF8574 (backpack padrao do LCD I2C):
//   bit 7..4 = D7..D4 do LCD (nibble de dado/instrucao em modo 4 bits)
//   bit 3    = backlight (1 = aceso). Mantido sempre em 1 para podermos ver.
//   bit 2    = EN  (Enable)
//   bit 1    = RW  (0 = escrita)
//   bit 0    = RS  (1 = caractere/dado, 0 = instrucao)  <- corresponde a isChar
//
// A escrita de 1 nibble exige 3 envios I2C (Figura 10 do enunciado):
//   1) RS e D[7:4] preparados com EN = 0 (e RW = 0)
//   2) mesma coisa com EN = 1   -> subida do enable
//   3) mesma coisa com EN = 0   -> descida do enable; o LCD amostra os dados
//
// Temporizacao: o periodo de bit do I2C a ~87 kHz e ~11.5 us, e cada i2cSend
// gasta ~250 us. Isso ja e MUITO maior que os requisitos do LCD
// (TEN >= 300 ns, TC >= 500 ns), entao nao precisa de delays adicionais entre
// os 3 envios.

#include <msp430.h>
#include <stdint.h>

#define LCD_ADDR    0x27        // troque para 0x3F se o seu chip for PCF8574AT
#define BL_BIT      0x08        // P3 do PCF8574 -> backlight
#define EN_BIT      0x04        // P2 -> EN
#define RW_BIT      0x02        // P1 -> R/W (escrita = 0)
#define RS_BIT      0x01        // P0 -> RS

// Prototipos
void    i2c_config(void);
uint8_t i2cSend(uint8_t addr, uint8_t data);
void    lcdWriteNibble(uint8_t nibble, uint8_t isChar);
void    delay_500ms(void);

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    i2c_config();

    // Teste da funcao: parte da sequencia de inicializacao do HD44780.
    // Envia 0x3 tres vezes (RS=0 => instrucao) para garantir que o LCD entre
    // no modo 8 bits a partir de qualquer estado inicial; depois envia 0x2
    // para passar ao modo 4 bits. Isso ainda nao e a inicializacao completa
    // (display/cursor/limpar) -- so exercita lcdWriteNibble.
    __delay_cycles(50000);          // ~50 ms apos o power-on do LCD
    lcdWriteNibble(0x3, 0);
    __delay_cycles(5000);           // ~5 ms (datasheet pede >= 4.1 ms apos o 1o 0x3)
    lcdWriteNibble(0x3, 0);
    __delay_cycles(200);            // >= 100 us
    lcdWriteNibble(0x3, 0);
    __delay_cycles(200);
    lcdWriteNibble(0x2, 0);         // entra em modo 4 bits
    __delay_cycles(200);

    // Loop principal: pisca backlight em ~1 Hz como "heartbeat" visual.
    while (1) {
        lcdWriteNibble(0x4, 0x0);
        lcdWriteNibble(0x1, 0x0);
    }
}

// Escreve 4 bits no LCD em modo 4 bits, pulsando EN. isChar => valor de RS.
// O backlight e mantido ligado para facilitar a inspecao visual; R/W = 0.
void lcdWriteNibble(uint8_t nibble, uint8_t isChar) {
    uint8_t base = ((nibble & 0x0F) << 4)           // D7..D4
                 | BL_BIT                            // backlight ON
                 | (isChar ? RS_BIT : 0);            // RS
    // R/W = 0 (RW_BIT nao setado) => escrita

    // 1) prepara dados com EN = 0
    i2cSend(LCD_ADDR, base);
    // 2) sobe EN mantendo o resto
    i2cSend(LCD_ADDR, base | EN_BIT);
    // 3) desce EN -> o LCD amostra D7..D4 na descida
    i2cSend(LCD_ADDR, base);
}

// --- Reaproveitado dos exercicios 1 e 2 ---

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

uint8_t i2cSend(uint8_t addr, uint8_t data) {
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

void delay_500ms(void) {
    __delay_cycles(500000);
}

// Modulo 3 - Exercicio 6 (Visto 1): Escreva seu nome no display
//
// Implementa:
//   lcdWriteNibble  (ex3) - envia 4 bits ao LCD via PCF8574 com pulso de EN
//   lcdWriteByte           - envia 1 byte como 2 nibbles (MSB primeiro)
//   lcdInit                - inicializa o HD44780 em modo 4 bits, 2 linhas
//   lcdWrite (ex6)         - escreve uma string, tratando '\n' e overflow
//
// Resultado esperado no LCD 16x2:
//   linha 1: "Alan Henrique"
//   linha 2: "Ex 6 - Visto 1"
//
// Para trocar o nome basta editar a string em main().

#include <msp430.h>
#include <stdint.h>

#define LCD_ADDR    0x27        // troque para 0x3F se nao houver ACK
#define BL_BIT      0x08        // P3 -> backlight
#define EN_BIT      0x04        // P2 -> EN
#define RS_BIT      0x01        // P0 -> RS  (RW = P1 fica em 0 = escrita)

#define LCD_COLS    16
#define LCD_ROWS    2

// Prototipos
void    i2c_config(void);
uint8_t i2cSend(uint8_t addr, uint8_t data);
void    lcdWriteNibble(uint8_t nibble, uint8_t isChar);
void    lcdWriteByte(uint8_t byte, uint8_t isChar);
void    lcdInit(void);
void    lcdWrite(char *str);

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    i2c_config();
    lcdInit();

    lcdWrite("alan  ");
    //lcdWrite("e Grazi ");

    while (1) {
        // lcdWrite("Alan Henrique ");
        // __delay_cycles(50000);
        // lcdWrite("e Grazi ");
        // __delay_cycles(50000);
        // nada a fazer; o conteudo fica fixo no LCD
    }
}

// Escreve uma string. Termina ao encontrar 0x00. Quebra de linha:
//   - '\n' explicito: pula para a linha de baixo
//   - apos 16 caracteres na linha atual: quebra automaticamente
// Se as duas linhas encherem, o restante e descartado.
void lcdWrite(char *str) {
    uint8_t col  = 0;
    uint8_t line = 0;

    while (*str != 0x00) {
        char c = *str;

        // Quebra explicita
        if (c == '\n') {
            if (line < LCD_ROWS - 1) {
                line++;
                col = 0;
                // Set DDRAM address: linha 2 comeca em 0x40
                lcdWriteByte(0x80 | (line * 0x40), 0);
                __delay_cycles(200);
            } else {
                // ja na ultima linha; ignora a quebra
            }
            str++;
            continue;
        }

        // Auto-quebra ao estourar a coluna
        if (col >= LCD_COLS) {
            if (line < LCD_ROWS - 1) {
                line++;
                col = 0;
                lcdWriteByte(0x80 | (line * 0x40), 0);
                __delay_cycles(200);
            } else {
                // sem espaco em nenhuma linha -> trunca
                break;
            }
        }

        lcdWriteByte((uint8_t)c, 1);   // escreve caractere (RS=1)
        col++;
        str++;
    }
}

// Inicializa o LCD HD44780 em modo 4 bits, 2 linhas, fonte 5x8.
// Sequencia conforme datasheet (pagina 46 do HD44780U).
void lcdInit(void) {
    // O LCD precisa de pelo menos ~40 ms apos a alimentacao subir
    __delay_cycles(50000);          // ~50 ms

    // ---- Forca modo 8 bits a partir de qualquer estado inicial ----
    lcdWriteNibble(0x3, 0);
    __delay_cycles(5000);           // >= 4.1 ms
    lcdWriteNibble(0x3, 0);
    __delay_cycles(200);            // >= 100 us
    lcdWriteNibble(0x3, 0);
    __delay_cycles(200);

    // ---- Configura interface para 4 bits ----
    lcdWriteNibble(0x2, 0);
    __delay_cycles(200);

    // A partir daqui os comandos sao bytes completos (2 nibbles).

    // Function set: 4 bits (M=0), 2 linhas (L=1), fonte 5x8 (F=0) -> 0010.1000
    lcdWriteByte(0x28, 0);
    __delay_cycles(200);

    // Display ON, cursor OFF, blink OFF -> 0000.1100
    lcdWriteByte(0x0C, 0);
    __delay_cycles(200);

    // Clear display -> 0000.0001 (leva 1.53 ms)
    lcdWriteByte(0x01, 0);
    __delay_cycles(2000);

    // Entry mode set: incrementa cursor, sem shift de display -> 0000.0110
    lcdWriteByte(0x06, 0);
    __delay_cycles(200);
}

// Envia 1 byte ao LCD como dois nibbles, MSB primeiro.
void lcdWriteByte(uint8_t byte, uint8_t isChar) {
    lcdWriteNibble((byte >> 4) & 0x0F, isChar);   // nibble alto (D7..D4)
    lcdWriteNibble(byte & 0x0F,        isChar);   // nibble baixo
}

// --- Reaproveitado do ex3 ---
void lcdWriteNibble(uint8_t nibble, uint8_t isChar) {
    uint8_t base = ((nibble & 0x0F) << 4)
                 | BL_BIT
                 | (isChar ? RS_BIT : 0);
    i2cSend(LCD_ADDR, base);              // EN = 0
    i2cSend(LCD_ADDR, base | EN_BIT);     // EN = 1
    i2cSend(LCD_ADDR, base);              // EN = 0 (LCD amostra na descida)
}

// --- Reaproveitado dos exercicios 1, 2 e 3 ---
void i2c_config(void) {
    UCB0CTL1 |= UCSWRST;
    UCB0CTL0  = UCMST | UCMODE_3 | UCSYNC;
    UCB0CTL1  = UCSSEL_2 | UCSWRST;
    UCB0BRW   = 12;                      // ~87 kHz com SMCLK default ~1 MHz

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

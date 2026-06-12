#include <msp430.h>
#include "lcd.h"
#include "i2c.h"
#include "types.h"


// conexão do LCD
// |      Nibble       | 
// | D7 | D6 | D5 | D4 | BT | EN | RW | RS |
void lcdWriteNibble(u8 nibble, u8 isChar){
    nibble <<= 4; // aqui priorizei o MSB do byte recebido no parametro se entra 0x2 sera levado para o i2c 0x20
    //                   | BT | EN | RW | RS |
    i2cSend(LCD, nibble | BT |  0 |  0 | isChar);
    i2cSend(LCD, nibble | BT | EN |  0 | isChar);
    i2cSend(LCD, nibble | BT |  0 |  0 | isChar);
}

void lcdWriteByte(u8 byte, u8 isChar){
    lcdWriteNibble(byte >> 4 , isChar);
    lcdWriteNibble(byte & 0x0f, isChar);
}

void lcdPrint(u8 * str){
    u8 line = 0;
    u8 col = 0;

    while (*str != 0x00) {
        char caracter = *str;

        // Quebra explicita
        if (caracter == '\n') {
            if (line < LCD_ROWS - 1) {
                col = 0;
                lcdWriteByte(0x80 | 0x40, INSTR); // intrução bebe para colocar a posição do cursor na segunda linha
            }
            line++;
            str++;
            continue;
        }

        // Auto-quebra ao estourar a coluna
        if (col >= LCD_COLS) {
            if (line < LCD_ROWS - 1) {
                line++;
                col = 0;
                lcdWriteByte(0x80 | 0x40, 0);
            } 
        }

        lcdWriteByte(caracter, 1);   // escreve caractere (RS=1)
        col++;
        str++;
    }
}

void lcdInit(){
    lcdWriteNibble(0x3, INSTR); // Garante que entre no
    lcdWriteNibble(0x3, INSTR); // modo 8 bits antes
    lcdWriteNibble(0x3, INSTR); // da inicialização

    lcdWriteNibble(0x2, INSTR); // RS is not CHAR, é uma instrução, e o 0x2 é justamente instrução para ativar o modo 4 bits

    lcdWriteByte(0x06, INSTR); // configura display
    lcdWriteByte(0x0F, INSTR); // Liga o display 
    lcdWriteByte(0x14, INSTR); // cursor anda para direita
    lcdWriteByte(0x28, INSTR); // Modo 4 bits com 2 linhas
    
    lcdWriteByte(0x01, INSTR); // CLEAR LCD 
}

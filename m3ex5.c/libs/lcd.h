#ifndef __LCD_H
#define __LCD_H

#include "types.h"

#define CHAR 1
#define INSTR 0

#define LCD 0x27

#define BT BIT3 // backlight luz da tela
#define EN BIT2 // enable, na borda de decida é enviado o nibble
#define RW BIT1 // Read ou Write, no momento queromos apenas escrever, então 0
#define RS BIT0 // informa ao lcd, se o nibble é de instrução (comando), ou um char

// Nibble = agrupamento de 4 bits
// emite o pulso de enable
//             <NibbleEnviado>
//           __
// EN -->  _|  |_

void lcdPrint(u8 * str);
void lcdInit();
void lcdWriteNibble(u8 nibble, u8 isChar);
void lcdWriteByte(u8 byte, u8 isChar);

#endif //__LCD_H

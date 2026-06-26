#ifndef __UART_H
#define __AURT_H
#include "types.h"

void uartOpen(u8 interface);
void uartPrint(char * str);
void uartPrintByte(u8 byte);

u8 uartReadByte();

#endif //__UART_H

#ifndef __ADC_H
#define __ADC_H

#include "types.h"
 
/**
 * @brief Recebe uma porta do pino 6 para fazer
 * a conversão de analog para dig.
 * @param port (0,7)
 * @return u16 
 */
u16 adcRead(u8 port);

/**
 * @brief Faz conversão ad, usando interrupção com o pino 6.0 e 6.1
 */
void initAdcInterrupt(void);


#endif //__ADC_H

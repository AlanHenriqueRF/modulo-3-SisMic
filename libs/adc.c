#include <msp430.h>
#include <stdint.h>

#include "types.h"

/**
 * @brief Faz conversão ad, usando interrupção com o pino 6.0 e 6.1
 */
void initAdcInterrupt() {
    P6SEL |= BIT0 | BIT1;

    //Desliga o môdulo
    ADC12CTL0 &= ~ADC12ENC;

    ADC12CTL0 = ADC12SHT0_3 | ADC12ON;

    ADC12CTL1 = ADC12CSTARTADD_0 |          // Guarda o resultado no MEM0
                ADC12SHS_0       |          // Usa o bit SC como trigger para inciar a conversão
                ADC12SHP         |          // Usa o timer interno do ADC12
                ADC12SSEL_0      |          // Usa o MODCLK @4.8MHz como clock do ADC
                ADC12CONSEQ_1;              // Modo: sequence of channels

    ADC12CTL2 = ADC12TCOFF |
                ADC12RES_0;      // 8 bits

    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0;   // X em P6.0/A0
    ADC12MCTL1 = ADC12SREF_0 | ADC12EOS | ADC12INCH_1;   // Y em P6.1/A1
    
    ADC12IE = ADC12IE0 | ADC12IE1;      // Liga a interrupção do canal 0 e do 1.

    //Liga o ADC.
    ADC12CTL0 |= ADC12ENC;
}

/**
 * @brief Recebe uma porta do pino 6 para fazer
 * a conversão de analog para dig.
 * @param port (0,7)
 * @return u16 
 */
u16 adcRead(u8 port){
    if (port > 7)
        return 0;

    // Seleciona apenas o pino desejado como entrada analógica
    P6SEL |= (1 << port); // constroi a mascara de bit BIT0, BIT1... 

    ADC12CTL0 &= ~ADC12ENC;                 // Zera o bit de ENC
    ADC12CTL0 = ADC12SHT0_3 |               // SHT = 16 batidas de clock (~3us)
                ADC12ON;                    // Liga o conversor AD
    
    ADC12CTL1 = ADC12CSTARTADD_0 |          // Guarda o resultado no MEM0
                ADC12SHS_0       |          // Usa o bit SC como trigger para inciar a conversão
                ADC12SHP         |          // Usa o timer interno do ADC12
                ADC12SSEL_0      |          // Usa o MODCLK @4.8MHz como clock do ADC
                ADC12CONSEQ_0;              // Modo: Single-channel, single-conversion

    ADC12CTL2 = ADC12TCOFF       |          // Desliga o sensor de temperatura (eco. energia)
                ADC12RES_0;                 // Usa resolução de 8 bits permite, x e y variar de [0, 255] ou 0x00 a 0xff

    ADC12MCTL0 = ADC12SREF_0      |         // Usa referência padrão de AVSS e AVCC
                 port;                      // Entrada no P6.{port}

    ADC12CTL0 |= ADC12ENC;                  // Habilita o trigger, enable conversion
    // SC -> start conversion
    ADC12CTL0 &= ~ADC12SC;                  // Força um flanco de subida
    ADC12CTL0 |=  ADC12SC;                  // Para o start conversion(SC)

    while(!(ADC12IFG & BIT0));              // Aguarda conversão terminar

    return ADC12MEM0;                       // retorna o valor convertido
}

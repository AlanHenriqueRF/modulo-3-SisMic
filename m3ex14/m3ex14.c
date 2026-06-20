#include <msp430.h>
#include <stdint.h>

#include "../libs/adc.h"

volatile u16 x; // led vermelho
volatile u16 y; // led verde

int main(void) {
  WDTCTL = WDTPW | WDTHOLD;

  P1DIR |= BIT0; // saida para led vermelho

  // Configure o mapeamento dos pinos da porta P4
  PMAPKEYID = 0x02D52;  // Escreve a senha para liberar a config do PMAP
  P4MAP3 = PM_TB0CCR1A; // Conecta P4.3 na saída do canal 1 do timer B
  P4MAP7 = PM_TB0CCR2A; // Conecta P4.7 na saída do canal 2 do timer B

  P4DIR |= BIT3 | BIT7; // coloca o led verde como saida, e a porta 3 como saida tbm para controlar o led vermelho via pwm
  P4SEL |= BIT3 | BIT7; // pwm por hardware p4.3 

  TB0CTL = TASSEL__ACLK | MC__UP | TBCLR;

  TB0CCR0 = 255; // passo de 256, [0, 255]

  TB0CCR1 = 128; // brilho inicial LED vermelho
  TB0CCR2 = 128; // brilho inicial LED verde

  TB0CCTL1 = OUTMOD_7; // reset/set PWM
  TB0CCTL2 = OUTMOD_7;

  initAdcInterrupt();
  __enable_interrupt();

  while (1) {
    ADC12CTL0 &= ~ADC12SC;
    ADC12CTL0 |= ADC12SC;
  }
}

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_interrupt(void)
{
    switch (_even_in_range(ADC12IV, 0x24)){
    case ADC12IV_ADC12IFG0:       //MEM0 Ready
        //Pego o valor no MEM0.
        x = ADC12MEM0; // apenas para debugger do adc
        TB0CCR1 = x;
        break;
    case ADC12IV_ADC12IFG1:      //MEM1 Ready
        y = ADC12MEM1; // apenas para debugger do adc
        TB0CCR2 = y;
        break;
    }
}

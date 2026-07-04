#include <msp430.h>

// Exercício 18: Controle remoto (receptor IR VS1838B / módulo KY-022, protocolo NEC)
//
// FIAÇÃO
//   "S"  -> P1.2   (entrada do sinal IR = TA0.1 / CCI1A, captura de hardware)
//   "+"  -> 3V3
//   "-"  -> GND
//
// PROTOCOLO (saída ATIVA EM NÍVEL BAIXO; medimos o tempo entre BORDAS DE DESCIDA):
//   início:    9 ms + 4,5 ms  -> 13,5 ms     bit '0': 1125 µs
//   repetição: 9 ms + 2,25 ms -> 11,25 ms    bit '1': 2250 µs
//   32 bits, 1º bit recebido vai para o MSB (montagem igual à da lib Arduino).
//
// CONTROLE (códigos do MEU controle):
//   ↑/↓  : + / - brilho do LED vermelho (P1.0)
//   →/←  : + / - brilho do LED verde   (P4.7)
//   OK   : acende os dois no máximo
//   0    : apaga os dois
//   1..9 : ajusta os dois juntos para o nível 1..9
//
// CLOCK: SMCLK no padrão (~1 MHz) => 1 tick ≈ 1 µs.

#define SINAL_RECEPTOR     BIT2   // P1.2 = TA0.1 (CCI1A)
#define LED_RED            BIT0   // P1.0
#define LED_GREEN          BIT7   // P4.7chat

// ---- Janelas de tempo (em µs ≈ ticks), folgadas de propósito ----
#define T_LEADER_MIN  12500u   // Pulso líder começa depois dos 12,5 ms
#define T_LEADER_MAX  15000u   // ... e antes dos 15 ms → confirma "é um start frame"

#define T_REPEAT_MIN  10000u   // Pulso de repetição (botão segurado): min 10 ms
#define T_REPEAT_MAX  12000u   // ... max 12 ms → fica abaixo do líder, sem confundir

#define T_BIT_MIN       700u   // Qualquer bit válido dura pelo menos 700 µs
#define T_BIT_MID      1700u   // fronteira entre '0' (~1125) e '1' (~2250)
#define T_BIT_MAX      2800u   // Qualquer bit válido dura no máximo 2800 µs

// ---- Códigos dos botões do MEU controle ----
#define BTN_UP     0x00FF18E7UL  // ↑
#define BTN_DOWN   0x00FF4AB5UL  // ↓
#define BTN_RIGHT  0x00FF5AA5UL  // →
#define BTN_LEFT   0x00FF10EFUL  // ←
#define BTN_OK     0x00FF38C7UL
#define BTN_0      0x00FF9867UL
#define BTN_1      0x00FFA25DUL
#define BTN_2      0x00FF629DUL
#define BTN_3      0x00FFE21DUL
#define BTN_4      0x00FF22DDUL
#define BTN_5      0x00FF02FDUL
#define BTN_6      0x00FFC23DUL
#define BTN_7      0x00FFE01FUL
#define BTN_8      0x00FFA857UL
#define BTN_9      0x00FF906FUL

// ---- PWM por software ----
#define PWM_PERIOD  1000u        // ~1 kHz em SMCLK ~1 MHz
#define LEVEL_MAX   9            // 10 níveis de brilho: 0 (apagado) .. 9 (máximo)

enum { ST_WAIT, ST_COLLECT };

// ---- Estado do decodificador IR (escrito na ISR do TA0) ----
volatile unsigned int  last_edge = 0;
volatile unsigned char state     = ST_WAIT;
volatile unsigned char bit_count = 0;
volatile unsigned long shift     = 0;
volatile unsigned long receptor_code   = 0;   // último código de 32 bits decodificado
volatile unsigned char receptor_ready  = 0;   // 1 = novo quadro completo
volatile unsigned char receptor_repeat = 0;   // 1 = código de repetição (botão segurado)

// ---- Estado do brilho (nível 0..9 de cada LED) ----
unsigned char level_red   = 0;
unsigned char level_green = 0;
unsigned long last_cmd    = 0;          // último comando aplicado (para a repetição)

// Converte nível 0..9 em duty para o PWM.
// nível 9 -> PWM_PERIOD (> TA1CCR0) faz o LED ficar sempre aceso.
static unsigned int level_to_duty(unsigned char level){
    return (unsigned int)(((unsigned long)level * PWM_PERIOD) / LEVEL_MAX);
}

static void update_pwm(void){
    TA1CCR1 = level_to_duty(level_red);    // duty do vermelho
    TA1CCR2 = level_to_duty(level_green);  // duty do verde
}

// Aplica um código de botão. Retorna 1 se reconheceu (para a lógica de repetição).
static unsigned char apply_command(unsigned long code){
    switch (code) {
        case BTN_UP:    if (level_red   < LEVEL_MAX) level_red++;   break;
        case BTN_DOWN:  if (level_red   > 0)         level_red--;   break;
        case BTN_RIGHT: if (level_green < LEVEL_MAX) level_green++; break;
        case BTN_LEFT:  if (level_green > 0)         level_green--; break;
        case BTN_OK:    level_red = LEVEL_MAX; level_green = LEVEL_MAX; break;
        case BTN_0:     level_red = 0;         level_green = 0;         break;
        case BTN_1: case BTN_2: case BTN_3:
        case BTN_4: case BTN_5: case BTN_6:
        case BTN_7: case BTN_8: case BTN_9: {
            static const unsigned long nums[9] = {
                BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9
            };
            unsigned char i;
            for (i = 0; i < 9; i++) {
                if (nums[i] == code) { level_red = level_green = i + 1; break; }
            }
            break;
        }
        default:
            return 0;   // botão não usado: ignora
    }
    update_pwm();
    return 1;
}

int main(void){
    WDTCTL = WDTPW | WDTHOLD; // interrompe whatdogs

    // LEDs como saída GPIO (PWM por software), apagados
    P1DIR |= LED_RED;
    P1OUT &= ~LED_RED;
    P4DIR |= LED_GREEN;
    P4OUT &= ~LED_GREEN;

    // P1.2 como entrada de captura do Timer_A0 (TA0.1 / CCI1A)
    P1DIR &= ~SINAL_RECEPTOR;
    P1SEL |=  SINAL_RECEPTOR;

    // TA0: captura na borda de descida do receptor infravermelho, contínuo, SMCLK (~1 µs/tick)
    TA0CCTL1 = CM_2 | CCIS_0 | SCS | CAP | CCIE;
    TA0CTL   = TASSEL__SMCLK | MC__CONTINUOUS | TACLR;

    // TA1: PWM por software dos LEDs, modo UP, SMCLK
    TA1CCR0  = PWM_PERIOD - 1;
    TA1CCTL0 = CCIE;            // início do período: liga os LEDs
    TA1CCTL1 = CCIE;            // duty do vermelho: desliga o vermelho
    TA1CCTL2 = CCIE;            // duty do verde:    desliga o verde
    update_pwm();
    TA1CTL   = TASSEL__SMCLK | MC__UP | TACLR;

    __enable_interrupt();

    while(1){
        if (receptor_ready) {
            receptor_ready = 0;
            if (apply_command(receptor_code))
                last_cmd = receptor_code;
        }
        if (receptor_repeat) {
            receptor_repeat = 0;
            // Segurar uma seta continua ajustando o brilho aos poucos
            if (last_cmd == BTN_UP   || last_cmd == BTN_DOWN ||
                last_cmd == BTN_LEFT || last_cmd == BTN_RIGHT)
                apply_command(last_cmd);
        }
    }
}

// ---- Decodificador NEC: captura das bordas de descida ----
#pragma vector = TIMER0_A1_VECTOR
__interrupt void isr_ir_capture(void){
    switch (TA0IV) {
        case TA0IV_TACCR1: {
            unsigned int now   = TA0CCR1;
            unsigned int delta = now - last_edge;   // subtração 16 bits (lida com wrap)
            last_edge = now;

            if (delta >= T_LEADER_MIN && delta <= T_LEADER_MAX) {
                state = ST_COLLECT;
                bit_count = 0;
                shift = 0;
            }
            else if (delta >= T_REPEAT_MIN && delta <= T_REPEAT_MAX) {
                receptor_repeat = 1;
                state = ST_WAIT;
            }
            else if (state == ST_COLLECT) {
                if (delta >= T_BIT_MIN && delta < T_BIT_MID) {
                    shift = (shift << 1);
                    bit_count++;            // bit '0'
                } else if (delta >= T_BIT_MID) {
                    shift = (shift << 1);
                     bit_count++;      // bit '1'
                } else {
                    
                    state = ST_WAIT; break;                        // fora de faixa: aborta
                }
                if (bit_count == 32) {
                    receptor_code = shift; 
                    receptor_ready = 1; 
                    receptor_repeat = 0; 
                    state = ST_WAIT;
                }
            }
            break;
        }
    }
}

// ---- PWM por software: liga os LEDs no início de cada período ----
#pragma vector = TIMER1_A0_VECTOR
__interrupt void isr_pwm_start(void){
    if (TA1CCR1 > 0) P1OUT |= LED_RED;     // só acende se o duty > 0
    if (TA1CCR2 > 0) P4OUT |= LED_GREEN;
}

// ---- PWM por software: desliga cada LED ao atingir seu duty ----
#pragma vector = TIMER1_A1_VECTOR
__interrupt void isr_pwm_duty(void){
    switch (TA1IV) {
        case TA1IV_TACCR1: P1OUT &= ~LED_RED;   break;
        case TA1IV_TACCR2: P4OUT &= ~LED_GREEN; break;
    }
}

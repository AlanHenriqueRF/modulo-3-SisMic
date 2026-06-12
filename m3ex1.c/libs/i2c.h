#ifndef __I2C_H
#define __I2C_H

#include "types.h"

/**
 * @brief Configura pino 3.0 (SDA) e 3.1(SCL) para funcionamento em I2C.
 * Resistor dos proprios pinos da placa ativo, e frequência de 10kHz.
 * @warning se for usar outros pinos, CONFIGURAR.
 * Se for usar com protobord, utilize resistores de pull-up 4,7k(ohm) para SDA e para SCL.
 * Nossos pinos do msp suportam apenas 3,3v, cuidado na conexão protobord
 */
void i2cConfig();

/**
 * @brief Envia 1 byte de informação para o escravo definido pelo addr 
 * e tras de volta o ack ou nack do periferico.
 * @param addr Endereço do periferico usado, geralmente vem por padrão de fábrica.
 * @param byte O byte de informação que quer enviar para o e periferico.
 * @return Retorna o ACK ou NACK do periferico, para saber se ele quer mais info ou não. 
 */
u8 i2cSend(u8 addr, u8 byte);

/**
 * @brief TESTAR. Percorre todos os posiveis endereços dos escravos conectados ao SDA
 * e retorna o numero de pessoas conectadas.
 * @param addrs 
 * @return u8 
 * @warning FALTA TESTE
 * @Draft
 */
u8 i2cScan(u8 * addrs);

#endif

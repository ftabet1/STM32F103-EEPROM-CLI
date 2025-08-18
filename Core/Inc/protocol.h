/*
 * protocol.h
 *
 *  Created on: Aug 14, 2025
 *      Author: tabet
 */

#ifndef INC_PROTOCOL_H_
#define INC_PROTOCOL_H_

#include "main.h"

#define 	PREFIX_CMD_LEN 		7
#define		MAX_PARAM_LEN		5

#define		PREFIX_CMD_STR		"eeprom "

#define		SPACE_CHAR			' '
#define		LF_CHAR				(char)0x0A

#define		FLAG_CHAR			'-'
#define		FLAG_WRITE			'w'		//must be followed with -a xxx & -v xxx flags
#define		FLAG_READ			'r'		//must be followed with -a xxx flag
#define		FLAG_ERASE			'e'
#define		FLAG_DUMP_ALL		'd'
#define		FLAG_ADDR			'a'
#define		FLAG_VALUE			'v'

#define		RX_STATE_IDLE		0
#define		RX_STATE_PREFIX		1
#define		RX_STATE_FLAG		2
#define		RX_STATE_FLAG_DATA	3

typedef struct
{
	uint8_t* p;
	uint32_t size;
	uint32_t cnt;
}uartSendTypedef;

typedef struct
{
	uint8_t busError;
	uint8_t	write;
	uint8_t read;
	uint8_t erase;
	uint8_t dumpAll;
	uint32_t addr;
	uint32_t value;
}cmdFlagsTypedef;

enum returnValue
{
	RV_ERROR,
	RV_OK
};

void uartRXNEHandler(USART_TypeDef* USART);
void uartTXEHandler(USART_TypeDef* USART, uartSendTypedef uartSendData);
uint8_t asciiToNum(char symbol, uint8_t* num); //Only decimal chars input

#endif /* INC_PROTOCOL_H_ */

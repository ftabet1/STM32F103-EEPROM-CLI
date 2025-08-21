/*
 * protocol.h
 *
 *  Created on: Aug 14, 2025
 *      Author: tabet
 */

#ifndef INC_PROTOCOL_H_
#define INC_PROTOCOL_H_

#include "main.h"

#define		MAX_PARAM_LEN		5	//max -a & -v parameter length

#define 	PREFIX_CMD_LEN 		7
#define		PREFIX_CMD_STR		"eeprom "	//command prefix

#define		SPACE_CHAR			' '
#define		LF_CHAR				(char)0x0A	//end command sign

#define		FLAG_CHAR			'-'
#define		FLAG_WRITE			'w'		//must be followed with -a xxx & -v xxx flags
#define		FLAG_READ			'r'		//must be followed with -a xxx flag
#define		FLAG_ERASE			'e'
#define		FLAG_DUMP_ALL		'd'
#define		FLAG_ADDR			'a'
#define		FLAG_VALUE			'v'

//RX state machine values
#define		RX_STATE_IDLE		0 //wait for input
#define		RX_STATE_PREFIX		1 //
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
	uint8_t busError; 	//must be set 1 when protocol error occured
	uint8_t	write;		//write flag (-w) detected
	uint8_t read;		//read 	flag (-r) detected
	uint8_t erase;		//erase flag (-e) detected
	uint8_t dumpAll;	//dumpAll flag (-d) detected
	uint32_t addr;		//addr flag  (-a) detected
	uint32_t value;		//value flag (-v) detected
}cmdFlagsTypedef;

enum returnValue
{
	RV_ERROR,
	RV_OK
};

void uartRXNEHandler(USART_TypeDef* USART); //RX not empty interrupt handler
void uartTXEHandler(USART_TypeDef* USART, uartSendTypedef uartSendData); //TX empty interrupt handler
uint8_t asciiToNum(char symbol, uint8_t* num); //Only decimal chars input

#endif /* INC_PROTOCOL_H_ */

/*
 * protocol.c
 *
 *  Created on: Aug 14, 2025
 *      Author: tabet
 */

#include "main.h"
#include "protocol.h"

const char cmdPrefix[PREFIX_CMD_LEN] = PREFIX_CMD_STR;

uint8_t rxState 	= 0; //current state
uint8_t rxCnt		= 0; //received bytes counter
uint8_t paramHandledFlag = 0;	//this flag display handled flag param or not

cmdFlagsTypedef flagTemp;
cmdFlagsTypedef flagStatus;

uartSendTypedef	uartSendData;

char 	prevFlag 				= (char)0;
char	*pCurrParam			= NULL;
char	valueParam[MAX_PARAM_LEN] = {(char)0};
char	addrParam[MAX_PARAM_LEN] = {(char)0};

void uartFaultReset()
{
	rxCnt = 0;
	memset(&flagTemp, 0, sizeof(cmdFlagsTypedef));
	rxState = RX_STATE_IDLE;
	flagStatus.busError = 1;
	return;
}

uint8_t asciiToNum(char symbol, uint8_t* num) //only decimal chars input
{
	if((symbol < 0x30) || (symbol > 39)) return RV_ERROR;
	else {*num = symbol - 0x30; return RV_OK;};
}

uint8_t uartSend(USART_TypeDef* USART, uartSendTypedef uartSendData, uint8_t* data, int size)
{
	if((USART->CR1 & USART_CR1_TXEIE) != 0) return 0;
	uartSendData.p = data;
	uartSendData.size = size;
	uartSendData.cnt = 0;
	USART->CR1 |= USART_CR1_TXEIE;
	return 1;
}

void uartRXNEHandler(USART_TypeDef* USART)
{
	uint32_t status = USART->SR;
		if((status & USART_SR_RXNE) != 0)
		{
			uint8_t rxData = USART->DR;

			switch(rxState)
			{
			case RX_STATE_IDLE:
				if(rxData != cmdPrefix[rxCnt++]) rxCnt = 0;
				if(rxCnt >= PREFIX_CMD_LEN) {rxState = RX_STATE_FLAG; rxCnt = 0;}
				break;
			case RX_STATE_FLAG:
				if(rxCnt == 0)
				{
					if(rxData == FLAG_CHAR) rxCnt++;	//go to flag type capture
					else if(rxData == LF_CHAR) //go to idle, command received
					{
						memcpy(&flagStatus, &flagTemp, sizeof(cmdFlagsTypedef));
						rxState = RX_STATE_IDLE;
					}
					else if(rxData != SPACE_CHAR) {uartFaultReset(); return;}//invalid flag prefix, go to fault handler
				}
				else if(rxCnt == 1)
				{
					switch(rxData) //flag type decoder
					{

					case FLAG_WRITE:
						pCurrParam = NULL;
						flagTemp.write = 1;
						break;
					case FLAG_READ:
						pCurrParam = NULL;
						flagTemp.read = 1;
						break;
					case FLAG_ERASE:
						pCurrParam = NULL;
						flagTemp.erase = 1;
						break;
					case FLAG_DUMP_ALL:
						pCurrParam = NULL;
						flagTemp.dumpAll = 1;
						break;
					case FLAG_ADDR:
						pCurrParam = addrParam;
						flagTemp.addr = 1;
						break;
					case FLAG_VALUE:
						pCurrParam = valueParam;
						flagTemp.value = 1;
						break;
					default:
						uartFaultReset(); //invalid flag, go to fault handler
						return;
						break;
					}

					if(pCurrParam != NULL)
					{
						rxState = RX_STATE_FLAG_DATA; //go to RX_STATE_FLAG_DATA state for param handling
						paramHandledFlag = 0;
						rxCnt = 0;
					}
					else rxCnt = 0; //initiate new flag capture
				}
				break;

			case RX_STATE_FLAG_DATA:
				if(rxData == FLAG_CHAR) {rxCnt = 1; rxState = RX_STATE_FLAG;} //going to flag type capture

				else if(rxData == SPACE_CHAR) {if(paramHandledFlag) {rxCnt = 0; rxState = RX_STATE_FLAG;}} //if param captured and received 'space', going to new flag capture
				else if(rxData == LF_CHAR) //if get LF char then copy flags to out struct
				{
					memcpy(&flagStatus, &flagTemp, sizeof(cmdFlagsTypedef));
					rxState = RX_STATE_IDLE;
				}
				else if(!((rxData < 0x30) || (rxData > 39))) //decimal char check
				{
					paramHandledFlag = 1;
					if(rxCnt == MAX_PARAM_LEN) {uartFaultReset(); return;}//wrong param length
					pCurrParam[rxCnt++] = rxData; //put param data
				}
				else if((rxData < 0x30) || (rxData > 39)) {uartFaultReset(); return;} //wrong param format
				break;
			default:
				break;

			}
		}
	return;
}

void uartTXEHandler(USART_TypeDef* USART, uartSendTypedef uartSendData)
{
	if(((USART->CR1 & USART_CR1_TXEIE) != 0) && ((USART->SR & USART_SR_TXE) != 0))
	{
		if(uartSendData.size > 0)
		{
			USART->DR = uartSendData.p[uartSendData.cnt++];
			uartSendData.size--;
		}
		else
		{
			USART->CR1 &= ~USART_CR1_TXEIE;
			uartSendData.cnt = 0;
		}
	}
}

void cmdHandler()
{
	if(flagStatus.busError)
	{
		flagStatus.busError = 0;
	}
	else if(flagStatus.dumpAll)
	{
		flagStatus.dumpAll = 0;
	}
	else if(flagStatus.read)
	{
		if(flagStatus.addr)
		{
			//check param
		}
		else
		{
			//invalid command
		}
	}
	else if(flagStatus.write)
	{
		if(flagStatus.addr && flagStatus.value)
		{

		}
		else
		{
			//invalid command
		}
	}
	else if(flagStatus.erase)
	{

	}

	memset(&flagStatus, 0, sizeof(cmdFlagsTypedef));
}


/*
 * protocol.c
 *
 *  Created on: Aug 14, 2025
 *      Author: tabet
 */

#include "main.h"
#include "protocol.h"
#include <math.h>

const char cmdPrefix[PREFIX_CMD_LEN] = PREFIX_CMD_STR;

uint8_t rxState 	= 0; //current state
uint8_t rxCnt		= 0; //received bytes counter
uint8_t paramHandledFlag = 0;	//this flag display handled flag param or not

cmdFlagsTypedef flagTemp;
cmdFlagsTypedef flagStatus;
uartSendTypedef	uartSendData;

char err[2] = "?\n";

char readValueStr[4] = {(char)0};

char 	prevFlag 				= (char)0;
char	*pCurrParam			= NULL;
char	valueParam[MAX_PARAM_LEN] = {(char)0};
char	addrParam[MAX_PARAM_LEN] = {(char)0};

void uartFaultReset()
{
	rxCnt = 0;
	memset(&flagTemp, 0, sizeof(cmdFlagsTypedef));
	memset(&flagStatus, 0, sizeof(cmdFlagsTypedef));
	rxState = RX_STATE_IDLE;
	flagStatus.busError = 1;
	TIM4->DIER &= ~TIM_DIER_UIE;
	return;
}

uint32_t powi(uint32_t a, uint32_t b)
{
	int32_t temp = 1;
	for(int i = 0; i < b; i++) temp *= a;
	return temp;
}

uint8_t asciiToNum(char symbol, uint8_t* num) //only decimal chars input
{
	if((symbol < 0x30) || (symbol > 0x39)) return RV_ERROR;
	else {*num = symbol - 0x30; return RV_OK;};
}

uint8_t numToHex(uint16_t num, char* str)
{
	uint32_t temp = 0;
	for(int i = 0; i < 4; i++)
	{
		temp = (num / powi(16,3-i)) & 0xF;
		str[i] = temp > 9 ? temp + 0x37 : temp + 0x30;
	}
	return RV_OK;
}

uint8_t paramToNum(char* param, uint16_t *result)
{
	uint32_t tempResult = 0;
	uint32_t tempNum 	= 0;
	int		 paramLen	= 0;
	for(int i = MAX_PARAM_LEN-1; i > -1; i--)
	{
		if(param[i] != 0)
		{
			tempNum = 0;
			if(!asciiToNum(param[i], (uint8_t*)&tempNum))  return RV_ERROR;

			for(int p = 0; p < paramLen; p++)  tempNum *= 10;
			paramLen++;
			tempResult += tempNum;
		}
	}

	if(paramLen == 0) return RV_ERROR;
	*result = tempResult;
	return RV_OK;
}


uint8_t uartSend(USART_TypeDef* USART, uartSendTypedef* uartSendData, uint8_t* data, int size)
{
	if((USART->CR1 & USART_CR1_TXEIE) != 0) return RV_ERROR;
	uartSendData->p = data;
	uartSendData->size = size;
	uartSendData->cnt = 0;
	USART->CR1 |= USART_CR1_TXEIE;
	return RV_OK;
}

void uartRXNEHandler(USART_TypeDef* USART, TIM_TypeDef *TIM)
{
	uint32_t status = USART->SR;
		if((status & USART_SR_RXNE) != 0)
		{
			uint32_t statu = TIM->SR;
			statu = TIM->SR;
			uint8_t rxData = USART->DR;
			timerStartup(TIM4);
			switch(rxState)
			{
			case RX_STATE_IDLE:
				if(rxData != cmdPrefix[rxCnt++]) rxCnt = 0;
				if(rxCnt >= PREFIX_CMD_LEN)
				{
					rxState = RX_STATE_FLAG;
					rxCnt = 0;
					memset(&flagTemp, 0, sizeof(cmdFlagsTypedef));
				}
				break;
			case RX_STATE_FLAG:
				if(rxCnt == 0)
				{
					if(rxData == FLAG_CHAR) rxCnt++;	//go to flag type capture
					else if(rxData == LF_CHAR) //go to idle, command received
					{
						memcpy(&flagStatus, &flagTemp, sizeof(cmdFlagsTypedef));
						TIM->DIER &= ~TIM_DIER_UIE;
						rxState = RX_STATE_IDLE;
						rxCnt = 0;
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
						memset(addrParam, 0, MAX_PARAM_LEN);
						flagTemp.addr = 1;
						break;
					case FLAG_VALUE:
						pCurrParam = valueParam;
						memset(valueParam, 0, MAX_PARAM_LEN);
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
					TIM->DIER &= ~TIM_DIER_UIE;
					rxState = RX_STATE_IDLE;
					rxCnt = 0;
				}
				else if(!((rxData < 0x30) || (rxData > 0x39))) //decimal char check
				{
					paramHandledFlag = 1;
					if(rxCnt == MAX_PARAM_LEN) {uartFaultReset(); return;}//wrong param length
					pCurrParam[rxCnt++] = rxData; //put param data
				}
				else if((rxData < 0x30) || (rxData > 0x39)) {uartFaultReset(); return;} //wrong param format
				break;
			default:
				break;
			}
		}
	return;
}

void uartTXEHandler(USART_TypeDef* USART, uartSendTypedef* uartSendData)
{
	if(((USART->CR1 & USART_CR1_TXEIE) != 0) && ((USART->SR & USART_SR_TXE) != 0))
	{
		if(uartSendData->size > 0)
		{
			USART->DR = uartSendData->p[uartSendData->cnt++];
			uartSendData->size--;
		}
		else
		{
			USART->CR1 &= ~USART_CR1_TXEIE;
			uartSendData->cnt = 0;
		}
	}
}

void timeoutHandler(TIM_TypeDef* TIM)
{
	if((TIM->SR & TIM_SR_UIF) != 0)
	{
		rxState = RX_STATE_IDLE;
		rxCnt = 0;
		TIM->SR = 1;
		TIM->DIER &= ~TIM_DIER_UIE;
		flagStatus.busError = 1;
	}
}

void timerStartup(TIM_TypeDef* TIM)
{
	TIM->DIER &= ~TIM_DIER_UIE;
	TIM->SR = 0;
	TIM->CNT = 1;
	TIM->EGR |= TIM_EGR_UG;
	TIM->SR = 0;
	TIM->DIER |= TIM_DIER_UIE;
}


void cmdHandler()
{
	if(flagStatus.busError)
	{
		flagStatus.busError = 0;
		//invalid command
		while(1)
		{
			if(uartSend(USART1, &uartSendData, (uint8_t*)err, strlen(err))) break;
		}
		memset(&flagStatus, 0, sizeof(cmdFlagsTypedef));
	}
	else if(flagStatus.dumpAll)
	{
		flagStatus.dumpAll = 0;
	}
	else if(flagStatus.read)
	{
		if(flagStatus.addr) //address data exist flag
		{
			uint16_t addr = 0;
			uint16_t data = 0;
			if(paramToNum(addrParam, &addr) == 0) {uartFaultReset(); return;}
			int16_t result = EE_ReadVariable((uint16_t)addr, &data);
			if(result != 0) {uartFaultReset(); return;}
			numToHex(data, readValueStr);
			while(1)
			{
				if(uartSend(USART1, &uartSendData, (uint8_t*)readValueStr, strlen(readValueStr))) break;
			}
			memset(&flagStatus, 0, sizeof(cmdFlagsTypedef));
		}
		else
		{
			//invalid command
			while(1)
			{
				if(uartSend(USART1, &uartSendData, (uint8_t*)err, strlen(err))) break;
			}
			memset(&flagStatus, 0, sizeof(cmdFlagsTypedef));
		}
	}
	else if(flagStatus.write)
	{
		if(flagStatus.addr && flagStatus.value)
		{
			uint16_t addr = 0;
			uint16_t data = 0;
			if(paramToNum(addrParam, &addr) == 0) {uartFaultReset(); return;}
			if(paramToNum(valueParam, &data) == 0) {uartFaultReset(); return;}
			int16_t result = EE_WriteVariable((uint16_t)addr, data);
			if(result != 0) {uartFaultReset(); return;}
			numToHex(data, readValueStr);
			while(1)
			{
				if(uartSend(USART1, &uartSendData, (uint8_t*)readValueStr, strlen(readValueStr))) break;
			}
			memset(&flagStatus, 0, sizeof(cmdFlagsTypedef));
		}
		else
		{
			//invalid command
			while(1)
			{
				if(uartSend(USART1, &uartSendData, (uint8_t*)err, strlen(err))) break;
			}
			memset(&flagStatus, 0, sizeof(cmdFlagsTypedef));
		}
	}
	else if(flagStatus.erase)
	{
		while(1)
		{
			if(uartSend(USART1, &uartSendData, (uint8_t*)err, strlen(err))) break;
		}
		memset(&flagStatus, 0, sizeof(cmdFlagsTypedef));
	}
}


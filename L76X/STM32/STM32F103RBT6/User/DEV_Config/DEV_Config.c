/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*                Used to shield the underlying layers of each master 
*                and enhance portability
*----------------
* |	This version:   V1.0
* | Date        :   2018-11-22
* | Info        :

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "DEV_Config.h"

/******************************************************************************
function:	
	Uart receiving and sending
******************************************************************************/
UBYTE DEV_Uart_ReceiveByte()
{ 
	UBYTE i[2];
	HAL_UART_Receive(&huart1, i, 1, 0XFFFF);
	return i[0];
}

void DEV_Uart_SendByte(char data)
{
	UBYTE i[2] = {data};
	HAL_UART_Transmit(&huart1, i, 1, 0xFFFF);
}

void DEV_Uart_SendString(char *data)
{
	UWORD i;
	
	for(i=0; data[i] != '\0'; i++){
		HAL_UART_Transmit(&huart1, data+i, 1, 0xFFFF);
	}
}

void DEV_Uart_ReceiveString(char *data, UWORD Num)
{
  HAL_UART_Receive(&huart1, data, Num, 0XFFFF);
	data[Num-1] = '\0';
}

void DEV_Set_GPIOMode(GPIO_TypeDef* GPIO_Port, UWORD Pin, UWORD mode)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	if(mode == 1){
		GPIO_InitStruct.Pin = Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIO_Port, &GPIO_InitStruct);
	}
	else if(mode == 0){
		GPIO_InitStruct.Pin = Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIO_Port, &GPIO_InitStruct);
	}
}


void DEV_Set_Baudrate(UDOUBLE Baudrate)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = Baudrate;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

}

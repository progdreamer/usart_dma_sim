#include "stm32f4xx_hal.h"
#include "uartRingBufDMA.h"
#include "string.h"

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;

#define UART huart2
#define DMA hdma_usart2_rx
#define RxBuf_SIZE 40
#define MainBuf_SIZE 40

uint8_t RxBuf[RxBuf_SIZE];
uint8_t MainBuf[MainBuf_SIZE];

uint16_t oldPos = 0;
uint16_t newPos = 0;

uint16_t Head, Tail;

int isDataAvailable = 0;

int isOK = 0;
int32_t TIMEOUT = 0;
void Ringbuf_Init (void)
{
	memset(RxBuf, '\0', RxBuf_SIZE);
	memset(MainBuf, '\0', MainBuf_SIZE);

	Head = Tail = 0;
	oldPos = 0;
	newPos = 0;

  HAL_UARTEx_ReceiveToIdle_DMA(&UART, RxBuf, RxBuf_SIZE);
  __HAL_DMA_DISABLE_IT(&DMA, DMA_IT_HT);
}
void Ringbuf_Reset (void)
{
	memset(MainBuf,'\0', MainBuf_SIZE);
	memset(RxBuf, '\0', RxBuf_SIZE);
	Tail = 0;
	Head = 0;
	oldPos = 0;
	newPos = 0;
	isOK = 0;
}

uint8_t checkString (char *str, char *buffertolookinto)
{
	int stringlength = strlen (str);
	int bufferlength = strlen (buffertolookinto);
	int so_far = 0;
	int indx = 0;
repeat:
	while (str[so_far] != buffertolookinto[indx])
	{
		indx++;
		if (indx>bufferlength) return 0;
	}

	if (str[so_far] == buffertolookinto[indx])
	{
		while (str[so_far] == buffertolookinto[indx])
		{
			so_far++;
			indx++;
		}
	}

	if (so_far != stringlength)
	{
		so_far =0;
		if (indx >= bufferlength) return 0;
		goto repeat;
	}

	if (so_far == stringlength) return 1;
	else return 0;
}

uint8_t isConfirmed (int32_t Timeout)
{
	TIMEOUT = Timeout;
	while ((!isOK)&&(TIMEOUT));
	isOK = 0;
	if (TIMEOUT <= 0) return 0;
	return 1;
}

int waitFor (char *string, uint32_t Timeout)
{
	int so_far =0;
	int len = strlen (string);

	TIMEOUT = Timeout;

	while ((Tail==Head)&&TIMEOUT);
	isDataAvailable = 0;

again:
	if (TIMEOUT <= 0) return 0;
	while (MainBuf[Tail] != string[so_far])
	{
		if (TIMEOUT <= 0) return 0;


		if (Tail == Head) goto again;
		Tail++;

		if (Tail==MainBuf_SIZE) Tail = 0;
	}
	while (MainBuf[Tail] == string[so_far])
	{
		if (TIMEOUT <= 0) return 0;
		so_far++;

		if (Tail == Head) goto again;
		Tail++;
		if (Tail==MainBuf_SIZE) Tail = 0;
		if (so_far == len) return 1;
	}

	HAL_Delay (100);

	if ((so_far!=len)&&isDataAvailable)
	{
		isDataAvailable = 0;
		goto again;
	}
	else
	{
		so_far = 0;
		goto again;
	}


	return 0;
}

int copyUpto (char *string, char *buffertocopyinto, uint32_t Timeout)
{
	int so_far =0;
	int len = strlen (string);
	int indx = 0;

	TIMEOUT = Timeout;
	while ((Tail==Head)&&TIMEOUT);
	isDataAvailable = 0;
again:

	if (TIMEOUT<=0) return 0;

	while (MainBuf[Tail] != string [so_far])
	{
		buffertocopyinto[indx] = MainBuf[Tail];

		if (Tail == Head) goto again;
		Tail++;
		indx++;
		if (Tail==MainBuf_SIZE) Tail = 0;
	}
	while (MainBuf[Tail] == string [so_far])
	{
		so_far++;
		buffertocopyinto[indx++] = MainBuf[Tail++];
		if (Tail==MainBuf_SIZE) Tail = 0;
		if (so_far == len) return 1;
	}

	HAL_Delay (100);

	if ((so_far!=len)&&isDataAvailable)
	{
		isDataAvailable = 0;
		goto again;
	}
	else
	{
		so_far = 0;
		goto again;
	}
    return 0;
}

int getAfter (char *string, uint8_t numberofchars, char *buffertocopyinto, uint32_t Timeout)
{
	if ((waitFor(string, Timeout)) != 1) return 0;
	HAL_Delay (100);
	for (int indx=0; indx<numberofchars; indx++)
	{
		if (Tail==MainBuf_SIZE) Tail = 0;
		buffertocopyinto[indx] = MainBuf[Tail++];
	}
	return 1;
}

void getDataFromBuffer (char *startString, char *endString, char *buffertocopyfrom, char *buffertocopyinto)
{
	int startStringLength = strlen (startString);
	int endStringLength   = strlen (endString);
	int so_far = 0;
	int indx = 0;
	int startposition = 0;
	int endposition = 0;

repeat1:

	while (startString[so_far] != buffertocopyfrom[indx]) indx++;
	if (startString[so_far] == buffertocopyfrom[indx])
	{
		while (startString[so_far] == buffertocopyfrom[indx])
		{
			so_far++;
			indx++;
		}
	}

	if (so_far == startStringLength) startposition = indx;
	else
	{
		so_far =0;
		goto repeat1;
	}

	so_far = 0;

repeat2:

	while (endString[so_far] != buffertocopyfrom[indx]) indx++;
	if (endString[so_far] == buffertocopyfrom[indx])
	{
		while (endString[so_far] == buffertocopyfrom[indx])
		{
			so_far++;
			indx++;
		}
	}

	if (so_far == endStringLength) endposition = indx-endStringLength;
	else
	{
		so_far =0;
		goto repeat2;
	}

	so_far = 0;
	indx=0;

	for (int i=startposition; i<endposition; i++)
	{
		buffertocopyinto[indx] = buffertocopyfrom[i];
		indx++;
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
		isDataAvailable = 1;

		oldPos = newPos;
		if (oldPos+Size > MainBuf_SIZE)
		{
			uint16_t datatocopy = MainBuf_SIZE-oldPos;
			memcpy ((uint8_t *)MainBuf+oldPos, (uint8_t *)RxBuf, datatocopy);

			oldPos = 0;
			memcpy ((uint8_t *)MainBuf, (uint8_t *)RxBuf+datatocopy, (Size-datatocopy));
			newPos = (Size-datatocopy);
		}

		else
		{
			memcpy ((uint8_t *)MainBuf+oldPos, (uint8_t *)RxBuf, Size);
			newPos = Size+oldPos;
		}

		if (Head+Size < MainBuf_SIZE) Head = Head+Size;
		else Head = Head+Size - MainBuf_SIZE;

		HAL_UARTEx_ReceiveToIdle_DMA(&UART, (uint8_t *) RxBuf, RxBuf_SIZE);
		__HAL_DMA_DISABLE_IT(&DMA, DMA_IT_HT);

	for (int i=0; i<Size; i++)
	{
		if ((RxBuf[i] == 'O') && (RxBuf[i+1] == 'K'))
		{
			isOK = 1;
		}
	}
}












#ifndef INC_UARTRINGBUFDMA_H_
#define INC_UARTRINGBUFDMA_H_

void Ringbuf_Init (void);

void Ringbuf_Reset (void);


uint8_t checkString (char *str, char *buffertolookinto);

uint8_t isConfirmed (int32_t Timeout);


int waitFor (char *string, uint32_t Timeout);


int copyUpto (char *string, char *buffertocopyinto, uint32_t Timeout);


int getAfter (char *string, uint8_t numberofchars, char *buffertocopyinto, uint32_t Timeout);




#endif

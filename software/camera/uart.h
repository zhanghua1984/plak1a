/*
 * uart.h
 *
 *  Created on: Oct 29, 2009
 *      Author: psampaio
 */

#ifndef UART_H_
#define UART_H_

#define UART0_RX_BUFFER_SIZE 1024         // UART0 receive buffer size
#define UART0_TX_BUFFER_SIZE 64        // UART0 transmit buffer size
#define UART1_RX_BUFFER_SIZE 64        // UART1 receive buffer size
#define UART1_TX_BUFFER_SIZE 64        // UART1 transmit buffer size

///////////////////////////////////////////////////////////////////////////////
// use the following macros to determine the 'baud' parameter values
// for uart0Init() and uart1Init()
// CAUTION - 'baud' SHOULD ALWAYS BE A CONSTANT or
// a lot of code will be generated.
#define UART_BAUD(baud) (unsigned short)((PCLK / ((baud) * 16.0)) + 0.5)

///////////////////////////////////////////////////////////////////////////////
// Definitions for typical UART 'baud' settings
#define B1200         UART_BAUD(1200)
#define B9600         UART_BAUD(9600)
#define B14400		  UART_BAUD(14400)
#define B19200        UART_BAUD(19200)
#define B38400        UART_BAUD(38400)
#define B57600        UART_BAUD(57600)
#define B115200       UART_BAUD(115200)

///////////////////////////////////////////////////////////////////////////////
// Definitions for typical UART 'mode' settings
#define UART_8N1      (unsigned char)(ULCR_CHAR_8 + ULCR_PAR_NO   + ULCR_STOP_1)
#define UART_7N1      (unsigned char)(ULCR_CHAR_7 + ULCR_PAR_NO   + ULCR_STOP_1)
#define UART_8N2      (unsigned char)(ULCR_CHAR_8 + ULCR_PAR_NO   + ULCR_STOP_2)
#define UART_7N2      (unsigned char)(ULCR_CHAR_7 + ULCR_PAR_NO   + ULCR_STOP_2)
#define UART_8E1      (unsigned char)(ULCR_CHAR_8 + ULCR_PAR_EVEN + ULCR_STOP_1)
#define UART_7E1      (unsigned char)(ULCR_CHAR_7 + ULCR_PAR_EVEN + ULCR_STOP_1)
#define UART_8E2      (unsigned char)(ULCR_CHAR_8 + ULCR_PAR_EVEN + ULCR_STOP_2)
#define UART_7E2      (unsigned char)(ULCR_CHAR_7 + ULCR_PAR_EVEN + ULCR_STOP_2)
#define UART_8O1      (unsigned char)(ULCR_CHAR_8 + ULCR_PAR_ODD  + ULCR_STOP_1)
#define UART_7O1      (unsigned char)(ULCR_CHAR_7 + ULCR_PAR_ODD  + ULCR_STOP_1)
#define UART_8O2      (unsigned char)(ULCR_CHAR_8 + ULCR_PAR_ODD  + ULCR_STOP_2)
#define UART_7O2      (unsigned char)(ULCR_CHAR_7 + ULCR_PAR_ODD  + ULCR_STOP_2)

///////////////////////////////////////////////////////////////////////////////
// Definitions for typical UART 'fmode' settings
#define UART_FIFO_OFF (0x00)
#define UART_FIFO_1   (unsigned char)(UFCR_FIFO_ENABLE + UFCR_FIFO_TRIG1)
#define UART_FIFO_4   (unsigned char)(UFCR_FIFO_ENABLE + UFCR_FIFO_TRIG4)
#define UART_FIFO_8   (unsigned char)(UFCR_FIFO_ENABLE + UFCR_FIFO_TRIG8)
#define UART_FIFO_14  (unsigned char)(UFCR_FIFO_ENABLE + UFCR_FIFO_TRIG14)

void uart0Init(unsigned short baud, unsigned char mode, unsigned char fmode);
int uart0Putch(int ch);
unsigned short uart0Space(void);
const char *uart0Puts(const char *string);
int uart0Write(const char *buffer, unsigned short count);
int uart0TxEmpty(void);
void uart0TxFlush(void);
int uart0Getch(void);

void uart1Init(unsigned short baud, unsigned char mode, unsigned char fmode);
int uart1Putch(int ch);
unsigned short uart1Space(void);
const char *uart1Puts(const char *string);
int uart1Write(const char *buffer, unsigned short count);
int uart1TxEmpty(void);
void uart1TxFlush(void);
int uart1Getch(void);

#endif /* UART_H_ */

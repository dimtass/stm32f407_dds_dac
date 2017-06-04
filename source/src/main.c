/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include <string.h>
#include <stdlib.h>

#include "stm32f4xx.h"
#include "platform_config.h"
#include "hw_config.h"
#include "internal_dac.h"
#include "dds.h"

tp_glb glb = {0};

void cmd_handler(uint8_t *buffer, size_t bufferlen, uint8_t sender)
{
	if (!strncmp((char*)buffer, "FREQ=1,", 7)) {
		glb.ch1_freq = strtof((char*) &buffer[7], NULL);
		TRACE(("Set CH1 frequency to: %f\n", glb.ch1_freq));
	}
	else if (!strncmp((char*)buffer, "FREQ=2,", 7)) {
		glb.ch2_freq = strtof((char*) &buffer[7], NULL);
		TRACE(("Set CH2 frequency to: %f\n", glb.ch2_freq));
	}
}

void main_loop(void)
{
	debug_uart_rx_poll();
}

int main(void)
{
	/* SysTick timer */
	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1){};	// Capture error
	}
	/* set trace levels */
	glb.trace_level =
			0
			| TRACE_LEVEL_DEFAULT
			;

	/* Configure GPIOs */
	GPIO_Configuration();

	debug_uart_init(USART1, 115200, glb.uart_rx_buff, UART_RX_BUFF_SIZE, glb.uart_tx_buff, UART_TX_BUFF_SIZE, &cmd_handler);
	debug_uart_set_trace_level(TRACE_LEVEL_DEFAULT, 1);

	glb.ch1_freq = 1000;
	glb.ch2_freq = 1000;

	/* Configure DACs */
	DDS_Init();
	DAC_init();

	TRACE(("Program started.\n"));
	TRACE(("System core clk: %lu\n", SystemCoreClock));

	while(1) {
		main_loop();
	}
}


/**
 * This is a (weak) function in syscalls.c and is used from printf
 * to print data to the UART1
 */
int __io_putchar(int ch)
{
	debug_uart_send(ch);
	return ch;
}

size_t send_string(uint8_t * buffer, size_t length)
{
	size_t i = 0;
	for (i=0; i<length; i++) {
		__io_putchar(buffer[i]);
	}
	return(i);
}



/**
  @file main.c
  @brief Free RTOS with D2XX
  An example of thread-safe D2XX library usage with FreeRTOS
 */
/*
 * ============================================================================
 * History
 * =======
 * Nov 2019		Initial beta for FT81x and FT80x
 * Mar 2020		Updated beta - added BT815/6 commands
 * Mar 2021     Beta with BT817/8 support added
 *
 *
 *
 *
 *
 * (C) Copyright,  Bridgetek Pte. Ltd.
 * ============================================================================
 *
 * This source code ("the Software") is provided by Bridgetek Pte Ltd
 * ("Bridgetek") subject to the licence terms set out
 * http://www.ftdichip.com/FTSourceCodeLicenceTerms.htm ("the Licence Terms").
 * You must read the Licence Terms before downloading or using the Software.
 * By installing or using the Software you agree to the Licence Terms. If you
 * do not agree to the Licence Terms then do not download or use the Software.
 *
 * Without prejudice to the Licence Terms, here is a summary of some of the key
 * terms of the Licence Terms (and in the event of any conflict between this
 * summary and the Licence Terms then the text of the Licence Terms will
 * prevail).
 *
 * The Software is provided "as is".
 * There are no warranties (or similar) in relation to the quality of the
 * Software. You use it at your own risk.
 * The Software should not be used in, or for, any medical device, system or
 * appliance. There are exclusions of Bridgetek liability for certain types of loss
 * such as: special loss or damage; incidental loss or damage; indirect or
 * consequential loss or damage; loss of income; loss of business; loss of
 * profits; loss of revenue; loss of contracts; business interruption; loss of
 * the use of money or anticipated savings; loss of information; loss of
 * opportunity; loss of goodwill or reputation; and/or loss of, damage to or
 * corruption of data.
 * There is a monetary cap on Bridgetek's liability.
 * The Software may have subsequently been amended by another user and then
 * distributed by that other user ("Adapted Software").  If so that user may
 * have additional licence terms that apply to those amendments. However, Bridgetek
 * has no liability in relation to those amendments.
 * ============================================================================
 */

/* INCLUDES ************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <ft900.h>
#include <ft900_dlog.h>
#include <ft900_memctl.h>

#include "EVE.h"

#include "tinyprintf.h"

#include "eve_example.h"


/**
 @brief Link to datalogger area defined in crt0.S file.
 @details Must be passed to dlog library functions to initialise and use
        datalogger functions. We use the datalogger area for persistent
        configuration storage.
 */
extern __flash__ uint32_t __dlog_partition[];

/* CONSTANTS ***********************************************************************/

/**
 @brief Page number in datalogger memory in Flash for touchscreen calibration
 values.
 */
#define CONFIG_PAGE_TOUCHSCREEN 0

/* LOCAL FUNCTIONS / INLINES *******************************************************/

void setup(void);
void debug_uart_init(void);
void tfp_putc(void* p, char c);

/* FUNCTIONS ***********************************************************************/

/**
 * @brief Functions used to store calibration data in flash.
 */
//@{
int8_t platform_calib_init(void)
{
	int	pgsz;
	int	i;
	int ret;

	ret = dlog_init(__dlog_partition, &pgsz, &i);
	if (ret < 0)
	{
		// Project settings incorrect. Require dlog support with modified
		// linker script and crt0.S file.
		// See AN_398 for examples.
		return -1;
	}
	return 0;
}

int8_t platform_calib_write(struct touchscreen_calibration *calib)
{
	uint8_t	flashbuf[260] __attribute__((aligned(4)));
	dlog_erase();

	calib->key = VALID_KEY_TOUCHSCREEN;
	memset(flashbuf, 0xff, sizeof(flashbuf));
	memcpy(flashbuf, calib, sizeof(struct touchscreen_calibration));
	if (dlog_prog(CONFIG_PAGE_TOUCHSCREEN, (uint32_t *)flashbuf) < 0)
	{
		// Flash not written.
		return -1;
	}

	return 0;
}

int8_t platform_calib_read(struct touchscreen_calibration *calib)
{
	uint8_t	flashbuf[260] __attribute__((aligned(4)));
	memset(flashbuf, 0x00, sizeof(flashbuf));
	if (dlog_read(CONFIG_PAGE_TOUCHSCREEN, (uint32_t *)flashbuf) < 0)
	{
		return -1;
	}

	if (((struct touchscreen_calibration *)flashbuf)->key == VALID_KEY_TOUCHSCREEN)
	{
		memcpy(calib, flashbuf, sizeof(struct touchscreen_calibration));
		return 0;
	}
	return -2;
}
//@}

int main(void)
{
	/* Setup UART */
	setup();

	/* Start example code */
	eve_example();

	// function never returns 
	for (;;) ;
}

void setup(void)
{
	// UART initialisation
	debug_uart_init();

#ifdef DEBUG
	/* Print out a welcome message... */
	tfp_printf ("(C) Copyright, Bridgetek Pte. Ltd. \r\n \r\n");
	tfp_printf ("---------------------------------------------------------------- \r\n");
	tfp_printf ("Welcome to BRT_AN_025 Example for FT9xx\r\n");
#endif
}

/** @name tfp_putc
 *  @details Machine dependent putc function for tfp_printf (tinyprintf) library.
 *  @param p Parameters (machine dependent)
 *  @param c The character to write
 */
void tfp_putc(void* p, char c)
{
	uart_write((ft900_uart_regs_t*)p, (uint8_t)c);
}

/* Initializes the UART for the testing */
void debug_uart_init(void)
{
	/* Enable the UART Device... */
	sys_enable(sys_device_uart0);
#if defined(__FT930__)
	/* Make GPIO23 function as UART0_TXD and GPIO22 function as UART0_RXD... */
	gpio_function(23, pad_uart0_txd); /* UART0 TXD */
	gpio_function(22, pad_uart0_rxd); /* UART0 RXD */
#else
	/* Make GPIO48 function as UART0_TXD and GPIO49 function as UART0_RXD... */
	gpio_function(48, pad_uart0_txd); /* UART0 TXD MM900EVxA CN3 pin 4 */
	gpio_function(49, pad_uart0_rxd); /* UART0 RXD MM900EVxA CN3 pin 6 */
	gpio_function(50, pad_uart0_rts); /* UART0 RTS MM900EVxA CN3 pin 8 */
	gpio_function(51, pad_uart0_cts); /* UART0 CTS MM900EVxA CN3 pin 10 */
#endif

	// Open the UART using the coding required.
	uart_open(UART0,                    /* Device */
			1,                        /* Prescaler = 1 */
			UART_DIVIDER_115200_BAUD,  /* Divider = 1302 */
			uart_data_bits_8,         /* No. buffer Bits */
			uart_parity_none,         /* Parity */
			uart_stop_bits_1);        /* No. Stop Bits */

	/* Print out a welcome message... */
	uart_puts(UART0,
			"\x1B[2J" /* ANSI/VT100 - Clear the Screen */
			"\x1B[H\r\n"  /* ANSI/VT100 - Move Cursor to Home */
	);

#ifdef DEBUG
	/* Enable tfp_printf() functionality... */
	init_printf(UART0, tfp_putc);
#endif
}


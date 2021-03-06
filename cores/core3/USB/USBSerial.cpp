/*
 * USBSerial.cpp
 *
 *  Created on: 18 Mar 2016
 *      Author: David
 */

#include "USBSerial.h"
#include "conf_usb.h"		// include this to check that the signatures of the callback functions are correct
#include "udi_cdc.h"		// Atmel CDC module

#include "udc.h" //For udc_detach()

// SerialCDC members

SerialCDC::SerialCDC() : /* _cdc_tx_buffer(), */ txBufsize(1), isConnected(false) {}

void SerialCDC::begin(uint32_t baud_count)
{
	// suppress "unused parameter" warning
	(void)baud_count;
}

void SerialCDC::begin(uint32_t baud_count, uint8_t config)
{
	// suppress "unused parameter" warning
	(void)baud_count;
	(void)config;
}

void SerialCDC::end()
{
	isConnected = false;
}

int SerialCDC::available()
{
	return (isConnected) ? udi_cdc_get_nb_received_data() : 0;
}

int SerialCDC::peek()
{
	return -1;				// not supported
}

int SerialCDC::read()
{
	return (udi_cdc_is_rx_ready()) ? udi_cdc_getc() : -1;
}

void SerialCDC::flush(void)
{
	while (isConnected && udi_cdc_get_free_tx_buffer() < txBufsize) {}
}

size_t SerialCDC::write(uint8_t c)
{
	if (isConnected)
	{
		udi_cdc_putc(c);
	}
	return 1;
}

// Non-blocking write to USB. Returns number of bytes written.
size_t SerialCDC::write(const uint8_t *buffer, size_t size)
{
	if (isConnected && size != 0)
	{
		size_t remaining = udi_cdc_write_buf(buffer, size);
		return size = remaining;
	}
	return size;
}

size_t SerialCDC::canWrite() const
{
	return (isConnected) ? udi_cdc_get_free_tx_buffer() : 0;
}

SerialCDC::operator bool() const
{
	return isConnected;
}

void SerialCDC::cdcSetConnected(bool b)
{
	isConnected = b;
}

void SerialCDC::cdcRxNotify()
{
	// nothing here until we use a Rx buffer
}

void SerialCDC::cdcTxEmptyNotify()
{
	// If we haven't yet found out how large the transmit buffer is, find out now
	if (txBufsize == 1)
	{
		txBufsize = udi_cdc_get_free_tx_buffer();
	}
}

// Declare the Serial USB device
SerialCDC SerialUSB;

// Callback glue functions, all called from the USB ISR

// This is called when we are plugged in and connect to a host
extern "C" bool core_cdc_enable(uint8_t port)
{
	SerialUSB.cdcSetConnected(true);
	return true;
}

// This is called when we get disconnected from the host
extern "C" void core_cdc_disable(uint8_t port)
{
	SerialUSB.cdcSetConnected(false);
}

// This is called when data has been received
extern "C" void core_cdc_rx_notify(uint8_t port)
{
	SerialUSB.cdcRxNotify();
}

// This is called when the transmit buffer has been emptied
extern "C" void core_cdc_tx_empty_notify(uint8_t port)
{
	SerialUSB.cdcTxEmptyNotify();
}

static void soft_reset() {
  #define RSTC_CR_KEY(value) ((RSTC_CR_KEY_Msk & ((value) << RSTC_CR_KEY_Pos)))
  const int RSTC_KEY = 0xA5;
  RSTC->RSTC_CR =
  RSTC_CR_KEY(RSTC_KEY) |
  RSTC_CR_PROCRST |
  RSTC_CR_PERRST;
}

static void enable_bootloader_magic() {
	#define BOOT_DOUBLE_TAP_ADDRESS           (IRAM_ADDR + IRAM_SIZE - 4)
	#define BOOT_DOUBLE_TAP_DATA              (*((volatile uint32_t *) BOOT_DOUBLE_TAP_ADDRESS))
	#define DOUBLE_TAP_MAGIC 0x07738135
	BOOT_DOUBLE_TAP_DATA = DOUBLE_TAP_MAGIC;
}

uint32_t g_cdcBaudRate;

extern "C" void core_cdc_set_coding_ext(uint8_t port, usb_cdc_line_coding_t *cfg) {
	g_cdcBaudRate = cfg->dwDTERate;
	if(g_cdcBaudRate == 1200) {
		enable_bootloader_magic();
		udc_detach();
		soft_reset();
	}
}

// End





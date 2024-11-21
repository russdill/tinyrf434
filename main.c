/*
 * Copyright (c) 2024 Russ Dill <russd@asu.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include <util/delay.h>

#include <string.h>

#define USB_HID_REPORT_TYPE_OUTPUT 2
#define USB_HID_REPORT_TYPE_FEATURE 3

#include <usbdrv.h>

#define MOSI_DDR DDRB
#define MOSI_BIT 1

#include "rf_usi.c"

const PROGMEM char usbDescriptorHidReport[] = {
#include HID_C
};

static unsigned char force_wdr;
static unsigned char report_type;
static unsigned char usb_buffer_len;
static unsigned char usb_buffer_pos;
static unsigned char tx_done_buf[2];
static bool transmit_pending;

uint8_t usbFunctionSetup(uint8_t data[8])
{
	usbRequest_t *rq = (void *) data;

#if VUSB_USING_VME
	/* Re-enter bootloader by triggering watchdog */
	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR) {
		wdt_enable(0);
	 	force_wdr = 1;
	}
#endif

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS)
		return 0;

	switch (rq->bRequest) {
	case USBRQ_HID_GET_REPORT:
		if (rq->wValue.bytes[0] == 2) {
			/* Status byte to indicate completion */
			tx_done_buf[1] = (transmit_pending << 7) | rf_protocol;
			usbMsgPtr = tx_done_buf;
			return sizeof(tx_done_buf);
		}
		break;

	case USBRQ_HID_SET_REPORT:
		usb_buffer_pos = 0;
		usb_buffer_len = rq->wLength.bytes[0] - 3;
		/* Maximum message length is preamble + 128 bits */
		if (usb_buffer_len > 16)
			usb_buffer_len = 16;
		report_type = rq->wValue.bytes[1];
		return USB_NO_MSG;
	}

	return 0;
}

uchar usbFunctionWrite(uchar *data, uchar orig_len)
{
	uchar len;
	switch (report_type) {
	case USB_HID_REPORT_TYPE_OUTPUT:
		if (transmit_pending) {
			report_type = 0xff;
			return USB_NO_MSG;
		}

		/* First packet in transaction */
		len = orig_len;
		if (!usb_buffer_pos) {
			rf_protocol = data[1];
			rf_msg_bit_len = data[2];
			data += 3;
			len -= 3;
		}

		/* Don't read past end of buffer */
		if (usb_buffer_pos + len > usb_buffer_len)
			len = usb_buffer_len - usb_buffer_pos;

		memcpy(rf_msg + usb_buffer_pos, data, len);
		usb_buffer_pos += len;

		/* Done, start the transfer */
		if (usb_buffer_pos == usb_buffer_len) {
			transmit_pending = true;
			report_type = 0;
			rf_usi_start();
			return 1;
		}
		return 0;

	case 0:
		return 1;

	default:
		return USB_NO_MSG;
	}

	return USB_NO_MSG;
}

USB_PUBLIC usbMsgLen_t usbFunctionDescriptor(usbRequest_t *rq)
{
	return 0;
}

int main(void)
{
#if !USB_CFG_USBINIT_CONNECT
	unsigned char i;
#endif

#if !VUSB_USING_VME
	OSCCAL = eeprom_read_byte(0);
#endif

	rf_usi_init();
	usbInit();

#if !USB_CFG_USBINIT_CONNECT
	usbDeviceDisconnect();
	i = 150;
	while (--i) {
		wdt_reset();
		_delay_ms(2);
	}
	usbDeviceConnect();
#endif

	sei();

	tx_done_buf[0] = 2;	/* Report ID */

	for (;;) {
		if (!force_wdr)
			wdt_reset();
		usbPoll();

		rf_usi_periodic();

		if (!usbConfiguration || !usbInterruptIsReady())
			continue;

		if (transmit_pending && !TCCR0B) {
			/* Notify completion */
			transmit_pending = false;
			tx_done_buf[1] = 0;
			usbSetInterrupt(tx_done_buf, sizeof(tx_done_buf));
		}
	}

}

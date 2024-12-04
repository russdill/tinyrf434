/*
 * Copyright (C) 2024 Russ Dill <russ.dill@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <stddef.h>

#include <avr/io.h>

#include "rf.c"

static void rf_usi_periodic(void)
{
	rf_periodic();

	/* Has counter overflowed to 16? */
	if (rf_needed_bits || !(USISR & _BV(USIOIF)))
		return;

	/* Sends MSB first. It's not documented, but USIBR is an output buffer
	 * register. */
	USIBR = rf_next();

	/*
	 * Set count at 8 + current pos so that we get an overflow
	 * flag when USIBR gets loaded into USIDR.
	 * (Note: If this bit is already set, we've had an underrurn)
	 */
	USISR |= 8;
	/* Clear overflow flag */
	USISR |= _BV(USIOIF);
	if (!rf_bit_pat_len_1) {
		/* Done */
		TCCR0B = 0;
		USICR = 0;
	}
}

static void rf_usi_start(void)
{
	/* Select Three-wire mode and Timer/Counter0 Compare Match clock */
	USICR = _BV(USIWM0) | _BV(USICS0);

	USIDR = 0;
	USIBR = 0;
	OCR0A = rf_start(); /* Indicates length of output bit period */
	USISR = _BV(USIOIF) | 15;

	/* Start timer */
	TCCR0B = TCNT0_PRESCALER_VAL;

}

static void rf_usi_init(void)
{
	/* Enable clear to compare mode */
	TCCR0A = _BV(WGM01);

	MOSI_DDR |= _BV(MOSI_BIT);
}

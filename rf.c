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

#include <stdbool.h>
#include <stddef.h>

#include "rf_protocols.c"

#define ARRAY_SIZE(n)   (sizeof(n) / sizeof((n)[0]))

static unsigned char rf_bit_pat_len_0; /* bit length of protocol's 0 pattern */
static unsigned char rf_bit_pattern_0; /* protocol's 0 pattern */
static unsigned char rf_bit_pat_len_1; /* bit length of protocol's 1 pattern */
static unsigned char rf_bit_pattern_1; /* protocol's 1 pattern */
static unsigned char rf_bit_pat_len; /* bits remaining of current 0/1 pattern */
static unsigned char rf_bit_pattern; /* current 0/1 pattern being shift out */
static unsigned char rf_preamble_pos; /* Current position within preamble */
static unsigned char rf_preamble_len; /* Total length of preamble */
static unsigned char rf_protocol; /* Currently selected rf_protocol */
static unsigned char rf_msg_pos; /* Byte position within the current msg */
static unsigned char rf_msg_bit; /* Bit position within current msg */
static unsigned char rf_msg_byte; /* Current msg byte being shifted out */
static unsigned char rf_msg_bit_len; /* Total length of message in bits */

static unsigned char rf_msg[16];

/* For building next output byte for shifting out on hardware */
static unsigned char rf_output_byte;
/* How many more bits are needed in the output byte */
static unsigned char rf_needed_bits;

/* Do little bits of work while "idle" */
static void rf_periodic(void)
{
	if (!rf_needed_bits)
		/* idle work done */
		return;

	if (rf_preamble_len) {
		/* Preamble goes an entire byte at a time */
		rf_preamble_len--;
		rf_output_byte = pgm_read_byte(rf_preambles + rf_preamble_pos++);
		rf_needed_bits = 0;
	} else if (rf_bit_pat_len) {
		/* Bit pattern is being shifted out into output byte */
		rf_output_byte = (rf_output_byte << 1) | !!(rf_bit_pattern & 0x80);
		rf_bit_pattern <<= 1;
		rf_bit_pat_len--;
		rf_needed_bits--;
	} else if (!rf_bit_pat_len_0) {
		/* rf_bit_pat_len_0 is special signal that indicates end of
		 * transfer and we need to flush the USI hardware */
		rf_output_byte = 0;
		rf_bit_pat_len_1 = 0;
		rf_needed_bits = 0;
	} else if (rf_msg_bit == rf_msg_bit_len) {
		/* Flush remaining message (these bits and then another 8) */
		rf_bit_pat_len = rf_needed_bits + 8;
		rf_bit_pattern = 0; /* Flushing is all zeros */
		rf_bit_pat_len_0 = 0; /* Mark as done */
	} else {
		if (!(rf_msg_bit++ % 8))
			/* Next byte from message */
			rf_msg_byte = rf_msg[rf_msg_pos++];

		/* Load new bit pattern from message bit */
		if (rf_msg_byte & 0x80) {
			rf_bit_pat_len = rf_bit_pat_len_1;
			rf_bit_pattern = rf_bit_pattern_1;
		} else {
			rf_bit_pat_len = rf_bit_pat_len_0;
			rf_bit_pattern = rf_bit_pattern_0;
		}
		rf_msg_byte <<= 1;
	}
}

static unsigned char rf_next(void)
{
	rf_needed_bits = 8;
	return rf_output_byte;
}

/* Hardware wants us to start the next message */
static unsigned char rf_start(void)
{
	const __flash struct rf_info *prot;
	if (rf_protocol >= ARRAY_SIZE(rf_protocols))
		rf_protocol = 0;
	for (prot = rf_protocols; rf_protocol; prot++, rf_protocol--);

	rf_bit_pat_len_0 = prot->bits_0;
	rf_bit_pattern_0 = prot->bit_0;
	rf_bit_pat_len_1 = prot->bits_1;
	rf_bit_pattern_1 = prot->bit_1;
	rf_bit_pat_len = 0;
	rf_preamble_pos = prot->preamble_pos;
	rf_preamble_len = prot->preamble_len;
	rf_msg_pos = 0;
	rf_msg_bit = 0;
	rf_needed_bits = 8;

	return prot->bit_ticks;
}

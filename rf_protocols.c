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

#include "rf_protocols.h"

/* These are preamble patterns for shifting out raw, MSB first */
static PROGMEM const char rf_preambles[] = {
	/* os10 12 1 bits, then sync of 4386us off, 5848us on, 5117us off */
	0x06, 0x66, 0x66, 0x66, 0x66, 0x66, 0x60, 0x7f, 0x80,
	/* os21 16 1 bits then LSB first 1010 */
	0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x96, 0x96,
	/* os30 24 1 bits then LSB first 1010 */
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x66,
	/* ar 620uS high, 620uS low repeated 4 times */
	0xe3, 0x8e, 0x38,
	/* aw2b 8 1's (500uS on, 1000uS off) */
	0x92, 0x49, 0x24,
	/* awf 11 1's, 0, then 1 */
	0x02, 0xaa, 0xaa, 0xa6,
	/* ar964tx Single 500uS pulse followed by 9000uS of silence (AcuRite 00964TX) */
	0x40, 0x00, 0x00,
	/* psm 500uS pulse, 1000uS silence repeated 3 times (Other PSM?) */
	0x01, 0x24,
};

/*
 * Because we store the preamble patterns in a contiguous array, we need to
 * do a little bit of manual tracking of where the offsets are in the array
 */
#define OS10_OFFSET 0
#define OS10_LEN 9

#define OS21_OFFSET OS10_OFFSET + OS10_LEN
#define OS21_LEN 10

#define OS30_OFFSET OS21_OFFSET + OS21_LEN
#define OS30_LEN 7

#define AR_OFFSET OS30_OFFSET + OS30_LEN
#define AR_LEN 3

#define AW2B_OFFSET AR_OFFSET + AR_LEN
#define AW2B_LEN 3

#define AWF_OFFSET AW2B_OFFSET + AW2B_LEN
#define AWF_LEN 4

#define AR964_OFFSET AWF_OFFSET + AWF_LEN
#define AR964_LEN 3

#define AR_PSM_OFFSET AR964_OFFSET + AR964_LEN
#define AR_PSM_LEN 2

enum {
	RF_PROTO_OS10,
	RF_PROTO_OS21,
	RF_PROTO_OS30,
	RF_PROTO_AR_PSM,
	RF_PROTO_AR_00964TX,
	RF_PROTO_AR_00609A1TX,
	RF_PROTO_AWF,
	RF_PROTO_WH2B,
	RF_PROTO_AR_PWM,
};

static PROGMEM const struct rf_info rf_protocols[] = {
	/* Oregon Scientific 1.0. */
	[RF_PROTO_OS10] = {
		/* Bit pattern length and MSB first bits for sending a 0 */
		4, 0b00110000,
		/* Bit pattern length and MSB first bits for sending a 1 */
		4, 0b11000000,
		/* Index and length of preamble */
		OS10_OFFSET, OS10_LEN,
		/* Time quanta for this protocol. Both the preamble and the
		 * bit pattern are shifted out at this rate */
		OS10_TICKS,
	},
	/* Oregon Scientific 2.1 */
	[RF_PROTO_OS21] = {
		4, 0b10010000, 4, 0b01100000,
		OS21_OFFSET, OS21_LEN,
		OS21_TICKS,
	},
	/* Oregon Scientific 3.0 */
	[RF_PROTO_OS30] = {
		2, 0b01000000, 2, 0b10000000,
		OS30_OFFSET, OS30_LEN,
		OS30_TICKS,
	},
	/* Oregon Scientific SL-109H
	 * AcuRite PSM (00955, 00782W3, 00606TX, 00609A1TX) */
	[RF_PROTO_AR_PSM] = {
		5, 0b10000000, 9, 0b10000000,
		AR_PSM_OFFSET, AR_PSM_LEN,
		AR_PSM_TICKS,
	},
	/* AcuRite PSM 00964TX */
	[RF_PROTO_AR_00964TX] = {
		5, 0b10000000, 9, 0b10000000,
		AR964_OFFSET, AR964_LEN,
		AR_PSM_TICKS,
	},
	/* Acurite PSM 00609A1TX */
	[RF_PROTO_AR_00609A1TX] = {
		3, 0b10000000, 5, 0b10000000,
		AR_PSM_OFFSET, AR_PSM_LEN,
		AR_PSM_TICKS,
	},
	/* Ambient Weather F007TH */
	[RF_PROTO_AWF] = {
		2, 0b01000000, 2, 0b10000000,
		AWF_OFFSET, AWF_LEN,
		AWF_TICKS,
	},
	/* Ambient Weather WH2B */
	[RF_PROTO_WH2B] = {
		5, 0b11100000, 3, 0b10000000,
		AW2B_OFFSET, AW2B_LEN,
		AW2B_TICKS,
	},
	/* AcuRite PWM (VN1TX, 00592TXR) */
	[RF_PROTO_AR_PWM] = {
		3, 0b10000000, 3, 0b11000000,
		AR_OFFSET, AR_LEN,
		AR_PWM_TICKS,
	},
};

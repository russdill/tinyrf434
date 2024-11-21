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

#ifndef RF_PROTOCOLS_H
#define RF_PROTOCOLS_H

/*
 * Our USI bit times range from 206.7uS to 731.0uS, which fits between a
 * clock select of 64*256/16.5MHz = 992uS and 8*256/16.5MHz = 124uS. So the
 * optimal clock select is /64, giving a per tick resolution of 3.879uS
 *
 * The 731.0uS timing comes from the 4x sampling of the 342Hz OS 1.0 protocol
 */
#define TCNT0_ROLLOVER_PERIOD_US (1000000 / (4 * 342))

#include "time.h"

struct rf_info {
	unsigned char bits_0;
	unsigned char bit_0;
	unsigned char bits_1;
	unsigned char bit_1;
	unsigned char preamble_pos;
	unsigned char preamble_len;
	unsigned char bit_ticks;
};

/* Oregon Scientific 1.0. */
#define OS10_TICKS	HZ_TO_JIFFIES_RND(342 * 4ULL)
/* Oregon Scientific 2.1 */
#define OS21_TICKS	HZ_TO_JIFFIES_RND(1024 * 2ULL)
/* Oregon Scientific 3.0 */
#define OS30_TICKS	HZ_TO_JIFFIES_RND(1024 * 2ULL)
/*
 * Oregon Scientific SL-109H
 * AcuRite PSM (00955, 00782W3, 00606TX, 00609A1TX)
 * AcuRite PSM 00964TX
 * Acurite PSM 00609A1TX
 */
#define AR_PSM_TICKS	US_TO_JIFFIES_RND(500ULL)
/* Ambient Weather F007TH */
#define AWF_TICKS	HZ_TO_JIFFIES_RND(1024 * 2ULL)
/* Ambient Weather WH2B */
#define AW2B_TICKS	US_TO_JIFFIES_RND(500ULL)
/* AcuRite PWM (VN1TX, 00592TXR) */
#define AR_PWM_TICKS	HZ_TO_JIFFIES_RND(1600 * 3ULL)

#endif

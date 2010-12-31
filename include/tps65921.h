/*
 * board/omap3621_boxer/tps65921_keypad.c
 *
 * Copyright (C) 2010 Barnes & Noble, Inc.
 * Intrinsyc Software International, Inc. on behalf of Barnes & Noble, Inc.
 *
 * Keypad read for u-boot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __INCLUDED_TPS65921_H
#define __INCLUDED_TPS65921_H

#include <common.h>

/* Register Offsets */
#define KEYP_CTRL			0x00
#define KEYP_DEB			0x01
#define KEYP_LONG_KEY		0x02
#define KEYP_LK_PTV			0x03
#define KEYP_TIMEOUT_L		0x04
#define KEYP_TIMEOUT_H		0x05
#define KEYP_KBC			0x06
#define KEYP_KBR			0x07
#define KEYP_SMS			0x08

/* KEYP_CTRL_REG Fields */
#define KEYP_CTRL_SOFT_NRST		(1 << 0)
#define KEYP_CTRL_SOFTMODEN		(1 << 1)
#define KEYP_CTRL_LK_EN			(1 << 2)
#define KEYP_CTRL_TOE_EN		(1 << 3)
#define KEYP_CTRL_TOLE_EN		(1 << 4)
#define KEYP_CTRL_RP_EN			(1 << 5)
#define KEYP_CTRL_KBD_ON		(1 << 6)

// Keep the key definitions consistend between hardware versions.
#define VOLUP_KEY (1 << 0)
#define VOLDN_KEY (1 << 1)
#define HOME_KEY  (1 << 2)

int tps65921_keypad_init(void);
int tps65921_keypad_reset(void);
int tps65921_keypad_keys_pressed(unsigned char *key);

#endif /* ! __INCLUDED_TPS65921_H */
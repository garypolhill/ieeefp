/*
    CIieeefp: x87FPUusys.h
    Copyright (C) 2003-2004, 2007  Macaulay Institute

    This file is part of CIieeefp, a partial implementation of the rounding
    control and exception checking IEEE routines for Cygwin on an Intel
    platform.

    CIieeefp is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    CIieeefp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details. (LICENCE file in
    this directory.)

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Contact information:
      Gary Polhill,
      Macaulay Institute, Craigiebuckler, Aberdeen, AB15 8QH. United Kingdom
      g.polhill@macaulay.ac.uk
*/

/* This file contains typedefs and macros needed for x87FPUutil.c
 */

#ifndef X87FPUUSYS_H
#define X87FPUUSYS_H

#include "x87FPUsys.h"

/* Status word access macros */

#define SW_B   0x8000U		/* FPU Busy */
#define SW_C3  0x4000U		/* Condition code MSB */
#define SW_TOP 0x3800U		/* Top of stack pointer */
#define SW_C2  0x0400U		/* Condition code */
#define SW_C1  0x0200U		/* Condition code */
#define SW_C0  0x0100U		/* Condition code LSB */
#define SW_ES  0x0080U		/* Error summary status */
#define SW_SF  0x0040U		/* Stack fault */
#define SW_PE  0x0020U		/* Precision exception flag */
#define SW_UE  0x0010U		/* Underflow exception flag */
#define SW_OE  0x0008U		/* Overflow exception flag */
#define SW_ZE  0x0004U		/* Division by zero exception flag */
#define SW_DE  0x0002U		/* Denormalisation exception flag */
#define SW_IE  0x0001U		/* Invalid operation exception flag */

#define SW_CC  0x4700U		/* All condition code flags */
#define SW_XF  0x003FU		/* All exception flags */

/* Control word access macros */

				/* 0xE000U reserved */
#define CW_X   0x1000U		/* Infinity control */
#define CW_RC  0x0C00U		/* Rounding control */
#define CW_PC  0x0300U		/* Precision control */
				/* 0x00C0U reserved */
#define CW_PM  0x0020U		/* Precision exception mask */
#define CW_UM  0x0010U		/* Underflow exception mask */
#define CW_OM  0x0008U		/* Overflow exception mask */
#define CW_ZM  0x0004U		/* Division by zero exception mask */
#define CW_DM  0x0002U		/* Denormalisation exception mask */
#define CW_IM  0x0001U		/* Invalid operation exception mask */

#define CW_XM  0x003FU		/* All exception masks */

#endif

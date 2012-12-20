/*
    CIieeefp: x87FPUutil.c
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

/* This file contains a series of utilities for accessing data stored
 * in the x87 FPU status and control words.
 */

#include "x87FPUusys.h"

/* get_X_word_flag(Yw, flag) -> flag value
 *
 * X = {status, control}, Y = {s, c}
 *
 * These two functions extract a flag value from a status or control
 * word. The control or status word and the required flag (one of the
 * macros from x87FPUusys.h) are passed in as argument. The word is
 * then ANDed with the flag to unset all bits not pertaining to the
 * required flag, and then both flag and word right shifted until the
 * flag has a 1 for the LSB. The word then contains the flag value.
 */

unsigned get_status_word_flag (x87FPU_status_word sw,
			       x87FPU_status_word flag) {
  sw &= flag;
  while((flag & 0x0001U) == 0x0000U) {
    sw >>= 1;
    flag >>= 1;
  }

  return sw;
}

unsigned get_control_word_flag (x87FPU_control_word cw,
				x87FPU_control_word flag) {
  cw &= flag;
  while((flag & 0x0001U) == 0x0000U) {
    cw >>= 1;
    flag >>= 1;
  }

  return cw;
}

/* set_control_word_flag(cw, flag, value) -> new control word
 *
 * Set a flag in the control word supplied as argument to the
 * specified value, returning the modified control word. This function
 * first unsets the flag bits in the control word by ANDing with the
 * ones complement of the flag. It then left shifts the value bits
 * until they are aligned with the flag bits through using a loop that
 * right shifts a copy of the flag bits until its LSB is 1,
 * left-shifting the value bits by the same amount. The value bits are
 * then ANDed with the flag bits to ensure that the value supplied as
 * argument is a valid setting for the flag before ORing the modified
 * control word with the value bits. 
 */

x87FPU_control_word set_control_word_flag(x87FPU_control_word cw,
					  x87FPU_control_word flag,
					  unsigned value) {
  x87FPU_control_word cw_value = (x87FPU_control_word)value;
  x87FPU_control_word dummy_flag = flag;

  cw &= (~flag);

  while((dummy_flag & 0x0001U) == 0x0000U) {
    cw_value <<= 1;
    dummy_flag >>= 1;
  }

  cw_value &= flag;
  return (cw | cw_value);
}

/* print_status_word(sw)
 *
 * Print out the various flag settings for the status word.
 */

void print_status_word(x87FPU_status_word sw) {
  printf("x87 FPU Status Word:\n\tFPU Busy: %u\n\tCondition Code: C3[%u] "
	 "C2[%u] C1[%u] C0[%u]\n\tTop of Stack Pointer: %u\n\tError Summary "
	 "Status: %u\n\tStack Fault: %u\n\tException Flags: IMP[%u] UF[%u] "
	 "OF[%u] DZ[%u] DNML[%u] INV[%u]\n",
	 get_status_word_flag(sw, SW_B),
	 get_status_word_flag(sw, SW_C3),
	 get_status_word_flag(sw, SW_TOP),
	 get_status_word_flag(sw, SW_C2),
	 get_status_word_flag(sw, SW_C1),
	 get_status_word_flag(sw, SW_C0),
	 get_status_word_flag(sw, SW_ES),
	 get_status_word_flag(sw, SW_SF),
	 get_status_word_flag(sw, SW_PE),
	 get_status_word_flag(sw, SW_UE),
	 get_status_word_flag(sw, SW_OE),
	 get_status_word_flag(sw, SW_ZE),
	 get_status_word_flag(sw, SW_DE),
	 get_status_word_flag(sw, SW_IE));
}

/* print_control_word(cw)
 *
 * Print out the various flag settings for the control word.
 */

void print_control_word(x87FPU_control_word cw) {
  printf("x87 FPU Control Word:\n\tInfinity Control: %u\n\tRounding Control: "
	 "%u\n\tPrecision Control: %u\n\tException Masks: IMP[%u] UF[%u] "
	 "OF[%u] DZ[%u] DNML[%u] INV[%u]\n",
	 get_control_word_flag(cw, CW_X),
	 get_control_word_flag(cw, CW_RC),
	 get_control_word_flag(cw, CW_PC),
	 get_control_word_flag(cw, CW_PM),
	 get_control_word_flag(cw, CW_UM),
	 get_control_word_flag(cw, CW_OM),
	 get_control_word_flag(cw, CW_ZM),
	 get_control_word_flag(cw, CW_DM),
	 get_control_word_flag(cw, CW_IM));
}

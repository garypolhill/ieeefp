/*
    CIieeefp: CIieeefp.c
    Copyright (C) 2004, 2007  Macaulay Institute

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

/* This file contains functions for implementing some of the POSIX
   standard functions for handling IEEE 754 floating point arithmetic
   that would typically appear in ieeefp.h */

#include <stdio.h>
#include <CIieeefp-sys.h>
#include "x87FPUutil.h"
#include "x87FPUcmds.h"

#define D2L (sizeof(double) / sizeof(long))
#define CC_UNS 0x0000U		/* no condition code flags set */
#define CC_NAN 0x0100U		/* C0 set */
#define CC_NRM 0x0400U		/* C2 set */
#define CC_INF 0x0500U		/* C2 | C0 set */
#define CC_ZRO 0x4000U		/* C3 set */
#define CC_MTY 0x4100U		/* C3 | C0 set */
#define CC_DNM 0x4400U		/* C3 | C2 set */

static fp_except saved_sticky_bits = 0;
				/* This is a global variable used to
                                   store the exception flags. This may
                                   affect the multi-threading safety
                                   of these functions. */

static const unsigned MASK_FP_BITS = (FP_X_INV | FP_X_DNML | FP_X_DZ
				      | FP_X_OFL | FP_X_UFL | FP_X_IMP);

/* fpgetround() -> rounding direction
 *
 * Return the current rounding direction as set in the control word of
 * the FPU.
 */

fp_rnd fpgetround(void) {
  fp_rnd round_bits = get_control_word_flag(x87FPU_fstcw(), CW_RC);
  
  switch(round_bits) {
  case FP_RN:
  case FP_RM:
  case FP_RP:
  case FP_RZ:
    return round_bits;
  default:
    fprintf(stderr, "PANIC: file %s line %d\n", __FILE__, __LINE__);
				/* Shouldn't ever get here! */
    abort();
  }
}

/* fpsetround(rnd_dir) -> previous rounding direction
 *
 * Set the rounding direction in the control word of the FPU to
 * rnd_dir, and return the previous setting. This function changes the
 * setting of the control word on the FPU chip. This requires the
 * exception flags in the status word to be cleared first. Thus before
 * changing the control word the exception flags need to be saved.
 */

fp_rnd fpsetround(fp_rnd rnd_dir) {
  fp_except current_sticky = (fp_except)get_status_word_flag(x87FPU_fstsw(),
							     SW_XF);
  x87FPU_control_word control_word = x87FPU_fstcw(); 
  fp_rnd old_rnd_dir = (fp_rnd)get_control_word_flag(control_word, CW_RC);

  saved_sticky_bits |= current_sticky;
				/* Save the sticky bits, because the
                                   call to x87FPU_fldcw will unset
                                   them all */

  switch(rnd_dir) {
  case FP_RN:
  case FP_RM:
  case FP_RP:
  case FP_RZ:
    control_word = set_control_word_flag(control_word, CW_RC,
					 (unsigned)rnd_dir);
    x87FPU_fldcw(control_word);
    break;
  default:
    fprintf(stderr, "fpsetround called with invalid rounding direction: "
	    "%hx\n", rnd_dir);
    abort();
  }

  switch(old_rnd_dir) {
  case FP_RN:
  case FP_RM:
  case FP_RP:
  case FP_RZ:
    return old_rnd_dir;
  default:
    fprintf(stderr, "PANIC: file %s line %d\n", __FILE__, __LINE__);
				/* Shouldn't ever get here! */
    abort();
  }
}

/* fpgetsticky() -> exception flags
 *
 * Return the current state of the floating point exception flags on
 * the FPU status word.
 */

fp_except fpgetsticky(void) {
  fp_except current_sticky = (fp_except)get_status_word_flag(x87FPU_fstsw(),
							     SW_XF);

  saved_sticky_bits |= current_sticky;

  return saved_sticky_bits;
}

/* fpsetsticky(sticky) -> previous exception flags
 *
 * Set the exception flags to the specified value. Return the previous
 * setting. In terms of the settings on the chip, this function just
 * clears all the exception flags. The setting passed as argument is
 * stored in saved_sticky_bits, the global variable to this file,
 * which is used to save flag settings from other accesses to the FPU
 * -- in particular, those involving fldcw, which requires an fclex
 * beforehand.
 */

fp_except fpsetsticky(fp_except sticky) {
  fp_except current_sticky = (fp_except)get_status_word_flag(x87FPU_fstsw(),
							     SW_XF);
  x87FPU_status_word sw = SW_XF;

  current_sticky |= saved_sticky_bits;
 
  /* Ensure that sticky contains a valid setting of the exception
     flags. Do this by left shifting sticky until it aligns with the
     exception flag position in the FPU. (This should happen exactly
     zero times because the exception flags happen to occupy the least
     significant six bits in the status word, but I'm just doing this
     for safety). Then unset all non-exception flag bits by AND-ing
     with the exception flag mask SW_XF. Finally, right-shift sticky
     back to where it was before. */
  while((sw & 0x0001) == 0x0000) {
    sticky <<= 1;
    sw >>= 1;
  }
  sticky &= SW_XF;
  sw = SW_XF;
  while((sw & 0x0001) == 0x0000) {
    sticky >>= 1;
    sw >>= 1;
  }

  saved_sticky_bits = sticky;
  x87FPU_fclex();		/* Clear the exception flags on chip */

  return current_sticky;
}

/* fpgetmask() -> exception masks
 *
 * Return the current setting of the exception mask bits in the
 * control word of the FPU.
 */

fp_except fpgetmask(void) {
  return (fp_except)(get_control_word_flag(x87FPU_fstcw(), CW_XM)
		     ^ MASK_FP_BITS);
}

/* fpsetmask(mask) -> previous exception mask
 *
 * Set the exception mask bits in the control word of the FPU to the
 * specified value. Return the previous setting. This function uses
 * the fldcw instruction to make the setting, which requires fclex to
 * be executed, clearing the exception flags in the status word. We
 * therefore need to save the current exception flag settings on the
 * chip before changing the control word.
 */

fp_except fpsetmask(fp_except mask) {
  x87FPU_control_word cw = x87FPU_fstcw();
  fp_except old_mask = get_control_word_flag(cw, CW_XM);
  fp_except current_sticky = (fp_except)get_status_word_flag(x87FPU_fstsw(),
							     SW_XF);
  
  saved_sticky_bits |= current_sticky;
				/* Save the sticky bits because the
                                   call to x87FPU_fldcw will clear
                                   them on the chip. */
  cw = set_control_word_flag(cw, CW_XM, (unsigned)mask ^ MASK_FP_BITS);
  x87FPU_fldcw(cw);

  return old_mask ^ MASK_FP_BITS;
}

/* fpclass(dsrc) -> class of floating point number
 *
 * This function uses the fxam instruction to get back the class of
 * floating point number passed in as argument.
 */

fpclass_t fpclass(double dsrc) {
  x87FPU_status_word sw;
  union {
    double d;
    long l[D2L];
  } tmp1, tmp2;
  int i;
 
  sw = x87FPU_fxam(dsrc);

  switch(sw & (SW_C3 | SW_C2 | SW_C0)) {
  case CC_UNS:
    return FP_INTEL_UNSUPPORTED;
  case CC_NAN:
    /* Need to check for Signalling vs Quiet NaN. A Signalling NaN has
       a 0 as the most significant bit of the significand.
       
       The method used below should be independent of machine
       architecture (big or little endian), but does rely on IEEE 754
       standard double precision format being used to represent the
       numbers. Hopefully this will work regardless of the mode of
       precision the x87 FPU has been set to work in, but we'll see...

       To begin with, dsrc contains the original number. This is
       copied into the double part of the tmp1 union. We know at this
       stage that it is a NaN, so it has all 11 bits of the exponent
       set to 1. The state of the sign bit and exponent are unknown.

       If we AND dsrc with +1.5, then we can convert the number into
       a result that will tell us easily whether or not the original
       number was SNaN or QNaN. The result will take the sign and
       exponent of +1.5 (since all exponent bits of NaNs are 1s),
       will have the most significant bit of the significand set to
       that of dsrc, and the rest 0s (from +1.5). Thus, the result will
       be 1.0 for an SNaN, and 1.5 for a QNaN.

       The following shows the procedure using binary representation
       of each double. Y...(n)...Y is shorthand for digit Y repeated n
       times.

                         sign exponent----- significand--
       dsrc:                X 11...(10)...1 ZX...(51)...X
       +1.5:                0 01...(10)...1 10...(51)...0

       A = dsrc AND +1.5:   0 01...(10)...1 Y0...(51)...0

       A = 1.0 if Y = 0, and Y = 0 if SNaN (Z = 0)
       A = 1.5 if Y = 1, and Y = 1 if QNaN (Z = 1)
       
    */
    memcpy(&tmp1.d, &dsrc, sizeof(double));
    tmp2.d = 1.5;
    for(i = 0; i < D2L; i++) tmp1.l[i] &= tmp2.l[i];
				/* AND dsrc with +1.5... */

    if(tmp1.d == 1.0) return FP_SNAN;
    else return FP_QNAN;

    /* In many ways all this is a waste of time. The signal should
       have been generated from the signalling NaN by the time the
       function is called (merely setting a variable to an SNaN causes
       the signal to be sent), with the effect that any variable
       having an SNaN in it will be converted to a QNaN. This function
       will only ever return QNaN therefore... */
  case CC_NRM:
    return ((sw & SW_C1) == 0U) ? FP_PNORM : FP_NNORM;
  case CC_INF:
    return ((sw & SW_C1) == 0U) ? FP_PINF : FP_NINF;
  case CC_ZRO:
    return ((sw & SW_C1) == 0U) ? FP_PZERO : FP_NZERO;
  case CC_MTY:
    fprintf(stderr, "PANIC: ST(0) empty!\n");
    abort();
  case CC_DNM:
    return ((sw & SW_C1) == 0U) ? FP_PDENORM : FP_NDENORM;
  default:
    fprintf(stderr, "PANIC: Invalid condition code flag status: %sx\n",
	    sw & (SW_C3 | SW_C2 | SW_C0));
    abort();
  }
}

/* finite(num) -> boolean
 *
 * Returns 1 (true) if the argument is a finite number, and 0 otherwise.
 */

int finite(double num) {
  switch(fpclass(num)) {
  case FP_PDENORM:
  case FP_NDENORM:
  case FP_PNORM:
  case FP_NNORM:
  case FP_PZERO:
  case FP_NZERO:
    return 1;
  case FP_PINF:
  case FP_NINF:
  case FP_SNAN:
  case FP_QNAN:
    return 0;
  default:
    fprintf(stderr, "PANIC: fpclass returned invalid class\n");
    abort();
  }
}


/******************************************************************************
 * Additional (non-POSIX standard) functions for Intel FPU
 */

/* fpgetprecision() -> current FPU precision setting
 *
 * The Intel FPU works in extended double mode by default, even if the
 * numbers concerned are double precision. In this mode, if two
 * doubles are added, for example, each is converted to an 80-bit long
 * double when put onto the FPU stack before the addition takes
 * place. In some cases this can generate different results than might
 * be expected from standard double precision operations, and contrary
 * to what you might think (and the Intel manual suggests), results
 * that are sometimes further from the true value than if the
 * calculation had taken place with 64-bit precision.
 *
 * This function allows you to access the current precision mode
 * setting, which may be single (32-bit) precision, double (64-bit)
 * precision, or extended double (80-bit) precision.
 */

fp_pctl fpgetprecision(void) {
  fp_pctl current_precision = (fp_pctl)get_control_word_flag(x87FPU_fstcw(),
							     CW_PC);

  switch(current_precision) {
  case FP_PC_SGL:
  case FP_PC_RES:		/* Reserved setting of the precision control */
  case FP_PC_DBL:
  case FP_PC_EXT:
    return current_precision;
  default:
    abort();			/* Shouldn't ever get here!) */
  }
}

/* fpsetprecision(pctl) -> previous FPU precision setting
 *
 * See premble to fpgetprecision comments for more background on why
 * this function is needed.
 *
 * This function allows you to set the precision mode on the FPU to
 * the required value. Note that even if you set the precision to
 * double rather than extended double, you are still not guaranteed to
 * get the same result on two different floating point chip makes for
 * the same operation.
 */

fp_pctl fpsetprecision(fp_pctl pctl) {
  fp_except current_sticky = (fp_except)get_status_word_flag(x87FPU_fstsw(),
							     SW_XF);
  x87FPU_control_word control_word = x87FPU_fstcw();
  fp_pctl old_pctl = (fp_pctl)get_control_word_flag(control_word, CW_PC);

  saved_sticky_bits |= current_sticky;
				/* Save the sticky bits, because the
                                   call to x87FPUfldcw will unset them
                                   all */

  switch(pctl) {
  case FP_PC_SGL:
  case FP_PC_DBL:
  case FP_PC_EXT:
    control_word = set_control_word_flag(control_word, CW_PC, (unsigned)pctl);
    x87FPU_fldcw(control_word);
    break;
  case FP_PC_RES:
    fprintf(stderr, "fpsetprecision called with reserved precision control"
	    "setting\n");
    abort();
  default:
    fprintf(stderr, "fpsetprecision called with invalid precision control: "
	    "%hx\n", pctl);
    abort();
  }

  switch(old_pctl) {
  case FP_PC_SGL:
  case FP_PC_RES:
  case FP_PC_DBL:
  case FP_PC_EXT:
    return old_pctl;
  default:
				/* Shouldn't ever get here! */
    abort();
  }
}

/* print_fpu_status()
 *
 * This function is provided for diagnostic purposes. It prints out
 * the current setting of all the bits in the status word of the FPU.
 */

void print_fpu_status(void) {
  print_status_word(x87FPU_fstsw());
}

/* print_fpu_control()
 *
 * This function is provided for diagnostic purposes. It prints out
 * the current setting of all the bits in the control word of the FPU.
 */

void print_fpu_control(void) {
  print_control_word(x87FPU_fstcw());
}

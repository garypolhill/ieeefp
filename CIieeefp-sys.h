/*
    CIieeefp: CIieeefp-sys.h
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
#ifndef CIIEEEFP_SYS_H
#define CIIEEEFP_SYS_H

/* Typedefs */

typedef unsigned fp_rnd;
typedef unsigned fp_except;
typedef unsigned fp_pctl;
typedef enum { FP_SNAN = 0, FP_QNAN, FP_NINF, FP_PINF, FP_NDENORM,
	       FP_PDENORM, FP_NZERO, FP_PZERO, FP_NNORM, FP_PNORM,
	       FP_INTEL_UNSUPPORTED } fpclass_t;

/* Exceptions */

#define FP_X_INV  0x01U
#define FP_X_DNML 0x02U
#define FP_X_DZ   0x04U
#define FP_X_OFL  0x08U
#define FP_X_UFL  0x10U
#define FP_X_IMP  0x20U

/* Rounding */

#define FP_RN 0U
#define FP_RM 1U
#define FP_RP 2U
#define FP_RZ 3U

/* Precision control */

#define FP_PC_SGL 0U
#define FP_PC_RES 1U
#define FP_PC_DBL 2U
#define FP_PC_EXT 3U

#endif

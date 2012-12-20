/*
    CIieeefp: CIieeefp.h
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
#ifndef CIIEEEFP_H
#define CIIEEEFP_H

#include <CIieeefp-sys.h>

/* POSIX functions */

extern fp_rnd fpgetround(void);
extern fp_rnd fpsetround(fp_rnd rnd_dir);
extern fp_except fpgetsticky(void);
extern fp_except fpsetsticky(fp_except sticky);
extern fp_except fpgetmask(void);
extern fp_except fpsetmask(fp_except mask);
extern fpclass_t fpclass(double dsrc);
extern int finite(double num);

/* Intel specific functions and utilities */

extern fp_pctl fpgetprecision(void);
extern fp_pctl fpsetprecision(fp_pctl pctl);
extern void print_fpu_status(void);
extern void print_fpu_control(void);

#endif

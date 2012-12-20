/*
    CIieeefp: test-CIieeefp.c
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
#ifdef __CYGWIN__
#include <CIieeefp.h>
#else
#include <ieeefp.h>
typedef unsigned short fp_pctl;
fp_pctl fpsetprecision(fp_pctl x) {
  return x;
}
const fp_pctl FP_PC_DBL = 0;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <float.h>
#include <math.h>

#include <sys/types.h>
#include <netinet/in.h>
#ifndef __CYGWIN__
#include <inttypes.h>
#else
#include <stdint.h>
#endif

#define STRNAN "**not-a-number**"

#define DBL2LNG sizeof(double) / sizeof(long)
				/* So, we're assuming doubles are a whole
				   number of times bigger than longs...
				*/
#define DBL2CHR sizeof(double) / sizeof(char)

#define FAIL_TEST { printf("X"); fflush(stdout); failures++; } else { printf("."); fflush(stdout); }

typedef union {
  double num;
  uint32_t hst[DBL2LNG];
} NUMBER;

typedef union {
  uint32_t net[DBL2LNG];
  unsigned char bytes[DBL2CHR];
} PRINTER;

typedef enum { PLUS = 0, MINUS, DIVIDE, MULTIPLY } OPERATOR;

#define MAXXLEN 5
#define MAXOPLEN 2
char operators[4][MAXOPLEN] = { "+", "-", "/", "*" };
#define MAXRNDLEN 2
char rnddir[4][MAXRNDLEN] = { "N", "M", "P", "Z" };
fp_rnd fpdir[4] = { FP_RN, FP_RM, FP_RP, FP_RZ };
#define MAXFPCLS 8
char fpcls[10][MAXFPCLS] = { "SNaN", "QNan", "-Inf", "+Inf", "-Denorm", 
			     "+Denorm", "-0", "+0", "-Norm", "+Norm" };

/* Global variables */

static double negten = -10.0;
static double negtwo = -2.0;
static double negone = -1.0;
static double negzero = -0.0;
static double zero = 0.0;
static double one = 1.0;
static double two = 2.0;
static double three = 3.0;
static double ten = 10.0;
static double two53 = 9007199254740992.0;
static double two_12 = 0.000244140625;
static double two53ptwo = 9007199254740994.0;

/* print(number)
 *
 * Print a floating point number in hex, in a platform independent
 * way. This is designed such that each number has a unique value when
 * printed, with the exception of NaNs, which are all printed in the
 * same way. (There are many different values for NaN, and different
 * platforms given different bit values for the same operation.)
 */

void print(double number, char *buf) {
  NUMBER in;
  PRINTER out;
  int i;

  if(isnan(number)) {
    sprintf(buf, STRNAN);
    return;
  }
  in.num = number;
  for(i = 0; i < DBL2LNG; i++) {
#ifdef __CYGWIN__
    out.net[DBL2LNG - i - 1] = htonl(in.hst[i]);
#else
    out.net[i] = htonl(in.hst[i]);
#endif
  }
  for(i = 0; i < DBL2CHR; i++) {
    sprintf(buf, "%02x", (unsigned int)out.bytes[i]);
    buf += 2;
  }
}

/* input(number)
 *
 * Allow a floating point number to be entered uniquely in hex, MSB to LSB.
 */

double input(char *buf) {
  PRINTER in;
  NUMBER out;
  int i;
  char *cbuf, *cp, *cq;

  if(strcmp(buf, STRNAN) == 0) {
    buf = "ffffffffffffffff";
  }
  cbuf = (char *)malloc((((strlen(buf) * 3) / 2) + 2) * sizeof(char));
  for(cp = cbuf, cq = buf;
      (*cq) != '\0' && (*(cq + 1)) != '\0';
      cp += 3, cq += 2) {
    (*cp) = (*cq);
    (*(cp + 1)) = (*(cq + 1));
    (*(cp + 2)) = ' ';
  }
  for(cp = cbuf, i = 0; i < DBL2CHR; i++) {
    unsigned int z;

    sscanf(cp, "%x", &z);
    if(z > 255) {
      fprintf(stderr, "Error in input stream: %ux is not a valid char.\n", z);
      abort();
    }
    cp += 3;
    in.bytes[i] = (unsigned char)z;
  }
  free(cbuf);

  for(i = 0; i < DBL2LNG; i++) {
#ifdef __CYGWIN__
    out.hst[DBL2LNG - i - 1] = ntohl(in.net[i]);
#else
    out.hst[i] = ntohl(in.net[i]);
#endif
  }

  return out.num;
}

/* op(num1, num2, op, dir)
 *
 * Perform arithmetic operator op on operands num1 and num2 in
 * rounding direction dir. Check for floating point errors. Since the
 * IEEE standard allows some ambiguity in which flags are set to
 * indicate which floating point errors, overflow, underflow and
 * imprecision are comflated to give the same indication.
 */

void op(double num1, double num2, OPERATOR op, int dir, FILE *fp,
	int *nopdiffs, int *nansdiffs, int *nxdiffs, int *nclsdiffs) {
  double ans1;
  char snum1[(DBL2CHR * 2) + 1], snum2[(DBL2CHR * 2) + 1],
    sans1[(DBL2CHR * 2) + 1];
  fp_except x;
  fp_pctl pc;

  pc = fpsetprecision(FP_PC_DBL);
  fpsetsticky(0);
  if(dir >= 0 && dir <= 3) {
    fpsetround(fpdir[dir]);
  }
  else {
    abort();
  }
  switch(op) {
  case PLUS:
    ans1 = num1 + num2;
    break;
  case MINUS:
    ans1 = num1 - num2;
    break;
  case DIVIDE:
    ans1 = num1 / num2;
    break;
  case MULTIPLY:
    ans1 = num1 * num2;
    break;
  default:
    abort();
  }
  x = fpgetsticky();
  fpsetprecision(pc);
  print(num1, snum1);
  print(num2, snum2);
  print(ans1, sans1);
  if(fp == NULL) {
    printf("%s %s %s %s = %s [ %s ]",
	   snum1, operators[op], rnddir[dir], snum2, sans1,
	   fpcls[fpclass(ans1)]);
    if(x & FP_X_INV) printf(" INV");
    if(x & FP_X_DZ) printf(" DZ");
    if(x & FP_X_OFL) printf(" OFL");
    if(x & FP_X_UFL) printf(" UFL");
    if(x & FP_X_IMP) printf(" IMP");
#ifdef FP_X_DNML
    if(x & FP_X_DNML) printf(" DNML");
#endif
    printf(" end\n");
  }
  else {
    char fp_op[MAXOPLEN], fp_rnd[MAXRNDLEN];
    char fp_num1[(DBL2CHR * 2) + 1], fp_num2[(DBL2CHR * 2) + 1],
      fp_ans1[(DBL2CHR * 2) + 1];
    char fp_cls[MAXFPCLS];
    char xbuf[MAXXLEN];
    int inv = 0, dz = 0, ofl = 0, ufl = 0, imp = 0, dnml = 0;
    int thisopdiff = 0, thisansdiff = 0, thisxdiff = 0, thisclsdiff = 0;
    fpclass_t thiscls;

    if(fscanf(fp, "%s %s %s %s = %s [ %s ]",
	      fp_num1, fp_op, fp_rnd, fp_num2, fp_ans1, fp_cls) != 6) {
      fprintf(stderr, "Problem reading comparison file\n");
      abort();
    }
    
    if(strcmp(operators[op], fp_op) != 0) {
      fprintf(stderr, "Operator mismatch in comparison file "
	      "(expecting %s found %s)\n", operators[op], fp_op);
      abort();
    }
    if(strcmp(rnddir[dir], fp_rnd) != 0) {
      fprintf(stderr, "Rounding direction mismatch in comparison file "
	      "(expecting %s found %s)\n", rnddir[dir], fp_rnd);
      abort();
    }

    if(strcmp(fp_num1, snum1) != 0) {
      thisopdiff = 1;
    }
    if(strcmp(fp_num2, snum2) != 0) {
      thisopdiff = 1;
    }
    if(strcmp(fp_ans1, sans1) != 0) {
      thisansdiff = 1;
    }
    thiscls = fpclass(ans1);
    if(strcmp(fp_cls, fpcls[thiscls]) != 0) {
      thisclsdiff = 1;
    }
    do {
      if(fscanf(fp, "%s", xbuf) != 1) {
	fprintf(stderr, "Problem reading comparison file");
	abort();
      }
      if(strcmp(xbuf, "INV") == 0) inv = FP_X_INV;
      else if(strcmp(xbuf, "DZ") == 0) dz = FP_X_DZ;
      else if(strcmp(xbuf, "UFL") == 0) ufl = FP_X_UFL;
      else if(strcmp(xbuf, "OFL") == 0) ofl = FP_X_OFL;
      else if(strcmp(xbuf, "IMP") == 0) imp = FP_X_IMP;
#ifdef FP_X_DNML
      else if(strcmp(xbuf, "DNML") == 0) dnml = FP_X_DNML;
#else
      else if(strcmp(xbuf, "DNML") == 0) dnml = -1;
#endif
      else if(strcmp(xbuf, "end") != 0) {
	fprintf(stderr, "Problem reading comparison file");
	abort();
      }
    } while(strcmp(xbuf, "end") != 0);

    if(inv != (x & FP_X_INV)) thisxdiff = 1;
    if(dz != (x & FP_X_DZ)) thisxdiff = 1;
    if(ufl != (x & FP_X_UFL)) thisxdiff = 1;
    if(ofl != (x & FP_X_OFL)) thisxdiff = 1;
    if(imp != (x & FP_X_IMP)) thisxdiff = 1;
#ifdef FP_X_DNML
    if(dnml != (x & FP_X_DNML)) thisxdiff = 1;
#else
    if(dnml == -1) thisxdiff = 1;
#endif
    
    if(thisansdiff || thisxdiff || thisclsdiff) {
      printf("In calculation %g %s %g = %g (rounding %s):\n",
	     num1, operators[op], num2, ans1, rnddir[dir]);
      if(thisansdiff && thisopdiff) {
	printf("\tOperators differ leading to different answer:\n");
	printf("\t\tFILE: op1=%s op2=%s ans=%s (%g)\n"
	       "\t\tHERE: op1=%s op2=%s ans=%s\n",
	       fp_num1, fp_num2, fp_ans1, input(fp_ans1), snum1, snum2, sans1);
      }
      else if(thisansdiff) {
	printf("\tDifferent answer:\n\t\tFILE: ans=%s (%g)\n"
	       "\t\tHERE: ans=%s\n",
	       fp_ans1, input(fp_ans1), sans1);
      }
      else if(thisclsdiff) {
	printf("\tDifferent class:\n"
	       "\t\tFILE: %s\n\t\tHERE: %s\n",
	       fp_cls, fpcls[thiscls]);
      }
      if(thisxdiff) {
	printf("\tDifferent exceptions raised:\n"
	       "\t\tFILE: %s %s %s %s %s %s\n"
	       "\t\tHERE: %s %s %s %s %s %s\n",
	       inv ? "I" : "-", dz ? "Z" : "-", ufl ? "U" : "-",
	       ofl ? "O" : "-", imp ? "P" : "-", dnml ? "D" : "-",
	       (x & FP_X_INV) ? "I" : "-", (x & FP_X_DZ) ? "Z" : "-",
	       (x & FP_X_UFL) ? "U" : "-", (x & FP_X_OFL) ? "O" : "-",
	       (x & FP_X_IMP) ? "P" : "-",
#ifdef FP_X_DNML
	       (x & FP_X_DNML) ? "D" : "-"
#else
	       "-"
#endif
	       );
      }
    }
    if(thisopdiff) (*nopdiffs)++;
    else if(thisansdiff) (*nansdiffs)++;
    else {
      if(thisxdiff) (*nxdiffs)++;
      if(thisclsdiff) (*nclsdiffs)++;
    }
  }
}

/* test_settings
 *
 * Check whether settings are remembered on the chip for rounding direction,
 * exception sticky bits and mask, and if applicable precision control.
 */

int test_settings(void) {
  int failures = 0;
  fp_except mask_orig;

  printf("Testing settings on the chip... ");
  fflush(stdout);

  fpsetround(FP_RM);
  if(fpgetround() != FP_RM) FAIL_TEST;
  fpsetround(FP_RP);
  if(fpgetround() != FP_RP) FAIL_TEST;
  fpsetround(FP_RZ);
  if(fpgetround() != FP_RZ) FAIL_TEST;
  fpsetround(FP_RN);
  if(fpgetround() != FP_RN) FAIL_TEST;
 
  fpsetsticky(FP_X_INV | FP_X_DZ | FP_X_IMP);
  if(fpgetsticky() != (FP_X_INV | FP_X_DZ | FP_X_IMP)) FAIL_TEST;
  fpsetsticky(FP_X_OFL | FP_X_IMP);
  if(fpgetsticky() != (FP_X_OFL | FP_X_IMP)) FAIL_TEST;
  fpsetsticky(FP_X_UFL | FP_X_DZ);
  if(fpgetsticky() != (FP_X_UFL | FP_X_DZ)) FAIL_TEST;
  fpsetsticky(0);
  if(fpgetsticky() != 0) FAIL_TEST;

  mask_orig = fpsetmask(FP_X_INV | FP_X_DZ);
  if(fpgetmask() != (FP_X_INV | FP_X_DZ)) FAIL_TEST;
  fpsetmask(FP_X_OFL | FP_X_IMP);
  if(fpgetmask() != (FP_X_OFL | FP_X_IMP)) FAIL_TEST;
  fpsetmask(FP_X_UFL | FP_X_DZ | FP_X_INV);
  if(fpgetmask() != (FP_X_UFL | FP_X_DZ | FP_X_INV)) FAIL_TEST;
  fpsetmask(mask_orig);
  if(fpgetmask() != mask_orig) FAIL_TEST;

#ifdef __CYGWIN__
  fpsetprecision(FP_PC_SGL);
  if(fpgetprecision() != FP_PC_SGL) FAIL_TEST;
  fpsetprecision(FP_PC_DBL);
  if(fpgetprecision() != FP_PC_DBL) FAIL_TEST;
  fpsetprecision(FP_PC_EXT);
  if(fpgetprecision() != FP_PC_EXT) FAIL_TEST;
#endif

  if(failures == 0) {
    printf(" PASSED\n");
    return 0;
  }
  else {
    printf(" %d failures\n", failures);
    return 1;
  }
}

/* test_rounding
 *
 * Here we test that the rounding direction works correctly. This is based
 * on section 7.3 of the IEEE 754 standard, which specifies what happens
 * on overflow. This function assumes the correct working of fpclass.
 */

int test_rounding(void) {
  int failures = 0;
  double prp, prn, prz, prm;
  double nrp, nrn, nrz, nrm;

  printf("Testing rounding direction... ");
  fflush(stdout);

#ifdef __CYGWIN__
  fpsetprecision(FP_PC_DBL);
#endif

  fpsetround(FP_RP);

  prp = (ten / three) * three;
  nrp = (negten / three) * three;

  fpsetround(FP_RM);

  prm = (ten / three) * three;
  nrm = (negten / three) * three;

  fpsetround(FP_RZ);

  prz = (ten / three) * three;
  nrz = (negten / three) * three;

  fpsetround(FP_RN);

  prn = (ten / three) * three;
  nrn = (negten / three) * three;

  /* Positive answers should be ordered RZ == RM < RN < RP */

  if(prp <= prn) FAIL_TEST;
  if(prn <= prm) FAIL_TEST;
  if(prz != prm) FAIL_TEST;

  /* Negative answers should be ordered RM < RN < RP == RZ */

  if(nrm >= nrn) FAIL_TEST;
  if(nrn >= nrp) FAIL_TEST;
  if(nrz != nrp) FAIL_TEST;

  /* IEEE 754, Section 7.3 check. Of course Intel doesn't comply...

  fpsetround(FP_RP);
  if(fpclass(DBL_MAX * two) != FP_PINF) FAIL_TEST;
  if(-DBL_MAX * two != -DBL_MAX) FAIL_TEST;

  fpsetround(FP_RM);
  if(DBL_MAX * two != DBL_MAX) FAIL_TEST;
  if(fpclass(-DBL_MAX * two) != FP_NINF) FAIL_TEST;

  fpsetround(FP_RZ);
  if(DBL_MAX * two != DBL_MAX) FAIL_TEST;
  if(-DBL_MAX * two != -DBL_MAX) FAIL_TEST;

  fpsetround(FP_RN);
  if(fpclass(DBL_MAX * two) != FP_PINF) FAIL_TEST;
  if(fpclass(-DBL_MAX * two) != FP_NINF) FAIL_TEST;

  */

#ifdef __CYGWIN__
  fpsetprecision(FP_PC_EXT);
#endif

  if(failures == 0) {
    printf(" PASSED\n");
    return 0;
  }
  else {
    printf(" %d failures\n", failures);
    return 1;
  }
}

/* test_sticky
 *
 * Here we check that the sticky bits are set when they should be. This relies
 * on the chip being IEEE 754 compliant.
 */

int test_sticky(void) {
  double ans;
  int failures = 0;

  printf("Testing sticky bits... ");
  fflush(stdout);

  fpsetsticky(0);
  ans = two / three;
  if((fpgetsticky() & FP_X_IMP) != FP_X_IMP) FAIL_TEST;

  fpsetsticky(0);
  ans = DBL_MIN / three;
  if((fpgetsticky() & FP_X_UFL) != FP_X_UFL) FAIL_TEST;
#ifdef __CYGWIN__
  ans = ans / three;
  if((fpgetsticky() & FP_X_DNML) != FP_X_DNML) FAIL_TEST;
#endif

  fpsetsticky(0);
  ans = DBL_MAX * three;
  if((fpgetsticky() & FP_X_OFL) != FP_X_OFL) FAIL_TEST;

  fpsetsticky(0);
  ans = one / zero;
  if((fpgetsticky() & FP_X_DZ) != FP_X_DZ) FAIL_TEST;

  fpsetsticky(0);
  ans = sqrt(negone);
  if((fpgetsticky() & FP_X_INV) != FP_X_INV) FAIL_TEST;

  fpsetsticky(0);
 
  if(failures == 0) {
    printf(" PASSED\n");
    return 0;
  }
  else {
    printf(" %d failures\n", failures);
    return 1;
  }
}

/* test_precision
 *
 * In double precision calculations, the result of 2^53 + (1 + 2^-12) should
 * be rounded up to 2^53 + 2, whilst in double extended precision calculations
 * the result should be rounded down to 2^53.
 */

int test_precision(void) {
#ifdef __CYGWIN__
  double ans;
  int failures = 0;

  printf("Testing precision... ");
  fflush(stdout);

  fpsetprecision(FP_PC_DBL);
  ans = two53 + (one + two_12);
  if(ans != two53ptwo) FAIL_TEST;

  fpsetprecision(FP_PC_EXT);
  ans = two53 + (one + two_12);
  if(ans != two53) FAIL_TEST;

  if(failures == 0) {
    printf(" PASSED\n");
    return 0;
  }
  else {
    printf(" %d failures\n", failures);
    return 1;
  }
#else
  return 0;
#endif
}

/* test_class
 *
 * Test that various classes of number are recognised properly. As at version
 * 2.0, denormalised numbers are not properly recognised. The test will
 * therefore not check this. It is also not possible to check for a signalling
 * NaN because as soon as its value is checked it is changed to a quiet NaN.
 */

int test_class(void) {
  int failures = 0;

  printf("Testing floating point classes... ");
  fflush(stdout);

  if(fpclass(one) != FP_PNORM) FAIL_TEST;
  if(fpclass(negone) != FP_NNORM) FAIL_TEST;
  if(fpclass(zero) != FP_PZERO) FAIL_TEST;
  if(fpclass(negzero) != FP_NZERO) FAIL_TEST;
#if (defined(DENORM_BUG_SOLVED) && defined(__CYGWIN__)) || defined(__sun__)
  if(fpclass(DBL_MIN / three) != FP_PDENORM) FAIL_TEST;
  if(fpclass(-DBL_MIN / three) != FP_NDENORM) FAIL_TEST;
#endif
  if(fpclass(DBL_MAX * two) != FP_PINF) FAIL_TEST;
  if(fpclass(DBL_MAX * negtwo) != FP_NINF) FAIL_TEST;
  if(fpclass(sqrt(negone)) != FP_QNAN) FAIL_TEST;

  if(failures == 0) {
    printf(" PASSED\n");
    return 0;
  }
  else {
    printf(" %d failures\n", failures);
    return 1;
  }
}
  

/* test_mask
 *
 * A thorough check of the exception masks will not be made in version 2.0.
 */

int test_mask(void) {
  return 0;
}

/* test_functions
 *
 * Go through each of the functions implemented and check that they actually
 * work. There are a number of checks to make:
 *
 * 1. Do fpget/setround fpget/setsticky fpget/setmask and fpget/setprecision
 *    remember how they have been set?
 *
 * 2. Does fpsetround work? Do imprecise calculations generate expected
 *    results?
 *
 * 3. Does fpgetsticky work? Are calculations that are supposed to generate
 *    exceptions detectable?
 *
 * 4. Does fpsetprecision work? Are calculations that should work differently
 *    under different precisions working as expected?
 *
 * 5. Does fpclass work? Is the correct class returned for a variety of 
 *    classes of floating point number?
 *
 * 6. Does fpgetmask work? If a mask bit is set, is an exception generated?
 *    (this probably won't be done).
 */

int test_functions(void) {
  int retval = 0;

  retval |= test_settings();
  retval |= test_rounding();
  retval |= test_sticky();
  retval |= test_precision();
  retval |= test_class();
  retval |= test_mask();

  return retval;
}

/* main(argc, argv)
 *
 * A list of numbers is given as arguments. Each number is then
 * operated on each number using each operator in each rounding
 * direction.
 */

int main(int argc, char **argv) {
  double *numbers;
  int i, j, n, r, o;
  int compare_mode = 0;
  int nopdiffs = 0, nansdiffs = 0, nxdiffs = 0, nclsdiffs = 0;
  int *numansdiffs, opansdiffs[4] = {0, 0, 0, 0};
  int rndansdiffs[4] = {0, 0, 0, 0};
  int *numxdiffs, opxdiffs[4] = {0, 0, 0, 0}, rndxdiffs[4] = {0, 0, 0, 0};
  int *numt, opt[4] = {0, 0, 0, 0}, rndt[4] = {0, 0, 0, 0};
  int oprndansdiffs[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int oprndxdiffs[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int oprndt[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  char *compare_file;
  FILE *fp = NULL;

  if(argc == 1) {
    fprintf(stderr, "Usage: %s [-test | -cmp <file> | <list of floats....>]\n",
	    argv[0]);
    exit(1);
  }
  
  if(argc >= 2) {
    if(strcmp(argv[1], "-cmp") == 0) {
      if(argc < 3) {
	fprintf(stderr, "You must supply a file to compare with\n");
	exit(1);
      }
      else if(argc > 3) {
	fprintf(stderr, "WARNING: ignoring arguments after %s\n", argv[3]);
      }
      compare_mode = 1;
      compare_file = argv[2];
      fp = fopen(compare_file, "r");
      if(fp == NULL) {
	fprintf(stderr, "Error opening comparison file ");
	perror(compare_file);
	abort();
      }
    }
    else if(strcmp(argv[1], "-test") == 0) {
      return test_functions();
    }
  }

  if(compare_mode) {
    if(fscanf(fp, "Numbers: %d", &n) != 1) {
      printf("Problem reading comparison file %s\n", compare_file);
      abort();
    }
  }
  else {
    n = argc - 1;
    printf("Numbers: %d\n", n);
  }
  numbers = malloc(n * sizeof(double));
  if(numbers == NULL) {
    perror("Memory allocation");
    abort();
  }
  numansdiffs = malloc(n * sizeof(int));
  if(numansdiffs == NULL) {
    perror("Memory allocation");
    abort();
  }
  numxdiffs = malloc(n * sizeof(int));
  if(numxdiffs == NULL) {
    perror("Memory allocation");
    abort();
  }
  numt = malloc(n * sizeof(int));
  if(numt == NULL) {
    perror("Memory allocation");
    abort();
  }
  
  for(i = 0; i < n; i++) {
    if(compare_mode) {
      if(fscanf(fp, "%lf", &numbers[i]) != 1) {
	printf("Problem reading comparison file %s\n", compare_file);
	abort();
      }
    }
    else {
      numbers[i] = atof(argv[i + 1]);
      printf("%s\n", argv[i + 1]);
    }
    numansdiffs[i] = 0;
    numxdiffs[i] = 0;
    numt[i] = 0;
  }

  for(i = 0; i < n; i++) {
    for(j = 0; j < n; j++) {
      for(o = 0; o <= 3; o++) {
	for(r = 0; r <= 3; r++) {
	  int pansdiffs = nansdiffs;
	  int pxdiffs = nxdiffs;

	  op(numbers[i], numbers[j], o, r, fp,
	     &nopdiffs, &nansdiffs, &nxdiffs, &nclsdiffs);

	  if(pansdiffs != nansdiffs) {
	    numansdiffs[i]++;
	    numansdiffs[j]++;
	    opansdiffs[o]++;
	    rndansdiffs[r]++;
	    oprndansdiffs[(o * 4) + r]++;
	  }
	  if(pxdiffs != nxdiffs) {
	    numxdiffs[i]++;
	    numxdiffs[j]++;
	    opxdiffs[o]++;
	    rndxdiffs[r]++;
	    oprndxdiffs[(o * 4) + r]++;
	  }
	  numt[i]++;
	  numt[j]++;
	  opt[o]++;
	  rndt[r]++;
	  oprndt[(o * 4) + r]++;
	}
      }
    }
  }

  if(compare_mode) {
    fclose(fp);
    printf("Summary of differences between comparison file and this run:\n");
    printf("\t(Each is a count of the number of calculations concerned)\n");
    printf("\t%d\t-- different representation of one or more operands\n",
	   nopdiffs);
    printf("\t%d\t-- different answer where operands represented the same\n",
	   nansdiffs);
    printf("\t%d\t-- different exceptions where answer the same\n",
	   nxdiffs);
    printf("\t%d\t-- different fpclasses where answer the same\n",
	   nclsdiffs);
    printf("Breakdown of differences by number, operator and rounding "
	   "direction:\n");
    printf("\t% 8s % 15s % 6s % 9s % 5s\n", "Type", "Data", "Answer",
	   "Exception", "Total");
    for(i = 0; i < n; i++) {
      printf("\t% 8s % 15g % 6d % 9d % 5d\n", "Number", numbers[i],
	     numansdiffs[i], numxdiffs[i], numt[i]);
    }
    for(i = 0; i < 4; i++) {
      printf("\t% 8s % 15s % 6d % 9d % 5d\n", "Operator", operators[i],
	     opansdiffs[i], opxdiffs[i], opt[i]);
    }
    for(i = 0; i < 4; i++) {
      printf("\t% 8s % 15s % 6d % 9d % 5d\n", "Round", rnddir[i],
	     rndansdiffs[i], rndxdiffs[i], rndt[i]);
    }
    for(o = 0; o < 4; o++) {
      for(r = 0; r < 4; r++) {
	printf("\t% 8s % 14s%s % 6d % 9d % 5d\n", "Op&Round", operators[o],
	       rnddir[r], oprndansdiffs[(o * 4) + r], oprndxdiffs[(o * 4) + r],
	       oprndt[(o * 4) + r]);
      }
    }
  }

  free(numbers);
  free(numansdiffs);
  free(numxdiffs);
  free(numt);

  return 0;
}

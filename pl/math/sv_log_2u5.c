/*
 * Double-precision SVE log(x) function.
 *
 * Copyright (c) 2020-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#define P(i) sv_f64 (__v_log_data.poly[i])
#define N (1 << V_LOG_TABLE_BITS)
#define Off (0x3fe6900900000000)
#define MaxTop (0x7ff)
#define MinTop (0x001)
#define ThreshTop (0x7fe) /* MaxTop - MinTop.  */

static svfloat64_t NOINLINE
special_case (svfloat64_t x, svfloat64_t y, svbool_t cmp)
{
  return sv_call_f64 (log, x, y, cmp);
}

/* SVE port of AdvSIMD log algorithm.
   Maximum measured error is 2.17 ulp:
   SV_NAME_D1 (log)(0x1.a6129884398a3p+0) got 0x1.ffffff1cca043p-2
					 want 0x1.ffffff1cca045p-2.  */
svfloat64_t SV_NAME_D1 (log) (svfloat64_t x, const svbool_t pg)
{
  svuint64_t ix = svreinterpret_u64_f64 (x);
  svuint64_t top = svlsr_n_u64_x (pg, ix, 52);
  svbool_t cmp
      = svcmpge_u64 (pg, svsub_n_u64_x (pg, top, MinTop), sv_u64 (ThreshTop));

  /* x = 2^k z; where z is in range [Off,2*Off) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  svuint64_t tmp = svsub_n_u64_x (pg, ix, Off);
  /* Equivalent to (tmp >> (52 - V_LOG_TABLE_BITS)) % N, since N is a power
     of 2.  */
  svuint64_t i = svand_n_u64_x (
      pg, svlsr_n_u64_x (pg, tmp, (52 - V_LOG_TABLE_BITS)), N - 1);
  svint64_t k = svasr_n_s64_x (pg, svreinterpret_s64_u64 (tmp),
			       52); /* Arithmetic shift.  */
  svuint64_t iz
      = svsub_u64_x (pg, ix, svand_n_u64_x (pg, tmp, 0xfffULL << 52));
  svfloat64_t z = svreinterpret_f64_u64 (iz);
  /* Lookup in 2 global lists (length N).  */
  svfloat64_t invc = svld1_gather_u64index_f64 (pg, __v_log_data.invc, i);
  svfloat64_t logc = svld1_gather_u64index_f64 (pg, __v_log_data.logc, i);

  /* log(x) = log1p(z/c-1) + log(c) + k*Ln2.  */
  svfloat64_t r = svmad_n_f64_x (pg, invc, z, -1);
  svfloat64_t kd = svcvt_f64_s64_x (pg, k);
  /* hi = r + log(c) + k*Ln2.  */
  svfloat64_t hi
      = svmla_n_f64_x (pg, svadd_f64_x (pg, logc, r), kd, __v_log_data.ln2);
  /* y = r2*(A0 + r*A1 + r2*(A2 + r*A3 + r2*A4)) + hi.  */
  svfloat64_t r2 = svmul_f64_x (pg, r, r);
  svfloat64_t y = svmla_f64_x (pg, P (2), r, P (3));
  svfloat64_t p = svmla_f64_x (pg, P (0), r, P (1));
  y = svmla_f64_x (pg, y, r2, P (4));
  y = svmla_f64_x (pg, p, r2, y);
  y = svmla_f64_x (pg, hi, r2, y);

  if (unlikely (svptest_any (pg, cmp)))
    return special_case (x, y, cmp);
  return y;
}

PL_SIG (SV, D, 1, log, 0.01, 11.1)
PL_TEST_ULP (SV_NAME_D1 (log), 1.68)
PL_TEST_INTERVAL (SV_NAME_D1 (log), -0.0, -inf, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 0, 0x1p-149, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 0x1p-149, 0x1p-126, 4000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 0x1p-23, 1.0, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 1.0, 100, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 100, inf, 50000)

/*
 * Double-precision SVE log10(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#define Min 0x0010000000000000
#define Max 0x7ff0000000000000
#define Thres 0x7fe0000000000000 /* Max - Min.  */
#define Off 0x3fe6900900000000
#define N (1 << V_LOG10_TABLE_BITS)

#define A(i) __v_log10_data.poly[i]
#define T(s, i) __v_log10_data.s[i]

static svfloat64_t NOINLINE
special_case (svfloat64_t x, svfloat64_t y, svbool_t special)
{
  return sv_call_f64 (log10, x, y, special);
}

/* SVE log10 algorithm.
   Maximum measured error is 2.46 ulps.
   SV_NAME_D1 (log10)(0x1.131956cd4b627p+0) got 0x1.fffbdf6eaa669p-6
					   want 0x1.fffbdf6eaa667p-6.  */
svfloat64_t SV_NAME_D1 (log10) (svfloat64_t x, const svbool_t pg)
{
  svuint64_t ix = svreinterpret_u64_f64 (x);
  svbool_t special = svcmpge_n_u64 (pg, svsub_n_u64_x (pg, ix, Min), Thres);

  /* x = 2^k z; where z is in range [Off,2*Off) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  svuint64_t tmp = svsub_n_u64_x (pg, ix, Off);
  svuint64_t i = sv_mod_n_u64_x (
      pg, svlsr_n_u64_x (pg, tmp, 52 - V_LOG10_TABLE_BITS), N);
  svfloat64_t k = svcvt_f64_s64_x (
      pg, svasr_n_s64_x (pg, svreinterpret_s64_u64 (tmp), 52));
  svfloat64_t z = svreinterpret_f64_u64 (
    svsub_u64_x (pg, ix, svand_n_u64_x (pg, tmp, 0xfffULL << 52)));

  /* log(x) = k*log(2) + log(c) + log(z/c).  */
  svfloat64_t invc = svld1_gather_u64index_f64 (pg, &T (invc, 0), i);
  svfloat64_t logc = svld1_gather_u64index_f64 (pg, &T (log10c, 0), i);

  /* We approximate log(z/c) with a polynomial P(x) ~= log(x + 1):
     r = z/c - 1 (we look up precomputed 1/c)
     log(z/c) ~= P(r).  */
  svfloat64_t r = svmla_f64_x (pg, sv_f64 (-1.0), invc, z);

  /* hi = log(c) + k*log(2).  */
  svfloat64_t w = svmla_n_f64_x (pg, logc, r, __v_log10_data.invln10);
  svfloat64_t hi = svmla_n_f64_x (pg, w, k, __v_log10_data.log10_2);

  /* y = r2*(A0 + r*A1 + r2*(A2 + r*A3 + r2*A4)) + hi.  */
  svfloat64_t r2 = svmul_f64_x (pg, r, r);
  svfloat64_t y = svmla_n_f64_x (pg, sv_f64 (A (2)), r, A (3));
  svfloat64_t p = svmla_n_f64_x (pg, sv_f64 (A (0)), r, A (1));
  y = svmla_n_f64_x (pg, y, r2, A (4));
  y = svmla_f64_x (pg, p, r2, y);
  y = svmla_f64_x (pg, hi, r2, y);

  if (unlikely (svptest_any (pg, special)))
    return special_case (x, y, special);
  return y;
}

PL_SIG (SV, D, 1, log10, 0.01, 11.1)
PL_TEST_ULP (SV_NAME_D1 (log10), 1.97)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), -0.0, -0x1p126, 100)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 0x1p-149, 0x1p-126, 4000)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 0x1p-23, 1.0, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 1.0, 100, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 100, inf, 50000)

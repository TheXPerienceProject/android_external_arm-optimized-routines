/*
 * Single-precision cos function.
 *
 * Copyright (c) 2018-2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <math.h>
#include "math_config.h"
#include "sincosf.h"

/* Fast cosf implementation.  Worst-case ULP is 0.5607, maximum relative
   error is 0.5303 * 2^-23.  A single-step range reduction is used for
   small values.  Large inputs have their range reduced using fast integer
   arithmetic.  */
float
cosf (float y)
{
  double x = y;
  double s;
  int n;
  const sincos_t *p = &__sincosf_table[0];

  if (abstop12 (y) < abstop12 (pio4f))
    {
      double x2 = x * x;

      if (unlikely (abstop12 (y) < abstop12 (0x1p-12f)))
	return 1.0f;

      return sinf_poly (x, x2, p, 1);
    }
  else if (likely (abstop12 (y) < abstop12 (120.0f)))
    {
      x = reduce_fast (x, p, &n);

      /* Setup the signs for sin and cos.  */
      s = p->sign[n & 3];

      if (n & 2)
	p = &__sincosf_table[1];

      return sinf_poly (x * s, x * x, p, n ^ 1);
    }
  else if (abstop12 (y) < abstop12 (INFINITY))
    {
      uint32_t xi = asuint (y);
      int sign = xi >> 31;

      x = reduce_large (xi, &n);

      /* Setup signs for sin and cos - include original sign.  */
      s = p->sign[(n + sign) & 3];

      if ((n + sign) & 2)
	p = &__sincosf_table[1];

      return sinf_poly (x * s, x * x, p, n ^ 1);
    }
  else
    return __math_invalidf (y);
}

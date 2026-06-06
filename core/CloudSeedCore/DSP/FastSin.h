#pragma once

#include <cstdint>
#include <arm_math.h>

#define FAST_MATH_TABLE_SIZE 512

namespace Cloudseed
{
	class FastSin
	{
	public:
		static const float32_t sinTable_f32[FAST_MATH_TABLE_SIZE + 1];
		static inline float32_t Get(float32_t in)
		{
			float32_t sinVal, fract;                   /* Temporary input, output variables */
			uint16_t index;                            /* Index variable */
			float32_t a, b;                            /* Two nearest output values */
			int32_t n;
			float32_t findex;

			/* Calculation of floor value of input */
			n = (int32_t) in;

			/* Make negative values towards -infinity */
			if (in < 0.0f)
			{
				n--;
			}

			/* Map input value to [0 1] */
			in = in - (float32_t) n;

			/* Calculation of index of the table */
			findex = (float32_t)FAST_MATH_TABLE_SIZE * in;
			index = (uint16_t)findex;

			/* when "in" is exactly 1, we need to rotate the index down to 0 */
			if (index >= FAST_MATH_TABLE_SIZE) {
				index = 0;
				findex -= (float32_t)FAST_MATH_TABLE_SIZE;
			}

			/* fractional value calculation */
			fract = findex - (float32_t) index;

			/* Read two nearest values of input value from the sin table */
			a = sinTable_f32[index];
			b = sinTable_f32[index+1];

			/* Linear interpolation process */
			sinVal = (1.0f - fract) * a + fract * b;

			/* Return output value */
			return sinVal;
		}
	};
}

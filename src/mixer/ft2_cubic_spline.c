#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "ft2_cubic_spline.h"
#include "../ft2_video.h" // showErrorMsgBox()

float *fCubicSplineLUT = NULL; // globalized

bool calcCubicSplineTable(void)
{
	fCubicSplineLUT = (float *)malloc(CUBIC_SPLINE_TAPS*CUBIC_SPLINE_PHASES * sizeof (float));
	if (fCubicSplineLUT == NULL)
	{
		showErrorMsgBox("Not enough memory!");
		return false;
	}

	float *fPtr = fCubicSplineLUT;
	for (int32_t i = 0; i < CUBIC_SPLINE_PHASES; i++)
	{
		const double x  = i * (1.0 / CUBIC_SPLINE_PHASES); // x = i / CUBIC_SPLINE_PHASES
		const double x2 = x * x;  // x^2
		const double x3 = x2 * x; // x^3

		*fPtr++ = (float)(-0.5 * x3 + 1.0 * x2 - 0.5 * x);
		*fPtr++ = (float)( 1.5 * x3 - 2.5 * x2 + 1.0);
		*fPtr++ = (float)(-1.5 * x3 + 2.0 * x2 + 0.5 * x);
		*fPtr++ = (float)( 0.5 * x3 - 0.5 * x2);
	}

	return true;
}

void freeCubicSplineTable(void)
{
	if (fCubicSplineLUT != NULL)
	{
		free(fCubicSplineLUT);
		fCubicSplineLUT = NULL;
	}
}

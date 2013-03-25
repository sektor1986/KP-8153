#include "geometry.h"

float ValueLine(float x, float x_1, float x_2, float y_1, float y_2)
{
	float result = 0.0;
	
	result = ((x-x_1)*(y_2-y_1)/(x_2-x_1))+y_1;
	
	return result;
}


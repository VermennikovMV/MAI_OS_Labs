#include <cmath>
#include "derivative.h"

extern "C" float Derivative(float A, float deltaX) {
    return (cosf(A + deltaX) - cosf(A)) / deltaX;
}
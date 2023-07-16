#pragma once

#include "ArrayAlgebra.h"
#include "Integration.h"

#include "DomainInterpolation.h"
#include "Interpolation.h"

constexpr int pos_modulo(int n, int d) { return (n % d + d) % d; }

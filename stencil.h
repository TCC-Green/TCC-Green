#ifndef STENCIL_H
#define STENCIL_H

#include <omp.h>
#include <stdio.h>
#include <cstdlib>

void stencil(double** arr, double** arr_new, int m, int n, int iterations);

#endif

#ifndef REDUCTION_H
#define REDUCTION_H

#include <omp.h>
#include <stdio.h>

long long reduce(double** arr, int m, int n, int iterations);

#endif

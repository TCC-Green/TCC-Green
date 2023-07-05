#include "stencil.h"

using namespace std;

double stencilTransform(double** originalArray, int originalArrayM, int originalArrayN, int x, int y)
{
    return (originalArray[x-1][y] + originalArray[x+1][y] + originalArray[x][y-1] + originalArray[x][y+1]) / 4;
}

void stencil(double** arr, double** arr_new, int m, int n, int iterations)
{
    #pragma omp parallel shared(arr,arr_new) firstprivate(m,n,iterations)
    {
        for (int it = 0; it < iterations; ++it)
        {
            #pragma omp for
            for (int x = 1; x < m - 1; ++x)
                for (int y = 1; y < n - 1; ++y)
                    arr_new[x][y] = stencilTransform(arr, m, n, x, y);

            #pragma omp single
            {
                double** temp = arr;
                arr = arr_new;
                arr_new = temp;
            }
        }
    }
}

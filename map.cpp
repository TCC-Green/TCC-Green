#include "map.h"

using namespace std;

double mapTransform(double value)
{
    return value * 7 + 13;
}

void map(double** arr, int m, int n, int iterations)
{
    #pragma omp parallel shared(arr) firstprivate(m,n,iterations)
    {
        for (int i = 0; i < iterations; ++i)
            #pragma omp for
            for (int x = 0; x < m; ++x)
                for (int y = 0; y < n; ++y)
                    arr[x][y] = mapTransform(arr[x][y]);
    }
}

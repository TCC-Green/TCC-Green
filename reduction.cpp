#include "reduction.h"

using namespace std;

long long reduce(double** arr, int m, int n, int iterations)
{
    long long sum = 0;
    #pragma omp parallel shared(arr) firstprivate(m,n,iterations)
    {
        for (int i = 0; i < iterations; ++i)
            #pragma omp for reduction(+:sum)
            for (int x = 0; x < m; ++x)
                for (int y = 0; y < n; ++y)
                    sum += arr[x][y];
    }
    return sum;
}

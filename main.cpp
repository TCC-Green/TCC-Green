// RAPL
// https://github.com/greensoftwarelab/Energy-Languages

#include "reduction.h"
#include "map.h"
#include "stencil.h"
#include "utils.h"
#include <cstdlib>
#include <vector>
#include <string>
#ifdef __linux__ 
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#endif

using namespace std;

int AvailableThreads, MaxThreads;

void wait(int seconds) {
    #ifdef __linux__ 
    sleep(1);
    #elif _WIN32
    Sleep(1);
    #endif
}

void measureIdle(vector<Rapl*> packages) {
    cout << "Measuring idle (1s)" << endl;
    resetPackages(packages);
    wait(1);
    samplePackages(packages);
    printPackages(packages);
    cout << endl;
}

void testReduction(vector<Rapl*> packages, int arrSize, int iter) {
    double clock_start, clock_end;
    // initialize
    omp_set_num_threads(AvailableThreads);
    const int reduceM = arrSize, reduceN = arrSize, iterations = iter; // 60kx60kx ~ 25GB of RAM | 40kx40k ~ 14GB
    cout << "Initialize Reduction (" << reduceM << "x" << reduceN << " array)" << endl;
    double** reduceArray = new double*[reduceM];
    #pragma omp parallel shared(reduceArray) firstprivate(reduceM,reduceN)
    {
        #pragma omp for
        for (int i = 0; i < reduceM; ++i)
            reduceArray[i] = new double[reduceN];
        #pragma omp for collapse(2)
        for (int x = 0; x < reduceM; ++x) // populate
            for (int y = 0; y < reduceN; ++y)
                reduceArray[x][y] = 1; // rand();
    }
    // execute
    omp_set_num_threads(MaxThreads);
    wait(1); // wait a bit
    cout << "Execute Reduction" << endl;
    resetPackages(packages);
    clock_start = omp_get_wtime();
    reduce(reduceArray, reduceM, reduceN, iterations);
    clock_end = omp_get_wtime();
    samplePackages(packages);
    printExecutionTime(clock_start, clock_end);
    printPackages(packages);
    // clean
    cout << "Clean Reduction" << endl;
    for (int i = 0; i < reduceM; ++i)
        delete reduceArray[i];
    delete []reduceArray;
    cout << endl;
}

void testMap(vector<Rapl*> packages, int arrSize, int iter) {
    double clock_start, clock_end;
    // initialize
    omp_set_num_threads(AvailableThreads);
    const int mapM = arrSize, mapN = arrSize, iterations = iter;
    cout << "Initialize Map (" << mapM << "x" << mapN << " array)" << endl;
    double** mapArray = new double*[mapM];
    #pragma omp parallel shared(mapArray) firstprivate(mapM,mapN)
    {
        #pragma omp for
        for (int i = 0; i < mapM; ++i)
            mapArray[i] = new double[mapN];
        #pragma omp for collapse(2)
        for (int x = 0; x < mapM; ++x) // populate
            for (int y = 0; y < mapN; ++y)
                mapArray[x][y] = 1; // rand();
    }
    // execute
    omp_set_num_threads(MaxThreads);
    wait(1); // wait a bit
    cout << "Execute Map" << endl;
    resetPackages(packages);
    clock_start = omp_get_wtime();
    map(mapArray, mapM, mapN, iterations);
    clock_end = omp_get_wtime();
    samplePackages(packages);
    printExecutionTime(clock_start, clock_end);
    printPackages(packages);
    // clean
    cout << "Clean Map" << endl;
    for (int i = 0; i < mapM; ++i)
        delete mapArray[i];
    delete []mapArray;
    cout << endl;
}

void testStencil(vector<Rapl*> packages, int arrSize, int iter) {
    double clock_start, clock_end;
    // initialize
    omp_set_num_threads(AvailableThreads);
    const int stencilM = arrSize + 2, stencilN = arrSize + 2, iterations = iter;
    cout << "Initialize Stencil (" << stencilM << "x" << stencilN << " array)" << endl;
    double** stencilArray = new double*[stencilM];
    double** newArray = new double*[stencilM];
    #pragma omp parallel shared(stencilArray, newArray) firstprivate(stencilM,stencilN,iterations)
    {
        #pragma omp for
        for (int i = 0; i < stencilM; ++i)
            stencilArray[i] = new double[stencilN];
        #pragma omp for collapse(2)
        for (int x = 0; x < stencilM; ++x) // populate
            for (int y = 0; y < stencilN; ++y)
                stencilArray[x][y] = 0;
        #pragma omp for
        for (int wallUD = 0; wallUD < stencilN; ++wallUD)
        {
            stencilArray[0][wallUD] = 1;
            stencilArray[stencilN - 1][wallUD] = 1;
        }
        #pragma omp for nowait
        for (int i = 0; i < stencilM; ++i)
            newArray[i] = new double[stencilN];
    }
    // execute
    omp_set_num_threads(MaxThreads);
    wait(1); // wait a bit
    cout << "Execute Stencil" << endl;
    resetPackages(packages);
    clock_start = omp_get_wtime();
    stencil(stencilArray, newArray, stencilM, stencilN, iterations);
    clock_end = omp_get_wtime();
    samplePackages(packages);
    printExecutionTime(clock_start, clock_end);
    printPackages(packages);
    // clean
    cout << "Clean Stencil" << endl;
    for (int i = 0; i < stencilM; ++i)
        delete stencilArray[i];
    delete []stencilArray;
    for (int i = 0; i < stencilM; ++i)
        delete newArray[i];
    delete []newArray;
    cout << endl;
}

int main(int argc, char *argv[])
{
    int nSize = 100, iterations = 5000000;
    AvailableThreads = omp_get_max_threads();
    MaxThreads = AvailableThreads;
    std::ios_base::sync_with_stdio(false);

    //omp_set_num_threads(12); //manually set max amount of threads
    cout << "Quantidade maxima de threads inicial: " << AvailableThreads << endl;

    vector<Rapl*> packages;
    initializePackages(&packages);
    cout << endl;

    if (argc == 1) {
        cout << "==== Help ====" << endl;
        cout << "-r | --reduction       Testa o algoritmo Reduction" << endl;
        cout << "-m | --map             Testa o algoritmo Map" << endl;
        cout << "-s | --stencil         Testa o algoritmo Stencil" << endl;
        cout << "-c | --cores <1..N>    Altera a quantidade de cores/threads para usar" << endl;
        cout << "-o | --output <file>   Altera a saida para um arquivo" << endl;
        cout << "-n | --number <1..N>   Altera o tamanho do array (NxN)" << endl;
        cout << "-i | --iter <1..N>     Altera a quantidade de iteracoes" << endl;
    }

    for (int i=1; i<argc; ++i) {
        string arg(argv[i]);
        if (arg == "-r" || arg == "--reduction") {
            measureIdle(packages);
            testReduction(packages, nSize, iterations);
            wait(1); // wait a bit
            measureIdle(packages);
        } else if (arg == "-m" || arg == "--map") {
            measureIdle(packages);
            testMap(packages, nSize, iterations);
            wait(1); // wait a bit
            measureIdle(packages);
        } else if (arg == "-s" || arg == "--stencil") {
            measureIdle(packages);
            testStencil(packages, nSize, iterations);
            wait(1); // wait a bit
            measureIdle(packages);
        } else if (arg == "-c" || arg == "--cores") {
            ++i;
            if (argc < i) {
                cout << "Faltando numero de cores apos '-c'" << endl;
                exit(127);
            }
            string coresstr(argv[i]);
            int cores = stoi(coresstr);
            if (cores < 1 || cores > AvailableThreads) {
                cout << "Quantidade de cores deve ser de 1 ate " << AvailableThreads << endl;
                exit(127);
            }
            MaxThreads = cores;
            cout << "Quantidade maxima de threads alterada para " << MaxThreads << endl << endl;
        }  else if (arg == "-o" || arg == "--output") {
            ++i;
            if (argc < i) {
                cout << "Faltando arquivo apos '-c'" << endl;
                exit(127);
            }
            string filestr(argv[i]);
            FILE* fp = freopen(argv[i],"a",stdout);
            if (fp==NULL) continue;
            cout << "Saida alterada para " << filestr << endl << endl;
        } else if (arg == "-n" || arg == "--number") {
            ++i;
            if (argc < i) {
                cout << "Faltando numero do tamanho do array apos '-n'" << endl;
                exit(127);
            }
            string sizestr(argv[i]);
            int size = stoi(sizestr);
            if (size < 0) {
                cout << "Tamanho do array deve ser maior que 1" << endl;
                exit(127);
            }
            nSize = size;
            cout << "Tamanho do array alterado para " << nSize << "x" << nSize << endl << endl;
        } else if (arg == "-i" || arg == "--iter") {
            ++i;
            if (argc < i) {
                cout << "Faltando numero de quantidade de iteracoes apos '-i'" << endl;
                exit(127);
            }
            string iterstr(argv[i]);
            int iter = stoi(iterstr);
            if (iter < 1) {
                cout << "Quantidade de iteracoes deve ser maior que 1" << endl;
                exit(127);
            }
            iterations = iter;
            cout << "Quantidade de iteracoes alterado para " << iterations << endl << endl;
        } else {
            cout << "Argumento desconhecido na posicao " << i << endl << endl;
        }
    }

    return 0;
}

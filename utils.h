#ifndef UTILS_H
#define UTILS_H

#include "rapl.h"
#include <omp.h>
#include <iostream>
#include <vector>
#include <thread>

void initializePackages(std::vector<Rapl*> *packages);
void resetPackages(std::vector<Rapl*> packages);
void samplePackages(std::vector<Rapl*> packages);
void printPackages(std::vector<Rapl*> packages);
void printExecutionTime(double clock_start, double clock_end);

#endif

#include "utils.h"

using namespace std;

void initializePackages(vector<Rapl*> *packages) {
    #ifdef _WIN32
    return;
    #endif
    const unsigned int processor_count = std::thread::hardware_concurrency();
    for (unsigned int i=0; i<processor_count; ++i){
        Rapl* r = new Rapl(i);
        bool is_new = true;
        for (Rapl* rapl : *packages)
            if (rapl->get_package() == r->get_package())
                is_new = false;
        if (is_new)
            packages->push_back(r);
    }
}

void resetPackages(vector<Rapl*> packages) {
    #ifdef _WIN32
    return;
    #endif
    for (Rapl* r : packages)
        r->reset();
}

void samplePackages(vector<Rapl*> packages) {
    #ifdef _WIN32
    return;
    #endif
    for (Rapl* r : packages)
        r->sample();
}

void printPackages(vector<Rapl*> packages) {
    #ifdef _WIN32
    return;
    #endif
    for (Rapl* r : packages)
    {
        cout << "Package " << r->get_package() << ": PKG=" << r->pkg_total_energy() << "J";
        if (r->pp0_available()) {
            cout << ", PP0=" << r->pp0_total_energy() << "J";
        }
        if (r->pp1_available()) {
                cout << ", PP1=" << r->pp1_total_energy() << "J";
        }
        if (r->dram_available()) {
            cout << ", DRAM=" << r->dram_total_energy() << "J";
        }
        cout << endl;
    }
}

void printExecutionTime(double clock_start, double clock_end) {
    cout << "Execution time: " << (clock_end - clock_start) << "s" << endl;
}
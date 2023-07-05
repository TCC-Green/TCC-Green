// Based on: https://github.com/kentcz/rapl-tools
//           https://github.com/deater/uarch-configure/blob/master/rapl-read/rapl-read.c

// More CPU families, including AMD: https://github.com/RRZE-HPC/likwid/blob/a3bd1d103d0f71a59a8d470a008bf2e7abbc8d02/src/power.c#L95-L226

#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#ifdef __linux__ 
#include <unistd.h>
#elif _WIN32
#include <io.h>
#endif
#include <fcntl.h>
#include <cmath>
#include <omp.h>
#include <iostream>
#include "rapl.h"

/* AMD Support */
#define MSR_AMD_RAPL_POWER_UNIT		0xc0010299
#define MSR_AMD_PKG_ENERGY_STATUS	0xc001029B
#define MSR_AMD_PP0_ENERGY_STATUS	0xc001029A

/* Intel support */
#define MSR_INTEL_RAPL_POWER_UNIT	0x606
#define MSR_INTEL_PKG_ENERGY_STATUS	0x611
#define MSR_INTEL_PP0_ENERGY_STATUS	0x639

/*
 * Platform specific RAPL Domains.
 * Note that PP1 RAPL Domain is supported on 062A only
 * And DRAM RAPL Domain is supported on 062D only
 */
/* Package RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT    0x610
//#define MSR_PKG_ENERGY_STATUS     0x611
#define MSR_PKG_PERF_STATUS         0x613
#define MSR_PKG_POWER_INFO          0x614

/* PP0 RAPL Domain */
#define MSR_PP0_POWER_LIMIT         0x638
//#define MSR_PP0_ENERGY_STATUS     0x639
#define MSR_PP0_POLICY              0x63A
#define MSR_PP0_PERF_STATUS         0x63B

/* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_POWER_LIMIT         0x640
#define MSR_PP1_ENERGY_STATUS       0x641
#define MSR_PP1_POLICY              0x642

/* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT        0x618
#define MSR_DRAM_ENERGY_STATUS      0x619
#define MSR_DRAM_PERF_STATUS        0x61B
#define MSR_DRAM_POWER_INFO         0x61C

/* PSYS RAPL Domain */
#define MSR_PLATFORM_ENERGY_STATUS	0x64d

/* RAPL UNIT BITMASK */
#define POWER_UNIT_OFFSET			0
#define POWER_UNIT_MASK             0x0F

#define ENERGY_UNIT_OFFSET          0x08
#define ENERGY_UNIT_MASK            0x1F00

#define TIME_UNIT_OFFSET            0x10
#define TIME_UNIT_MASK              0xF000

/* CPU Vendors */
#define CPU_VENDOR_INTEL			1
#define CPU_VENDOR_AMD				2

/* Intel CPUs */
#define CPU_SANDYBRIDGE				42
#define CPU_SANDYBRIDGE_EP			45
#define CPU_IVYBRIDGE				58
#define CPU_IVYBRIDGE_EP			62
#define CPU_HASWELL					60
#define CPU_HASWELL_ULT				69
#define CPU_HASWELL_GT3E			70
#define CPU_HASWELL_EP				63
#define CPU_BROADWELL				61
#define CPU_BROADWELL_GT3E			71
#define CPU_BROADWELL_EP			79
#define CPU_BROADWELL_DE			86
#define CPU_SKYLAKE					78
#define CPU_SKYLAKE_HS				94
#define CPU_SKYLAKE_X				85
#define CPU_KNIGHTS_LANDING			87
#define CPU_KNIGHTS_MILL			133
#define CPU_KABYLAKE_MOBILE			142
#define CPU_KABYLAKE				158
#define CPU_ATOM_SILVERMONT			55
#define CPU_ATOM_AIRMONT			76
#define CPU_ATOM_MERRIFIELD			74
#define CPU_ATOM_MOOREFIELD			90
#define CPU_ATOM_GOLDMONT			92
#define CPU_ATOM_GEMINI_LAKE		122
#define CPU_ATOM_DENVERTON			95

/* AMD CPUs */
#define CPU_AMD_FAM17H				0xc000

Rapl::Rapl(int core_number) {
	core = core_number;

	open_msr();
	int cpu_model = detect_cpu();
	if (cpu_model == -1) {
		exit(127);
	}
	package = detect_package();
	if (package == -1) {
		std::cout << "Package not found" << std::endl;
		exit(127);
	}

	/* Read MSR_RAPL_POWER_UNIT Register */
	uint64_t raw_value = read_msr(msr_rapl_units);
	power_units = pow(0.5,	(double) (raw_value & 0xf));
	cpu_energy_units = pow(0.5,	(double) ((raw_value >> 8) & 0x1f));
	time_units = pow(0.5,	(double) ((raw_value >> 16) & 0xf));

	/* On Haswell EP and Knights Landing */
	/* The DRAM units differ from the CPU ones */
	if (different_units) {
		dram_energy_units=pow(0.5, (double)16);
	}
	else {
		dram_energy_units=cpu_energy_units;
	}

	/*printf("\t\tPower units = %.3fW\n",power_units);
	printf("\t\tCPU Energy units = %.8fJ\n",cpu_energy_units);
	printf("\t\tDRAM Energy units = %.8fJ\n",dram_energy_units);
	printf("\t\tTime units = %.8fs\n",time_units);
	printf("\n");*/

	switch(cpu_model) {

		case CPU_SANDYBRIDGE_EP:
		case CPU_IVYBRIDGE_EP:
			pp0_avail=1;
			pp1_avail=0;
			dram_avail=1;
			different_units=0;
			psys_avail=0;
			break;

		case CPU_HASWELL_EP:
		case CPU_BROADWELL_EP:
		case CPU_SKYLAKE_X:
			pp0_avail=0;
			pp1_avail=0;
			dram_avail=1;
			different_units=1;
			psys_avail=0;
			break;

		case CPU_KNIGHTS_LANDING:
		case CPU_KNIGHTS_MILL:
			pp0_avail=0;
			pp1_avail=0;
			dram_avail=1;
			different_units=1;
			psys_avail=0;
			break;

		case CPU_SANDYBRIDGE:
		case CPU_IVYBRIDGE:
			pp0_avail=1;
			pp1_avail=1;
			dram_avail=0;
			different_units=0;
			psys_avail=0;
			break;

		case CPU_HASWELL:
		case CPU_HASWELL_ULT:
		case CPU_HASWELL_GT3E:
		case CPU_BROADWELL:
		case CPU_BROADWELL_GT3E:
		case CPU_ATOM_GOLDMONT:
		case CPU_ATOM_GEMINI_LAKE:
		case CPU_ATOM_DENVERTON:
			pp0_avail=1;
			pp1_avail=1;
			dram_avail=1;
			different_units=0;
			psys_avail=0;
			break;

		case CPU_SKYLAKE:
		case CPU_SKYLAKE_HS:
		case CPU_KABYLAKE:
		case CPU_KABYLAKE_MOBILE:
			pp0_avail=1;
			pp1_avail=1;
			dram_avail=1;
			different_units=0;
			psys_avail=1;
			break;

		case CPU_AMD_FAM17H:
			pp0_avail=1;		// maybe
			pp1_avail=0;
			dram_avail=0;
			different_units=0;
			psys_avail=0;
			break;
	}

	/*
		Nota: Não é utilizado e possivelmente incorreto para diferentes tipos de CPU.
	*/
	/* Read MSR_PKG_POWER_INFO Register */
	/*raw_value = read_msr(MSR_PKG_POWER_INFO);
	thermal_spec_power = power_units * ((double)(raw_value & 0x7fff));
	minimum_power = power_units * ((double)((raw_value >> 16) & 0x7fff));
	maximum_power = power_units * ((double)((raw_value >> 32) & 0x7fff));
	time_window = time_units * ((double)((raw_value >> 48) & 0x7fff));*/

	reset();
}

void Rapl::reset() {

	prev_state = &state1;
	current_state = &state2;
	next_state = &state3;

	// sample twice to fill current and previous
	sample();
	sample();

	// Initialize running_total
	running_total.pkg = 0;
	running_total.pp0 = 0;
	running_total.pp1 = 0;
	running_total.dram = 0;
	running_total.psys = 0;
 	running_total.tsc = omp_get_wtick();
}

int Rapl::detect_cpu() {
	FILE *fff;

	int vendor=-1,family,model=-1;
	char buffer[BUFSIZ],*result;
	char vendor_string[BUFSIZ];

	fff=fopen("/proc/cpuinfo","r");
	if (fff==NULL) return -1;

	while(1) {
		result=fgets(buffer,BUFSIZ,fff);
		if (result==NULL) break;

		if (!strncmp(result,"vendor_id",8)) {
			sscanf(result,"%*s%*s%s",vendor_string);

			if (!strncmp(vendor_string,"GenuineIntel",12)) {
				vendor=CPU_VENDOR_INTEL;
			}
			if (!strncmp(vendor_string,"AuthenticAMD",12)) {
				vendor=CPU_VENDOR_AMD;
			}
		}

		if (!strncmp(result,"cpu family",10)) {
			sscanf(result,"%*s%*s%*s%d",&family);
		}

		if (!strncmp(result,"model",5)) {
			sscanf(result,"%*s%*s%d",&model);
		}

	}

	if (vendor==CPU_VENDOR_INTEL) {
		if (family!=6) {
			std::cout << "Wrong CPU family " << family << std::endl;
			return -1;
		}

		msr_rapl_units=MSR_INTEL_RAPL_POWER_UNIT;
		msr_pkg_energy_status=MSR_INTEL_PKG_ENERGY_STATUS;
		msr_pp0_energy_status=MSR_INTEL_PP0_ENERGY_STATUS;

		//printf("Found ");

		switch(model) {
			case CPU_SANDYBRIDGE:
				//printf("Sandybridge");
				break;
			case CPU_SANDYBRIDGE_EP:
				//printf("Sandybridge-EP");
				break;
			case CPU_IVYBRIDGE:
				//printf("Ivybridge");
				break;
			case CPU_IVYBRIDGE_EP:
				//printf("Ivybridge-EP");
				break;
			case CPU_HASWELL:
			case CPU_HASWELL_ULT:
			case CPU_HASWELL_GT3E:
				//printf("Haswell");
				break;
			case CPU_HASWELL_EP:
				//printf("Haswell-EP");
				break;
			case CPU_BROADWELL:
			case CPU_BROADWELL_GT3E:
				//printf("Broadwell");
				break;
			case CPU_BROADWELL_EP:
				//printf("Broadwell-EP");
				break;
			case CPU_SKYLAKE:
			case CPU_SKYLAKE_HS:
				//printf("Skylake");
				break;
			case CPU_SKYLAKE_X:
				//printf("Skylake-X");
				break;
			case CPU_KABYLAKE:
			case CPU_KABYLAKE_MOBILE:
				//printf("Kaby Lake");
				break;
			case CPU_KNIGHTS_LANDING:
				//printf("Knight's Landing");
				break;
			case CPU_KNIGHTS_MILL:
				//printf("Knight's Mill");
				break;
			case CPU_ATOM_GOLDMONT:
			case CPU_ATOM_GEMINI_LAKE:
			case CPU_ATOM_DENVERTON:
				//printf("Atom");
				break;
			default:
				std::cout << "Unsupported model " << model << std::endl;
				model=-1;
				break;
		}
	}

	if (vendor==CPU_VENDOR_AMD) {
		msr_rapl_units=MSR_AMD_RAPL_POWER_UNIT;
		msr_pkg_energy_status=MSR_AMD_PKG_ENERGY_STATUS;
		msr_pp0_energy_status=MSR_AMD_PP0_ENERGY_STATUS;

		if (family!=23) {
			std::cout << "Wrong CPU family " << family << std::endl;
			return -1;
		}
		model=CPU_AMD_FAM17H;
	}

	fclose(fff);

	//printf(" Processor type\n");

	return model;
}

int Rapl::detect_package() {
	char filename[BUFSIZ];
	FILE *fff;
	int package;

	sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id", core);
	fff=fopen(filename,"r");
	if (fff==NULL) return -1;
	int pkgr = fscanf(fff,"%d",&package);
	if (pkgr == 0) {
		package = -1;
	}
	fclose(fff);

	return package;
}

void Rapl::open_msr() {
	char msr_filename[BUFSIZ];

	sprintf(msr_filename, "/dev/cpu/%d/msr", core);
	fd = open(msr_filename, O_RDONLY);
	if ( fd < 0 ) {
		if ( errno == ENXIO ) {
			std::cout << "rdmsr: No CPU " << core << std::endl;
			exit(2);
		} else if ( errno == EIO ) {
			std::cout << "rdmsr: CPU " << core << " doesn't support MSRs" << std::endl;
			exit(3);
		} else {
			perror("rdmsr:open");
			std::cout << "Trying to open " << msr_filename << std::endl;
			exit(127);
		}
	}
}

int Rapl::get_package() {
	return package;
}

bool Rapl::pp0_available() {
	return pp0_avail;
}

bool Rapl::pp1_available() {
	return pp1_avail;
}

bool Rapl::dram_available() {
	return dram_avail;
}

uint64_t Rapl::read_msr(unsigned int msr_offset) {
	uint64_t data;
  #ifdef __linux__
	if (pread(fd, &data, sizeof(data), msr_offset) != sizeof(data)) {
		perror("read_msr():pread");
		exit(127);
	}
  #else
	std::cout << "unsupported OS" << std::endl;
  	exit(127);
  #endif
	return data;
}

void Rapl::sample() {
	next_state->pkg = (double)read_msr(msr_pkg_energy_status)*cpu_energy_units;
	if (pp0_avail) {
		next_state->pp0 = (double)read_msr(msr_pp0_energy_status)*cpu_energy_units;
	}
	if (pp1_avail) {
		next_state->pp1 = (double)read_msr(MSR_PP1_ENERGY_STATUS)*cpu_energy_units;
	}
	if (dram_avail) {
		next_state->dram = (double)read_msr(MSR_DRAM_ENERGY_STATUS)*cpu_energy_units;
	}
	if (psys_avail) {
		next_state->psys = (double)read_msr(MSR_PLATFORM_ENERGY_STATUS)*cpu_energy_units;
	}

 	next_state->tsc = omp_get_wtime();

	// Update running total
	running_total.pkg += energy_delta(current_state->pkg, next_state->pkg);
	running_total.pp0 += energy_delta(current_state->pp0, next_state->pp0);
	running_total.pp1 += energy_delta(current_state->pp1, next_state->pp1);
	running_total.dram += energy_delta(current_state->dram, next_state->dram);
	running_total.psys += energy_delta(current_state->psys, next_state->psys);

	// Rotate states
	rapl_state_t *pprev_state = prev_state;
	prev_state = current_state;
	current_state = next_state;
	next_state = pprev_state;
}

double Rapl::time_delta(double begin, double end) {
        return end - begin;
}

double Rapl::energy_delta(double before, double after) {
	/*uint64_t max_int = ~((uint32_t) 0);
	uint64_t eng_delta = after - before;

	// Check for rollovers
	if (before > after) {
		eng_delta = after + (max_int - before);
	}

	return eng_delta;*/
	return after - before;
}

/*double Rapl::pkg_average_power() {
	return pkg_total_energy() / total_time();
}

double Rapl::pp0_average_power() {
	return pp0_total_energy() / total_time();
}

double Rapl::pp1_average_power() {
	return pp1_total_energy() / total_time();
}

double Rapl::dram_average_power() {
	return dram_total_energy() / total_time();
}*/

double Rapl::pkg_total_energy() {
	return (double)running_total.pkg;
}

double Rapl::pp0_total_energy() {
	return (double)running_total.pp0;
}

double Rapl::pp1_total_energy() {
	return (double)running_total.pp1;
}

double Rapl::dram_total_energy() {
	return (double)running_total.dram;
}

double Rapl::psys_total_energy() {
	return (double)running_total.psys;
}

double Rapl::total_time() {
	return time_delta(running_total.tsc, current_state->tsc);
}

double Rapl::current_time() {
	return time_delta(prev_state->tsc, current_state->tsc);
}

// Based on: https://github.com/kentcz/rapl-tools

#include <cstdint>

#ifndef RAPL_H_
#define RAPL_H_

// https://user-images.githubusercontent.com/894892/120764898-ecf07280-c518-11eb-9155-92780cabcf52.png
struct rapl_state_t {
	double pkg; 	// entire package (cores, cache, controller, graphics)
	double pp0; 	// all cores
	double pp1; 	// graphics
	double dram; 	// all dram
	double psys; 	// package + edram + pch
	double tsc;		// time
};

class Rapl {

private:
	// Rapl configuration
	int fd;
	int core = -1, package = -1;
	double power_units, cpu_energy_units, time_units, dram_energy_units;
	//double thermal_spec_power, minimum_power, maximum_power, time_window; -- não usado
	unsigned int msr_rapl_units, msr_pkg_energy_status, msr_pp0_energy_status;
	bool dram_avail=0, pp0_avail=0, pp1_avail=0, psys_avail=0, different_units=0;

	// Rapl state
	rapl_state_t *current_state;
	rapl_state_t *prev_state;
	rapl_state_t *next_state;
	rapl_state_t state1, state2, state3, running_total;

	int detect_cpu();
	int detect_package();
	void open_msr();
	uint64_t read_msr(unsigned int msr_offset);
	double time_delta(double begin, double after);
	double energy_delta(double before, double after);

public:
	Rapl(int core_number);
	void reset();
	void sample();

	int get_package();

	bool pp0_available();
	bool pp1_available();
	bool dram_available();

	/*double pkg_average_power();
	double pp0_average_power();
	double pp1_average_power();
	double dram_average_power();*/

	double pkg_total_energy();
	double pp0_total_energy();
	double pp1_total_energy();
	double dram_total_energy();
	double psys_total_energy();

	double total_time();
	double current_time();
};

#endif /* RAPL_H_ */
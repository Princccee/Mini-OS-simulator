#ifndef RUNNER_H
#define RUNNER_H

#include "process.h"
#include "instruction.h"
#include "filesys.h"
#include <vector>
#include <climits>

// Runner simulates CPU/time, scheduling and IO waiting.
// For now Runner uses FCFS selection of READY processes.
// It uses FileSystem (passed in) to handle syscalls.
class Runner {
public:
    Runner(FileSystem &fs);

    // add a process (with its program) to the simulation
    void add_process(Process &&p);

    // run the simulation until all processes terminate
    void run_simulation(bool verbose = true);

private:
    FileSystem &fs;
    std::vector<Process> procs;

    int current_time;

    // move arrivals to ready
    void wake_arrivals();

    // wake IO completions
    void wake_io();

    // pick next ready process (FCFS by arrival/start_time)
    int pick_next_ready();

    // handle a syscall (returns true if process blocked)
    bool handle_syscall(Process &p, const Syscall &s);

    // log helper
    void log(const std::string &msg) const;
};

#endif // RUNNER_H

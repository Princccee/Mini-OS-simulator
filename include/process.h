#ifndef PROCESS_H
#define PROCESS_H

#include <string>

enum class ProcState { NEW, READY, RUNNING, WAITING, TERMINATED };

struct Process {
    int pid;
    std::string name;
    int arrival; // arrival time (integer time unit)
    int burst;   // original CPU burst
    int remaining; // remaining CPU time
    int priority;
    ProcState state;

    int start_time;      // first time the process gets CPU (-1 if never)
    int completion_time; // when finished (-1 if not)
    int response_time;   // start_time - arrival
    int waiting_time;    // turnaround - burst
    int turnaround_time; // completion - arrival

    Process(int pid_, const std::string &name_, int arrival_, int burst_, int priority_ = 0)
      : pid(pid_), name(name_), arrival(arrival_), burst(burst_), remaining(burst_),
        priority(priority_), state(ProcState::NEW),
        start_time(-1), completion_time(-1), response_time(-1),
        waiting_time(0), turnaround_time(0) {}
};

#endif // PROCESS_H

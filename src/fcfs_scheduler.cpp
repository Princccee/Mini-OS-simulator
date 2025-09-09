#include "fcfs_scheduler.h"
#include <algorithm>

void FCFS_Scheduler::add_process(const Process &p) {
    procs.push_back(p);
}

void FCFS_Scheduler::run() {
    if (procs.empty()) return;

    // sort by arrival time, then pid
    std::sort(procs.begin(), procs.end(), [](const Process &a, const Process &b){
        if (a.arrival != b.arrival) return a.arrival < b.arrival;
        return a.pid < b.pid;
    });

    int time = 0;
    for (auto p : procs) { // copy so we can mutate local p
        if (time < p.arrival) time = p.arrival;
        p.start_time = time;
        p.response_time = p.start_time - p.arrival;
        p.state = ProcState::RUNNING;

        // Gantt: process runs to completion (non-preemptive)
        gantt.push_back({p.pid, p.burst});
        time += p.burst;

        p.completion_time = time;
        p.remaining = 0;
        p.state = ProcState::TERMINATED;
        p.turnaround_time = p.completion_time - p.arrival;
        p.waiting_time = p.turnaround_time - p.burst;

        finished.push_back(p);
    }
}

std::vector<Process> FCFS_Scheduler::get_finished_processes() const { return finished; }
std::vector<GanttSeg> FCFS_Scheduler::get_gantt() const { return gantt; }

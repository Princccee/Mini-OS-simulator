#include "rr_scheduler.h"
#include <queue>
#include <algorithm>

RR_Scheduler::RR_Scheduler(int quantum_) : quantum(quantum_) {}

void RR_Scheduler::add_process(const Process &p) {
    procs.push_back(p);
}

void RR_Scheduler::run() {
    if (procs.empty()) return;

    // sort by arrival time so we can push arriving processes as time advances
    std::sort(procs.begin(), procs.end(), [](const Process &a, const Process &b){
        if (a.arrival != b.arrival) return a.arrival < b.arrival;
        return a.pid < b.pid;
    });

    int n = (int)procs.size();
    std::queue<int> rq; // indexes into procs
    int time = 0;
    size_t next = 0;
    int finished_count = 0;

    // add processes that arrive at t=0
    while (next < procs.size() && procs[next].arrival <= time) {
        procs[next].state = ProcState::READY;
        rq.push((int)next);
        ++next;
    }

    // if nothing at time 0, fast-forward to the first arrival
    if (rq.empty() && next < procs.size()) {
        time = procs[next].arrival;
        procs[next].state = ProcState::READY;
        rq.push((int)next++);
    }

    while (finished_count < n) {
        if (rq.empty()) {
            // advance time to next arrival if no ready processes
            if (next < procs.size()) {
                time = std::max(time, procs[next].arrival);
                procs[next].state = ProcState::READY;
                rq.push((int)next++);
            }
            continue;
        }

        int idx = rq.front(); rq.pop();
        Process &p = procs[idx];

        // first time the process gets CPU -> set start/response
        if (p.start_time == -1) {
            p.start_time = time;
            p.response_time = p.start_time - p.arrival;
        }

        p.state = ProcState::RUNNING;
        int run = std::min(quantum, p.remaining);
        gantt.push_back({p.pid, run});
        time += run;
        p.remaining -= run;

        // push newly arrived processes that came during this time slice
        while (next < procs.size() && procs[next].arrival <= time) {
            procs[next].state = ProcState::READY;
            rq.push((int)next++);
        }

        if (p.remaining == 0) {
            p.completion_time = time;
            p.state = ProcState::TERMINATED;
            p.turnaround_time = p.completion_time - p.arrival;
            p.waiting_time = p.turnaround_time - p.burst;
            finished.push_back(p);
            ++finished_count;
        } else {
            p.state = ProcState::READY;
            rq.push(idx);
        }
    }
}

std::vector<Process> RR_Scheduler::get_finished_processes() const { return finished; }
std::vector<GanttSeg> RR_Scheduler::get_gantt() const { return gantt; }

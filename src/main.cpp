#include <iostream>
#include <iomanip>
#include <vector>

#include "process.h"
#include "fcfs_scheduler.h"
#include "rr_scheduler.h"

using namespace std;

void print_gantt(const vector<GanttSeg> &g) {
    cout << "Gantt chart (PID(run_time)):\n|";
    for (auto &seg : g) {
        cout << " P" << seg.first << "(" << seg.second << ") |";
    }
    cout << "\n";
}

void print_stats(const vector<Process> &procs) {
    cout << left << setw(6) << "PID" << setw(10) << "Name"
         << setw(8) << "Arrival" << setw(8) << "Burst"
         << setw(8) << "Start" << setw(12) << "Completion"
         << setw(12) << "Turnaround" << setw(8) << "Waiting"
         << setw(10) << "Response" << "\n";

    double total_turnaround = 0, total_waiting = 0, total_response = 0;
    for (auto &p : procs) {
        cout << setw(6) << p.pid << setw(10) << p.name
             << setw(8) << p.arrival << setw(8) << p.burst
             << setw(8) << p.start_time << setw(12) << p.completion_time
             << setw(12) << p.turnaround_time << setw(8) << p.waiting_time
             << setw(10) << p.response_time << "\n";
        total_turnaround += p.turnaround_time;
        total_waiting += p.waiting_time;
        total_response += p.response_time;
    }
    int n = (int)procs.size();
    cout << fixed << setprecision(3);
    cout << "\nAvg Turnaround = " << (total_turnaround / n)
         << ", Avg Waiting = " << (total_waiting / n)
         << ", Avg Response = " << (total_response / n) << "\n";
}

int main() {
    // sample processes: (pid, name, arrival, burst)
    vector<Process> sample = {
        Process(1, "A", 0, 5),
        Process(2, "B", 1, 3),
        Process(3, "C", 2, 7)
    };

    cout << "=== FCFS Scheduler ===\n";
    FCFS_Scheduler fcfs;
    for (auto &p : sample) fcfs.add_process(p);
    fcfs.run();
    print_gantt(fcfs.get_gantt());
    print_stats(fcfs.get_finished_processes());

    cout << "\n=== Round Robin (quantum = 2) ===\n";
    RR_Scheduler rr(2);
    for (auto &p : sample) rr.add_process(p);
    rr.run();
    print_gantt(rr.get_gantt());
    print_stats(rr.get_finished_processes());

    return 0;
}

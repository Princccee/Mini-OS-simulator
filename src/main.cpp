#include <iostream>
#include <iomanip>
#include <vector>

#include "process.h"
#include "fcfs_scheduler.h"
#include "rr_scheduler.h"
#include "sjf_scheduler.h"
#include "priority_scheduler.h"

using namespace std;

struct Summary {
    string algo;
    double avg_turnaround;
    double avg_waiting;
    double avg_response;
};

void print_summary_table(const vector<Summary> &results) {
    cout << "\n=== Final Summary (All Algorithms) ===\n";
    cout << left << setw(20) << "Algorithm"
         << setw(15) << "Avg Turnaround"
         << setw(15) << "Avg Waiting"
         << setw(15) << "Avg Response" << "\n";
    cout << string(65, '-') << "\n";

    for (auto &r : results) {
        cout << setw(20) << r.algo
             << setw(15) << fixed << setprecision(3) << r.avg_turnaround
             << setw(15) << r.avg_waiting
             << setw(15) << r.avg_response << "\n";
    }
}


void print_gantt(const vector<GanttSeg> &g) {
    cout << "Gantt chart (PID(run_time)):\n|";
    for (auto &seg : g) {
        cout << " P" << seg.first << "(" << seg.second << ") |";
    }
    cout << "\n";
}

Summary print_stats(const string &algo, const vector<Process> &procs) {
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

    return {algo, total_turnaround / n, total_waiting / n, total_response / n};
}

int main() {
    vector<Process> sample = {
        Process(1, "A", 0, 5, 2),
        Process(2, "B", 1, 3, 1),
        Process(3, "C", 2, 7, 3)
    };

    vector<Summary> results;

    // --- FCFS ---
    cout << "=== FCFS Scheduler ===\n";
    FCFS_Scheduler fcfs;
    for (auto &p : sample) fcfs.add_process(p);
    fcfs.run();
    print_gantt(fcfs.get_gantt());
    results.push_back(print_stats("FCFS", fcfs.get_finished_processes()));

    // --- Round Robin ---
    cout << "\n=== Round Robin (quantum = 2) ===\n";
    RR_Scheduler rr(2);
    for (auto &p : sample) rr.add_process(p);
    rr.run();
    print_gantt(rr.get_gantt());
    results.push_back(print_stats("Round Robin (q=2)", rr.get_finished_processes()));

    // --- SJF Non-Preemptive ---
    cout << "\n=== SJF (Non-Preemptive) ===\n";
    SJFScheduler sjf_np(SJFType::NON_PREEMPTIVE);
    for (auto &p : sample) sjf_np.addProcess(p);
    sjf_np.run();
    print_gantt(sjf_np.get_gantt());
    results.push_back(print_stats("SJF (NP)", sjf_np.get_finished_processes()));

    // --- SJF Preemptive (SRTF) ---
    cout << "\n=== SJF (Preemptive - SRTF) ===\n";
    SJFScheduler sjf_p(SJFType::PREEMPTIVE);
    for (auto &p : sample) sjf_p.addProcess(p);
    sjf_p.run();
    print_gantt(sjf_p.get_gantt());
    results.push_back(print_stats("SJF (P)", sjf_p.get_finished_processes()));

    // --- Priority Non-Preemptive ---
    cout << "\n=== Priority (Non-Preemptive) ===\n";
    PriorityScheduler prio_np(PriorityType::NON_PREEMPTIVE);
    for (auto &p : sample) prio_np.addProcess(p);
    prio_np.run();
    print_gantt(prio_np.get_gantt());
    results.push_back(print_stats("Priority (NP)", prio_np.get_finished_processes()));

    // --- Priority Preemptive ---
    cout << "\n=== Priority (Preemptive) ===\n";
    PriorityScheduler prio_p(PriorityType::PREEMPTIVE);
    for (auto &p : sample) prio_p.addProcess(p);
    prio_p.run();
    print_gantt(prio_p.get_gantt());
    results.push_back(print_stats("Priority (P)", prio_p.get_finished_processes()));

    // --- Final Summary Table ---
    print_summary_table(results);

    return 0;
}

#include "priority_scheduler.h"
#include <iostream>
#include <algorithm>
#include <queue>

PriorityScheduler::PriorityScheduler(PriorityType type) : type(type) {}

void PriorityScheduler::addProcess(const Process& p) {
    processes.push_back(p);
}

void PriorityScheduler::run() {
    finished.clear();
    gantt.clear();

    int n = processes.size();
    if (n == 0) return;

    std::vector<Process> procs = processes;
    int time = 0, completed = 0;

    // Lower value = higher priority
    auto cmp = [](Process* a, Process* b) {
        if (a->priority == b->priority) return a->arrival > b->arrival;
        return a->priority > b->priority;
    };
    std::priority_queue<Process*, std::vector<Process*>, decltype(cmp)> pq(cmp);

    while (completed < n) {
        // Push processes that arrived
        for (auto& p : procs) {
            if (p.arrival <= time && p.state == ProcState::READY) {
                pq.push(&p);
                p.state = ProcState::WAITING;
            }
        }

        if (!pq.empty()) {
            Process* cur = pq.top();
            pq.pop();

            if (cur->remaining == cur->burst) {
                cur->start_time = time;
                cur->response_time = cur->start_time - cur->arrival;
            }

            cur->state = ProcState::RUNNING;

            if (type == PriorityType::NON_PREEMPTIVE) {
                // Run entire burst
                gantt.push_back({cur->pid, cur->remaining});
                time += cur->remaining;
                cur->remaining = 0;
                cur->completion_time = time;
                cur->turnaround_time = cur->completion_time - cur->arrival;
                cur->waiting_time = cur->turnaround_time - cur->burst;
                cur->state = ProcState::TERMINATED;
                finished.push_back(*cur);
                completed++;
            } else {
                // Preemptive: run 1 unit
                gantt.push_back({cur->pid, 1});
                cur->remaining--;
                time++;

                if (cur->remaining == 0) {
                    cur->completion_time = time;
                    cur->turnaround_time = cur->completion_time - cur->arrival;
                    cur->waiting_time = cur->turnaround_time - cur->burst;
                    cur->state = ProcState::TERMINATED;
                    finished.push_back(*cur);
                    completed++;
                } else {
                    cur->state = ProcState::WAITING;
                    pq.push(cur);
                }
            }
        } else {
            // CPU idle
            gantt.push_back({-1, 1}); // -1 for Idle
            time++;
        }
    }
}

std::vector<Process> PriorityScheduler::get_finished_processes() const { return finished; }
std::vector<GanttSeg> PriorityScheduler::get_gantt() const { return gantt; }

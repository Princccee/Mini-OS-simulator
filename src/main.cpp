// src/main.cpp
#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>

enum class ProcState { NEW, READY, RUNNING, WAITING, TERMINATED };

struct Process {
    int pid;
    std::string name;
    int burst;      // total CPU demand
    int remaining;  // remaining CPU time
    int priority;
    ProcState state;
    Process(int id, const std::string &n, int b, int pr = 0)
        : pid(id), name(n), burst(b), remaining(b), priority(pr), state(ProcState::NEW) {}
};

class Scheduler {
public:
    virtual ~Scheduler() = default;
    virtual void add_process(const Process &p) = 0;
    virtual void run() = 0;
};

class RoundRobin : public Scheduler {
    std::vector<Process> procs;
    int quantum;
public:
    explicit RoundRobin(int q = 2) : quantum(q) {}
    void add_process(const Process &p) override {
        Process np = p;
        np.state = ProcState::READY;
        procs.push_back(np);
    }
    void run() override {
        if (procs.empty()) {
            std::cout << "No processes to schedule.\n";
            return;
        }
        std::queue<int> q;
        for (int i = 0; i < (int)procs.size(); ++i) q.push(i);

        int time = 0;
        std::cout << "Gantt chart (PID(run_time)):\n|";
        while (!q.empty()) {
            int idx = q.front(); q.pop();
            Process &p = procs[idx];
            p.state = ProcState::RUNNING;
            int run = std::min(quantum, p.remaining);
            std::cout << " P" << p.pid << "(" << run << ") |";
            time += run;
            p.remaining -= run;
            if (p.remaining <= 0) {
                p.state = ProcState::TERMINATED;
            } else {
                p.state = ProcState::READY;
                q.push(idx);
            }
        }
        std::cout << "\nTotal simulated time: " << time << "\n\nFinal states:\n";
        for (auto &p : procs) {
            std::cout << "PID " << p.pid << " name=" << p.name
                      << " state=" << (p.state == ProcState::TERMINATED ? "TERMINATED" : "OTHER")
                      << " remaining=" << p.remaining << "\n";
        }
    }
};

int main() {
    RoundRobin scheduler(2);
    // sample processes
    scheduler.add_process(Process(1, "A", 5));
    scheduler.add_process(Process(2, "B", 3));
    scheduler.add_process(Process(3, "C", 7));

    std::cout << "=== Mini OS Simulator (skeleton) ===\n";
    scheduler.run();
    return 0;
}

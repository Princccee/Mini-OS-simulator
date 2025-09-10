#ifndef SJF_SCHEDULER_H
#define SJF_SCHEDULER_H

#include "process.h"
#include <vector>
#include <string>

// Consistent with RR Scheduler
using GanttSeg = std::pair<int, int>;

enum class SJFType {
    NON_PREEMPTIVE,
    PREEMPTIVE // Shortest Remaining Time First
};

class SJFScheduler {
public:
    SJFScheduler(SJFType type);
    void addProcess(const Process& p);
    void run();

    std::vector<Process> get_finished_processes() const;
    std::vector<GanttSeg> get_gantt() const;

private:
    SJFType type;
    std::vector<Process> processes;
    std::vector<Process> finished;
    std::vector<GanttSeg> gantt;
};

#endif

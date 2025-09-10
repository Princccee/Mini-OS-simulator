#ifndef PRIORITY_SCHEDULER_H
#define PRIORITY_SCHEDULER_H

#include "process.h"
#include <vector>
#include <string>

// Consistent with RR Scheduler
using GanttSeg = std::pair<int, int>;

enum class PriorityType {
    NON_PREEMPTIVE,
    PREEMPTIVE
};

class PriorityScheduler {
public:
    PriorityScheduler(PriorityType type);
    void addProcess(const Process& p);
    void run();

    std::vector<Process> get_finished_processes() const;
    std::vector<GanttSeg> get_gantt() const;

private:
    PriorityType type;
    std::vector<Process> processes;
    std::vector<Process> finished;
    std::vector<GanttSeg> gantt;
};

#endif

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <vector>
#include <utility> // pair

// Pair format for gantt/log: <pid, runtime>
using GanttSeg = std::pair<int,int>;

class Scheduler {
public:
    virtual ~Scheduler() = default;
    virtual void add_process(const Process &p) = 0;
    virtual void run() = 0;
    virtual std::vector<Process> get_finished_processes() const = 0;
    virtual std::vector<GanttSeg> get_gantt() const = 0;
};

#endif // SCHEDULER_H

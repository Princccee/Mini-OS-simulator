#ifndef FCFS_SCHEDULER_H
#define FCFS_SCHEDULER_H

#include "scheduler.h"

class FCFS_Scheduler : public Scheduler {
    std::vector<Process> procs;
    std::vector<Process> finished;
    std::vector<GanttSeg> gantt;
public:
    void add_process(const Process &p) override;
    void run() override;
    std::vector<Process> get_finished_processes() const override;
    std::vector<GanttSeg> get_gantt() const override;
};

#endif // FCFS_SCHEDULER_H

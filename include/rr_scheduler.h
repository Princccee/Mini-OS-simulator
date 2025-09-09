#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H

#include "scheduler.h"

class RR_Scheduler : public Scheduler {
    std::vector<Process> procs;
    std::vector<Process> finished;
    std::vector<GanttSeg> gantt;
    int quantum;
public:
    explicit RR_Scheduler(int quantum_ = 2);
    void add_process(const Process &p) override;
    void run() override;
    std::vector<Process> get_finished_processes() const override;
    std::vector<GanttSeg> get_gantt() const override;
};

#endif // RR_SCHEDULER_H

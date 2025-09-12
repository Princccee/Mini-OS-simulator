#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include "instruction.h"

// Process state enum you already have
enum class ProcState { NEW, READY, RUNNING, WAITING, TERMINATED };

// Forward-declare Instruction (defined in instruction.h)
// struct Instruction;

struct Process {
    int pid;
    std::string name;
    int arrival;      // arrival time (simulation)
    int burst;        // original CPU burst (optional legacy)
    int remaining;    // remaining CPU time (legacy)
    int priority;

    // scheduling bookkeeping (start/completion/response etc.)
    int start_time;
    int completion_time;
    int response_time;
    int waiting_time;
    int turnaround_time;
    ProcState state;

    // Program model
    std::vector<Instruction> program; // sequence of instructions (CPU + SYSCALL)
    size_t pc;             // index of next instruction
    int instr_remaining;   // remaining time for current CPU instruction

    // blocking / I/O bookkeeping
    int blocked_until;     // if WAITING, simulation time when it becomes READY
    // Owned resource ids (e.g., memory blocks) — used later
    std::vector<int> owned_blocks;

    Process() = delete;
    Process(int pid_, const std::string &name_, int arrival_, int burst_, int priority_ = 0)
      : pid(pid_), name(name_), arrival(arrival_), burst(burst_), remaining(burst_),
        priority(priority_), state(ProcState::NEW),
        start_time(-1), completion_time(-1), response_time(-1),
        waiting_time(0), turnaround_time(0),
        pc(0), instr_remaining(0), blocked_until(-1) {}
};

#endif // PROCESS_H

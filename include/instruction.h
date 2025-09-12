#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>

// Simple syscall descriptor used by Instruction
struct Syscall {
    std::string name;            // "write", "read", "delete", "touch", etc.
    std::vector<std::string> args; // arguments, e.g. path, content, size (string encoded)
    int io_latency = 0;          // simulated I/O latency (time units)
};

// Instruction types
enum class InstrType { CPU, SYSCALL, SLEEP };

// Instruction: CPU(n) — use CPU n units; SYSCALL — perform an FS op; SLEEP waits
struct Instruction {
    InstrType type;
    int cpu_time = 0;      // for CPU
    Syscall syscall;       // for SYSCALL
    int sleep_time = 0;    // for SLEEP

    // convenience ctors
    static Instruction CPU(int t) {
        Instruction i; i.type = InstrType::CPU; i.cpu_time = t; return i;
    }
    static Instruction SYSCALL(const Syscall &s) {
        Instruction i; i.type = InstrType::SYSCALL; i.syscall = s; return i;
    }
    static Instruction SLEEP(int t) {
        Instruction i; i.type = InstrType::SLEEP; i.sleep_time = t; return i;
    }
};

#endif // INSTRUCTION_H

#include "runner.h"
#include "instruction.h"
#include "filesys.h"
#include <iostream>

int main() {
    FileSystem fs;
    // Optionally load saved FS state:
    fs.load_from_file("fs_state.json");

    // Ensure /tmp exists so writes to /tmp/* succeed
    fs.mkdir("/tmp");

    Runner runner(fs);

    // Process P1: CPU(2) -> write /tmp/a.txt -> CPU(1) -> read it
    Process p1(1, "P1", 0, 0, 0);
    p1.program.push_back(Instruction::CPU(2));
    Syscall w1; w1.name = "write"; w1.args = {"/tmp/a.txt", "hello-from-p1"}; w1.io_latency = 3;
    p1.program.push_back(Instruction::SYSCALL(w1));
    p1.program.push_back(Instruction::CPU(1));
    Syscall r1; r1.name = "read"; r1.args = {"/tmp/a.txt"}; r1.io_latency = 2;
    p1.program.push_back(Instruction::SYSCALL(r1));

    // Process P2 (arrives at t=1): CPU(1) -> read -> CPU(1)
    Process p2(2, "P2", 1, 0, 0);
    p2.program.push_back(Instruction::CPU(1));
    Syscall r2; r2.name = "read"; r2.args = {"/tmp/a.txt"}; r2.io_latency = 1;
    p2.program.push_back(Instruction::SYSCALL(r2));
    p2.program.push_back(Instruction::CPU(1));

    runner.add_process(std::move(p1));
    runner.add_process(std::move(p2));

    runner.run_simulation(true);

    // Save FS state for inspection
    fs.save_to_file("fs_state_after.json");
    return 0;
}

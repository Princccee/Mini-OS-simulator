#include "runner.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <climits>   // for INT_MAX

Runner::Runner(FileSystem &fs_) : fs(fs_), current_time(0) {}

void Runner::add_process(Process &&p) {
    p.state = ProcState::NEW;
    p.pc = 0;
    p.instr_remaining = 0;
    p.blocked_until = -1;
    procs.push_back(std::move(p));
}

void Runner::wake_arrivals() {
    for (auto &p : procs) {
        if (p.state == ProcState::NEW && p.arrival <= current_time) {
            p.state = ProcState::READY;
            if (p.start_time == -1) p.start_time = current_time;
        }
    }
}

void Runner::wake_io() {
    for (auto &p : procs) {
        if (p.state == ProcState::WAITING && p.blocked_until <= current_time) {
            p.state = ProcState::READY;
            p.blocked_until = -1;
            std::ostringstream oss;
            oss << "t=" << current_time << ": PID " << p.pid << " I/O done -> READY";
            log(oss.str());
        }
    }
}

int Runner::pick_next_ready() {
    int best = -1;
    for (size_t i = 0; i < procs.size(); ++i) {
        if (procs[i].state == ProcState::READY) {
            if (best == -1) best = (int)i;
            else {
                if (procs[i].arrival < procs[best].arrival) best = (int)i;
                else if (procs[i].arrival == procs[best].arrival && procs[i].pid < procs[best].pid) best = (int)i;
            }
        }
    }
    return best;
}

bool Runner::handle_syscall(Process &p, const Syscall &s) {
    std::ostringstream oss;
    oss << "t=" << current_time << ": PID " << p.pid << " SYSCALL " << s.name;
    for (auto &a : s.args) oss << " \"" << a << "\"";
    oss << " (latency=" << s.io_latency << ")";
    log(oss.str());

    if (s.name == "write") {
        if (s.args.size() < 2) {
            log("  write: invalid args");
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            return false;
        }
        const std::string &path = s.args[0];
        const std::string &content = s.args[1];
        bool ok = fs.write_file(path, content);
        if (!ok) {
            log("  write: failed (terminating process)");
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            return false;
        } else {
            log("  write: success");
            if (s.io_latency > 0) {
                p.state = ProcState::WAITING;
                p.blocked_until = current_time + s.io_latency;
                return true;
            }
            return false;
        }
    } else if (s.name == "read") {
        if (s.args.size() < 1) {
            log("  read: invalid args");
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            return false;
        }
        const std::string &path = s.args[0];
        std::string out;
        bool ok = fs.cat(path, out);
        if (!ok) {
            log("  read: file not found (terminating process)");
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            return false;
        } else {
            std::ostringstream o2;
            o2 << "  read: \"" << out << "\"";
            log(o2.str());
            if (s.io_latency > 0) {
                p.state = ProcState::WAITING;
                p.blocked_until = current_time + s.io_latency;
                return true;
            }
            return false;
        }
    } else if (s.name == "delete") {
        if (s.args.size() < 1) {
            log("  delete: invalid args");
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            return false;
        }
        const std::string &path = s.args[0];
        bool ok = fs.remove_file(path);
        if (!ok) {
            log("  delete: failed (terminating process)");
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            return false;
        } else {
            log("  delete: ok");
            if (s.io_latency > 0) {
                p.state = ProcState::WAITING;
                p.blocked_until = current_time + s.io_latency;
                return true;
            }
            return false;
        }
    } else if (s.name == "touch") {
        if (s.args.size() < 1) {
            log("touch: invalid args");
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            return false;
        }
        const std::string &path = s.args[0];
        bool ok = fs.touch(path);
        if (!ok) {
            log("  touch: failed (terminating process)");
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            return false;
        } else {
            log("  touch: ok");
            if (s.io_latency > 0) {
                p.state = ProcState::WAITING;
                p.blocked_until = current_time + s.io_latency;
                return true;
            }
            return false;
        }
    }

    log("  unknown syscall");
    p.state = ProcState::TERMINATED;
    p.completion_time = current_time;
    return false;
}

void Runner::log(const std::string &msg) const {
    std::cout << msg << "\n";
}

void Runner::run_simulation(bool verbose) {
    (void)verbose;
    std::sort(procs.begin(), procs.end(), [](const Process &a, const Process &b){
        if (a.arrival != b.arrival) return a.arrival < b.arrival;
        return a.pid < b.pid;
    });

    int earliest = -1;
    for (auto &p : procs) if (earliest == -1 || p.arrival < earliest) earliest = p.arrival;
    current_time = (earliest >= 0 ? earliest : 0);

    wake_arrivals();

    while (true) {
        wake_io();
        wake_arrivals();

        bool any_left = false;
        for (auto &p : procs) if (p.state != ProcState::TERMINATED) { any_left = true; break; }
        if (!any_left) break;

        int idx = pick_next_ready();
        if (idx == -1) {
            int next_time = INT_MAX;
            for (auto &p : procs) {
                if (p.state == ProcState::NEW) next_time = std::min(next_time, p.arrival);
                if (p.state == ProcState::WAITING && p.blocked_until >= 0) next_time = std::min(next_time, p.blocked_until);
            }
            if (next_time == INT_MAX) break;
            current_time = std::max(current_time, next_time);
            wake_io();
            wake_arrivals();
            continue;
        }

        Process &p = procs[idx];
        p.state = ProcState::RUNNING;
        if (p.start_time == -1) p.start_time = current_time;
        std::ostringstream oss;
        oss << "t=" << current_time << ": PID " << p.pid << " START running";
        log(oss.str());

        if (p.pc >= p.program.size()) {
            p.state = ProcState::TERMINATED;
            p.completion_time = current_time;
            std::ostringstream o2;
            o2 << "t=" << current_time << ": PID " << p.pid << " TERMINATED";
            log(o2.str());
            continue;
        }

        Instruction instr = p.program[p.pc];
        if (instr.type == InstrType::CPU) {
            int take = instr.cpu_time;
            current_time += take;
            std::ostringstream o3;
            o3 << "t=" << (current_time - take) << " -> " << current_time << ": PID " << p.pid
               << " CPU(" << take << ")";
            log(o3.str());
            p.pc++;
        }  
        else if (instr.type == InstrType::SYSCALL) {
                bool blocked = handle_syscall(p, instr.syscall);
                current_time += 1;

                if (p.state != ProcState::TERMINATED) {
                    p.pc++;  // always advance to next instruction
                }

                if (p.pc >= p.program.size() && p.state != ProcState::TERMINATED) {
                    p.state = ProcState::TERMINATED;
                    p.completion_time = current_time;
                    std::ostringstream o6;
                    o6 << "t=" << current_time << ": PID " << p.pid << " TERMINATED";
                    log(o6.str());
                } else if (blocked) {
                    std::ostringstream o4;
                    o4 << "t=" << current_time << ": PID " << p.pid << " BLOCKED until " << p.blocked_until;
                    log(o4.str());
                }
        } 
        else if (instr.type == InstrType::SLEEP) {
            int until = current_time + instr.sleep_time;
            p.state = ProcState::WAITING;
            p.blocked_until = until;
            std::ostringstream o5;
            o5 << "t=" << current_time << ": PID " << p.pid << " SLEEP until " << until;
            log(o5.str());
        }

        if (p.state == ProcState::RUNNING) {
            if (p.pc >= p.program.size()) {
                p.state = ProcState::TERMINATED;
                p.completion_time = current_time;
                std::ostringstream o7;
                o7 << "t=" << current_time << ": PID " << p.pid << " TERMINATED";
                log(o7.str());
            } else {
                p.state = ProcState::READY;
            }
        }
    }

    std::cout << "\n=== Simulation complete at t=" << current_time << " ===\n";
    for (const auto &p : procs) {
        std::cout << "PID " << p.pid << " state=" 
                  << (p.state == ProcState::TERMINATED ? "TERMINATED":"OTHER")
                  << " start=" << p.start_time 
                  << " completion=" << p.completion_time << "\n";
    }
}

# Mini OS Simulator

This project is a **modular operating system simulator** built in C++ that models the essential subsystems of a real operating system.  
It provides a sandbox to explore and experiment with **process scheduling**, **memory management**, **file system operations**, and **system call handling** without the complexity of kernel-level development.

---

## 🚀 Features

The simulator is split into modular components, each representing a subsystem of a real OS:

### 1. **Process Scheduling**
- Implements multiple scheduling algorithms:
  - First-Come-First-Serve (FCFS)
  - Shortest Job First (SJF)
  - Round Robin (RR)
  - Priority Scheduling
- Tracks process states: `NEW`, `READY`, `RUNNING`, `WAITING`, `TERMINATED`
- Supports context switching, CPU burst, and I/O blocking.

### 2. **Memory Management**
- Basic memory manager with allocation/freeing
- Paging support (fixed-size pages, page tables)
- Demonstrates how processes access virtual memory mapped to physical memory.

### 3. **File System**
- Hierarchical, in-memory file system
- Supports operations like:
  - `touch` (create file)
  - `write` (write to file)
  - `read` (read contents)
  - `delete` (remove file)
  - `mkdir` (create directories)
- Filesystem state can be **saved/loaded** as JSON.

### 4. **System Calls**
- Processes can issue system calls in their "program":
  - `read`, `write`, `delete`, `touch`
  - `sleep` (block for a certain time)
- System calls incur **I/O latency**, blocking processes until completion.

### 5. **Runner (Simulation Orchestrator)**
- Advances simulation time, wakes I/O, picks next process to run.
- Logs execution trace: CPU bursts, syscalls, state transitions.
- Provides statistics like process completion time.

---

## 🧩 Project Structure

- `include/` → Header files
- `src/` → Implementation files
  - `fcfs_scheduler.cpp`, `rr_scheduler.cpp`, `sjf_scheduler.cpp`, `priority_scheduler.cpp`
  - `memory_manager.cpp`, `paging.cpp`
  - `filesys.cpp`
  - `runner.cpp`
  - `*_demo.cpp` (demo drivers with `main()`)
- `CMakeLists.txt` → Modular build setup

---

## 🔧 Building and Running

This project uses **CMake** and can be built inside Docker for a reproducible environment.

### 1. Build Docker Image
```bash
docker build -t mini-os-simulator .
```

### 2. Run the docker container with bind mount
```bash
docker run --rm -it -v "$PWD":/workspace -w /workspace mini-os.sim:dev
```

### 3. Build the Simulator
```bash
mkdir -p build && cd build
cmake ..
make
```

### 4. Run the demo
```bash
./os_simulator
./runner_demo
./memory_demo
./paging_demo
./filesys_demo
```
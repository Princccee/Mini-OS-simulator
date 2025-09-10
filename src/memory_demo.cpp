#include "memory_manager.h"
#include <iostream>

int main() {
    // 1000 units total memory, unit = 1
    MemoryManager mm(1000, 1);

    std::cout << "Initial state:\n";
    mm.printMemoryMap(60);

    int b1 = mm.allocate(101, 200, MemoryManager::FIRST_FIT);
    std::cout << "\nAllocated pid=101 size=200 -> block " << b1 << "\n";
    mm.printMemoryMap(60);

    int b2 = mm.allocate(102, 150, MemoryManager::BEST_FIT);
    std::cout << "\nAllocated pid=102 size=150 -> block " << b2 << "\n";
    mm.printMemoryMap(60);

    int b3 = mm.allocate(103, 100, MemoryManager::WORST_FIT);
    std::cout << "\nAllocated pid=103 size=100 -> block " << b3 << "\n";
    mm.printMemoryMap(60);

    std::cout << "\nFreeing block " << b2 << " (pid=102)\n";
    mm.freeByBlockId(b2);
    mm.printMemoryMap(60);

    std::cout << "\nAllocate pid=104 size=120 (FIRST_FIT)\n";
    int b4 = mm.allocate(104, 120, MemoryManager::FIRST_FIT);
    std::cout << "-> block " << b4 << "\n";
    mm.printMemoryMap(60);

    std::cout << "\nFree pid=101 and pid=103\n";
    mm.freeByPid(101);
    mm.freeByPid(103);
    mm.printMemoryMap(60);

    std::cout << "\nFinal: compact() (merge adjacent free blocks)\n";
    mm.compact();
    mm.printMemoryMap(60);

    return 0;
}

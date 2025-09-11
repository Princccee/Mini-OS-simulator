#include "memory_manager.h"
#include <iostream>

int main() {
    int total;
    std::cout << "Enter total memory size: ";
    std::cin >> total;

    MemoryManager mm(total);

    while (true) {
        std::cout << "\n1. Allocate\n2. Free\n3. Compact\n4. Show Memory\n5. Exit\nChoice: ";
        int choice;
        std::cin >> choice;

        if (choice == 1) {
            int pid, size, strat;
            std::cout << "Enter PID: ";
            std::cin >> pid;
            std::cout << "Enter size: ";
            std::cin >> size;
            std::cout << "Strategy (0=FirstFit, 1=BestFit, 2=WorstFit): ";
            std::cin >> strat;
            mm.allocate(pid, size, static_cast<MemoryManager::Strategy>(strat));
        } 
        else if (choice == 2) {
            int pid;
            std::cout << "Enter PID to free: ";
            std::cin >> pid;
            mm.freeByPid(pid);
        } 
        else if (choice == 3) {
            mm.compact();
        } 
        else if (choice == 4) {
            mm.printMemoryMap();
        } 
        else {
            break;
        }
    }
}


#include "paging.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>

int main() {
    try {
        int memSize, pageSize;
        std::cout << "Enter total physical memory size (bytes): ";
        if (!(std::cin >> memSize)) return 0;

        std::cout << "Enter page size (bytes): ";
        if (!(std::cin >> pageSize)) return 0;

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::cout << "Physical frames available: " << (memSize / pageSize) << "\n";

        // Input reference string
        std::cout << "Enter reference string (space-separated page numbers):\n";
        std::string line;
        std::getline(std::cin, line);

        if (line.empty()) {
            std::cout << "Reference string required.\nExample: 0 1 2 3 2 4 1 0 3 2\n";
            std::getline(std::cin, line);
        }

        std::istringstream iss(line);
        std::vector<int> refs;
        int val;
        while (iss >> val) refs.push_back(val);

        if (refs.empty()) {
            std::cerr << "No valid reference string entered. Exiting.\n";
            return 0;
        }

        // // Choose replacement policy
        // std::cout << "Choose replacement policy (0=FIFO, 1=LRU, 2=OPT): ";
        // int pol;
        // if (!(std::cin >> pol)) return 0;

        // PagingSimulator::Policy policy = PagingSimulator::FIFO;
        // if (pol == 1) policy = PagingSimulator::LRU;
        // else if (pol == 2) policy = PagingSimulator::OPT;

        // // Run simulation
        // PagingSimulator sim(memSize, pageSize, policy, refs);
        // sim.run();

        // Run all three policies
        std::cout << "\n=== Running all policies for comparison ===\n";

        std::cout << "\n>>> FIFO <<<\n";
        {
            PagingSimulator sim(memSize, pageSize, PagingSimulator::FIFO, refs);
            sim.run();
        }

        std::cout << "\n>>> LRU <<<\n";
        {
            PagingSimulator sim(memSize, pageSize, PagingSimulator::LRU, refs);
            sim.run();
        }

        std::cout << "\n>>> OPT <<<\n";
        {
            PagingSimulator sim(memSize, pageSize, PagingSimulator::OPT, refs);
            sim.run();
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return 0;
}

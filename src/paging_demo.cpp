#include "paging.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>

int main() {
    try {
        int phys, page;
        std::cout << "Enter total physical memory size (bytes): ";
        if (!(std::cin >> phys)) return 0;
        std::cout << "Enter page size (bytes): ";
        if (!(std::cin >> page)) return 0;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        PagingSimulator sim(phys, page);
        std::cout << "Physical frames available: " << sim.num_frames() << "\n";

        std::cout << "Enter reference string (space separated page numbers or virtual addresses):\n";
        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) {
            std::cout << "Reference string (single line) required. Example: 0 1 2 3 2 4 1 0 3 2\n";
            std::getline(std::cin, line);
        }

        // parse tokens as integers; treat them as page numbers already
        std::istringstream iss(line);
        std::vector<int> refs;
        int v;
        while (iss >> v) refs.push_back(v);

        // policy choose
        std::cout << "Choose replacement policy (0=FIFO, 1=LRU, 2=OPT): ";
        int pol; std::cin >> pol;
        ReplacementPolicy rp = ReplacementPolicy::FIFO;
        if (pol == 1) rp = ReplacementPolicy::LRU;
        else if (pol == 2) rp = ReplacementPolicy::OPT;

        sim.set_reference_string(refs);

        std::cout << "\n--- Simulation (policy=" << pol << ") ---\n";
        PagingStats stats = sim.run(rp, true);

        std::cout << "\n--- Final stats ---\n";
        std::cout << "Total references: " << stats.total_references << "\n";
        std::cout << "Page faults: " << stats.page_faults << "\n";
        std::cout << "Replacements: " << stats.replacements << "\n";
        std::cout << "Hit ratio: " << stats.hit_ratio() << "\n";
        std::cout << "Miss ratio: " << stats.miss_ratio() << "\n";
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return 0;
}

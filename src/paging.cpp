#include "paging.h"
#include <unordered_set>
#include <algorithm>

PagingSimulator::PagingSimulator(int memSize, int pageSize, Policy policy, const std::vector<int>& refs)
    : memSize(memSize), pageSize(pageSize), policy(policy), refs(refs) {
    numFrames = memSize / pageSize;
    frames.assign(numFrames, -1);
}

bool PagingSimulator::isPageInMemory(int page) {
    return std::find(frames.begin(), frames.end(), page) != frames.end();
}

void PagingSimulator::handleFIFO(int page, int idx) {
    if (isPageInMemory(page)) {
        std::cout << "Ref " << page << " -> HIT\n";
        return;
    }

    pageFaults++;

    // If there’s a free frame
    auto it = std::find(frames.begin(), frames.end(), -1);
    if (it != frames.end()) {
        *it = page;
        fifoQueue.push(page);
        std::cout << "Ref " << page << " -> PAGE FAULT. Loaded into free frame.\n";
        return;
    }

    // Replacement
    int victim = fifoQueue.front();
    fifoQueue.pop();

    int victimIndex = std::find(frames.begin(), frames.end(), victim) - frames.begin();
    frames[victimIndex] = page;
    fifoQueue.push(page);
    replacements++;

    std::cout << "Ref " << page << " -> PAGE FAULT. Evicted page " 
              << victim << " (FIFO), loaded page " << page << ".\n";
}

void PagingSimulator::handleLRU(int page, int idx) {
    if (isPageInMemory(page)) {
        std::cout << "Ref " << page << " -> HIT\n";
        return;
    }

    pageFaults++;

    // Free frame
    auto it = std::find(frames.begin(), frames.end(), -1);
    if (it != frames.end()) {
        *it = page;
        std::cout << "Ref " << page << " -> PAGE FAULT. Loaded into free frame.\n";
        return;
    }

    // Find least recently used
    int lruIndex = -1;
    int farthest = idx;
    for (int i = 0; i < numFrames; i++) {
        int lastUse = -1;
        for (int j = idx - 1; j >= 0; j--) {
            if (refs[j] == frames[i]) {
                lastUse = j;
                break;
            }
        }
        if (lastUse < farthest) {
            farthest = lastUse;
            lruIndex = i;
        }
    }

    int victim = frames[lruIndex];
    frames[lruIndex] = page;
    replacements++;

    std::cout << "Ref " << page << " -> PAGE FAULT. Evicted page " 
              << victim << " (LRU), loaded page " << page << ".\n";
}

void PagingSimulator::handleOPT(int page, int idx) {
    if (isPageInMemory(page)) {
        std::cout << "Ref " << page << " -> HIT\n";
        return;
    }

    pageFaults++;

    // Free frame
    auto it = std::find(frames.begin(), frames.end(), -1);
    if (it != frames.end()) {
        *it = page;
        std::cout << "Ref " << page << " -> PAGE FAULT. Loaded into free frame.\n";
        return;
    }

    // Find the page not needed for longest time
    int farthest = idx;
    int victimIndex = -1;
    for (int i = 0; i < numFrames; i++) {
        int nextUse = std::numeric_limits<int>::max();
        for (int j = idx + 1; j < (int)refs.size(); j++) {
            if (refs[j] == frames[i]) {
                nextUse = j;
                break;
            }
        }
        if (nextUse > farthest) {
            farthest = nextUse;
            victimIndex = i;
        }
    }

    int victim = frames[victimIndex];
    frames[victimIndex] = page;
    replacements++;

    std::cout << "Ref " << page << " -> PAGE FAULT. Evicted page " 
              << victim << " (OPT), loaded page " << page << ".\n";
}

void PagingSimulator::printStats() {
    int hits = refs.size() - pageFaults;
    std::cout << "\n--- Final stats ---\n";
    std::cout << "Total references: " << refs.size() << "\n";
    std::cout << "Page faults: " << pageFaults << "\n";
    std::cout << "Replacements: " << replacements << "\n";
    std::cout << "Hit ratio: " << (double)hits / refs.size() << "\n";
    std::cout << "Miss ratio: " << (double)pageFaults / refs.size() << "\n";
}

void PagingSimulator::run() {
    std::cout << "\n--- Simulation (policy=" << policy << ") ---\n";
    for (int i = 0; i < (int)refs.size(); i++) {
        int page = refs[i];
        if (policy == FIFO) handleFIFO(page, i);
        else if (policy == LRU) handleLRU(page, i);
        else handleOPT(page, i);
    }
    printStats();
}

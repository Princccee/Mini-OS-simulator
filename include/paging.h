#ifndef PAGING_H
#define PAGING_H

#include <vector>
#include <unordered_map>
#include <queue>
#include <limits>
#include <iostream>

class PagingSimulator {
public:
    enum Policy { FIFO, LRU, OPT };

    PagingSimulator(int memSize, int pageSize, Policy policy, const std::vector<int>& refs);

    void run();

private:
    int memSize;
    int pageSize;
    int numFrames;
    Policy policy;
    std::vector<int> refs;

    std::vector<int> frames; // holds page numbers in memory
    std::unordered_map<int, int> pageTable; // maps page -> frame index
    std::queue<int> fifoQueue; // for FIFO

    int pageFaults = 0;
    int replacements = 0;

    void handleFIFO(int page, int idx);
    void handleLRU(int page, int idx);
    void handleOPT(int page, int idx);

    bool isPageInMemory(int page);
    void printStats();
};

#endif

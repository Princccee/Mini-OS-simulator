#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vector>
#include <string>

struct Block {
    int id;         // unique block id
    int start;      // start offset (0..total_size-1)
    int size;       // size in units
    bool free;      // whether block is free
    int owner_pid;  // owner pid (-1 if free)
    int req_size;   // requested size by owner (for internal fragmentation)
};

class MemoryManager {
public:
    enum Strategy { FIRST_FIT=0, BEST_FIT=1, WORST_FIT=2 };

    MemoryManager(int total_size, int unit_size = 1);

    // allocate returns block id (>0) or -1 on failure
    int allocate(int pid, int req_size, Strategy s = FIRST_FIT);

    // free by block id or by pid (first matching)
    bool freeByBlockId(int blockId);
    bool freeByPid(int pid);

    // compact memory (merge free blocks - this implementation only merges neighbors on free)
    void compact(); // optional; currently merges adjacent free blocks (no relocation)

    // visualization and info
    void printMemoryMap(int width = 80) const; // textual scaled map
    void printBlockTable() const;              // human-readable block list

    // fragmentation stats
    int totalFree() const;
    int largestFreeBlock() const;
    double externalFragmentationRatio() const;
    int freeBlockCount() const;
    int totalSize() const { return total_size; }
    int unitSize() const { return unit; }
    int internalFragmentation() const; // sum(allocated_size - requested_size)

private:
    int total_size;
    int unit;
    int next_block_id;
    std::vector<Block> blocks; // sorted by start

    int roundUpToUnit(int sz) const;
    void tryMergeAroundIndex(size_t idx); // merges free neighbors around idx
};

#endif // MEMORY_MANAGER_H

#include "memory_manager.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <climits>

MemoryManager::MemoryManager(int total_size_, int unit_size_)
  : total_size(total_size_), unit(unit_size_), next_block_id(1)
{
    // start with one big free block
    blocks.push_back(Block{ next_block_id++, 0, total_size, true, -1, 0 });
}

int MemoryManager::roundUpToUnit(int sz) const {
    if (unit <= 1) return sz;
    return ((sz + unit - 1) / unit) * unit;
}

int MemoryManager::allocate(int pid, int req_size, Strategy s) {
    if (req_size <= 0) return -1;
    int allocSize = roundUpToUnit(req_size);

    // collect candidate indices
    std::vector<size_t> candidates;
    for (size_t i = 0; i < blocks.size(); ++i) {
        const Block &b = blocks[i];
        if (b.free && b.size >= allocSize) candidates.push_back(i);
    }
    if (candidates.empty()) return -1;

    size_t chosen_idx = candidates[0];
    if (s == FIRST_FIT) {
        // already first
    } else if (s == BEST_FIT) {
        int bestSize = INT_MAX;
        for (size_t idx : candidates) {
            if (blocks[idx].size < bestSize) {
                bestSize = blocks[idx].size;
                chosen_idx = idx;
            }
        }
    } else { // WORST_FIT
        int worstSize = -1;
        for (size_t idx : candidates) {
            if (blocks[idx].size > worstSize) {
                worstSize = blocks[idx].size;
                chosen_idx = idx;
            }
        }
    }

    Block chosen = blocks[chosen_idx];
    int newBlockId = next_block_id++;

    if (chosen.size == allocSize) {
        // exact fit
        blocks[chosen_idx].free = false;
        blocks[chosen_idx].owner_pid = pid;
        blocks[chosen_idx].req_size = req_size;
        return blocks[chosen_idx].id;
    } else {
        // split the block into allocated + remaining free
        Block allocBlock{ newBlockId, chosen.start, allocSize, false, pid, req_size };
        Block remBlock{ next_block_id++, chosen.start + allocSize, chosen.size - allocSize, true, -1, 0 };

        // replace chosen with allocBlock and insert remBlock after
        blocks[chosen_idx] = allocBlock;
        blocks.insert(blocks.begin() + chosen_idx + 1, remBlock);
        return allocBlock.id;
    }
}

bool MemoryManager::freeByBlockId(int blockId) {
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].id == blockId) {
            if (blocks[i].free) return false; // already free
            blocks[i].free = true;
            blocks[i].owner_pid = -1;
            blocks[i].req_size = 0;
            // try merge with neighbors
            tryMergeAroundIndex(i);
            return true;
        }
    }
    return false;
}

bool MemoryManager::freeByPid(int pid) {
    bool freed = false;
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (!blocks[i].free && blocks[i].owner_pid == pid) {
            blocks[i].free = true;
            blocks[i].owner_pid = -1;
            blocks[i].req_size = 0;
            tryMergeAroundIndex(i);
            freed = true;
            // continue to free any other blocks owned by same pid (if you support multiple)
        }
    }
    return freed;
}

void MemoryManager::tryMergeAroundIndex(size_t idx) {
    // merge with previous if free
    if (idx > 0 && blocks[idx].free && blocks[idx-1].free) {
        blocks[idx-1].size += blocks[idx].size;
        blocks.erase(blocks.begin() + idx);
        idx = idx - 1;
    }
    // merge with next if free
    if (idx + 1 < blocks.size() && blocks[idx].free && blocks[idx+1].free) {
        blocks[idx].size += blocks[idx+1].size;
        blocks.erase(blocks.begin() + idx + 1);
    }
}

void MemoryManager::compact() {
    // simple compaction: merge all free blocks into one at the end
    // Note: this does not relocate allocated blocks in this simulation
    // because we don't simulate relocation; we only merge adjacent free blocks.
    // For true compaction with relocation, you'd move allocated blocks and update start offsets.
    for (size_t i = 0; i + 1 < blocks.size();) {
        if (blocks[i].free && blocks[i+1].free) {
            blocks[i].size += blocks[i+1].size;
            blocks.erase(blocks.begin() + i + 1);
        } else {
            ++i;
        }
    }
}

void MemoryManager::printBlockTable() const {
    std::cout << "\nBlock Table:\n";
    std::cout << std::left << std::setw(6) << "ID" << std::setw(8) << "Start"
              << std::setw(8) << "Size" << std::setw(8) << "Free"
              << std::setw(8) << "PID" << std::setw(8) << "ReqSize" << "\n";
    for (const auto &b : blocks) {
        std::cout << std::setw(6) << b.id
                  << std::setw(8) << b.start
                  << std::setw(8) << b.size
                  << std::setw(8) << (b.free ? "Y":"N")
                  << std::setw(8) << (b.owner_pid == -1 ? "-" : std::to_string(b.owner_pid))
                  << std::setw(8) << (b.req_size==0 ? "-" : std::to_string(b.req_size))
                  << "\n";
    }
    std::cout << std::flush;
}

void MemoryManager::printMemoryMap(int width) const {
    if (width < 20) width = 20;
    std::string map;
    map.reserve(width);
    double scale = (double)total_size / width;

    for (int col = 0; col < width; ++col) {
        // corresponding memory offset
        int offset = (int)(col * scale);
        // find block containing offset
        const Block* cur = nullptr;
        for (const auto &b : blocks) {
            if (offset >= b.start && offset < b.start + b.size) {
                cur = &b;
                break;
            }
        }
        if (!cur) { map.push_back('?'); continue; }
        if (cur->free) map.push_back('.');
        else map.push_back('#');
    }

    // print map and legend
    std::cout << "\nMemory Map (width=" << width << "):\n";
    std::cout << map << "\n";
    std::cout << "Legend: '#' = allocated  '.' = free\n";
    printBlockTable();

    // fragmentation stats
    std::cout << "\nFragmentation:\n";
    std::cout << " Total memory: " << total_size << "\n";
    std::cout << " Total free: " << totalFree() << "\n";
    std::cout << " Largest free block: " << largestFreeBlock() << "\n";
    std::cout << " Free blocks: " << freeBlockCount() << "\n";
    std::cout << " External fragmentation ratio: " << std::fixed << std::setprecision(3)
              << externalFragmentationRatio() << "\n";
    std::cout << " Internal fragmentation (sum): " << internalFragmentation() << "\n";
}

int MemoryManager::totalFree() const {
    int total = 0;
    for (const auto &b : blocks) if (b.free) total += b.size;
    return total;
}

int MemoryManager::largestFreeBlock() const {
    int best = 0;
    for (const auto &b : blocks) if (b.free && b.size > best) best = b.size;
    return best;
}

double MemoryManager::externalFragmentationRatio() const {
    int total = totalFree();
    if (total <= 0) return 0.0;
    int largest = largestFreeBlock();
    return 1.0 - ((double)largest / (double)total);
}

int MemoryManager::freeBlockCount() const {
    int cnt = 0;
    for (const auto &b : blocks) if (b.free) ++cnt;
    return cnt;
}

int MemoryManager::internalFragmentation() const {
    int sum = 0;
    for (const auto &b : blocks) {
        if (!b.free && b.req_size > 0) sum += (b.size - b.req_size);
    }
    return sum;
}

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

int MemoryManager::allocate(int pid, int size, Strategy strategy) {
    if (size <= 0) {
        std::cout << "Invalid request size.\n";
        return -1;
    }

    // Check if PID already exists (owner_pid)
    for (const auto &blk : blocks) {
        if (!blk.free && blk.owner_pid == pid) {
            std::cout << "Allocation failed: PID " << pid << " already exists.\n";
            return -1;
        }
    }

    int index = -1;

    if (strategy == Strategy::FIRST_FIT) {
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i].free && blocks[i].size >= size) { index = (int)i; break; }
        }
    } else if (strategy == Strategy::BEST_FIT) {
        int bestSize = INT_MAX;
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i].free && blocks[i].size >= size && blocks[i].size < bestSize) {
                bestSize = blocks[i].size;
                index = (int)i;
            }
        }
    } else { // WORST_FIT
        int worstSize = -1;
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i].free && blocks[i].size >= size && blocks[i].size > worstSize) {
                worstSize = blocks[i].size;
                index = (int)i;
            }
        }
    }

    if (index == -1) {
        std::cout << "Allocation failed: No suitable block found for PID "
                  << pid << " (size " << size << ").\n";
        return -1;
    }

    // If splitting required: keep current block's id for allocated part,
    // create a new block id for the remaining free block.
    if (blocks[index].size > size) {
        // allocated part: reuse block id
        blocks[index].req_size = size;
        blocks[index].owner_pid = pid;
        blocks[index].free = false;
        // size remains to be set to requested
        int original_start = blocks[index].start;
        int original_size = blocks[index].size;

        blocks[index].size = size;
        // start remains original_start

        // remaining free block
        Block rem;
        rem.id = next_block_id++;
        rem.start = original_start + size;
        rem.size = original_size - size;
        rem.free = true;
        rem.owner_pid = -1;
        rem.req_size = 0;

        // insert remainder after current index
        blocks.insert(blocks.begin() + index + 1, rem);
    } else {
        // exact fit: mark owner fields
        blocks[index].owner_pid = pid;
        blocks[index].req_size = size;
        blocks[index].free = false;
    }

    std::cout << "Allocated PID " << pid << " at address " << blocks[index].start
              << " (size " << blocks[index].size << ").\n";
    return blocks[index].start;
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
            freed = true;
            // merge neighbors; tryMergeAroundIndex updates blocks vector
            tryMergeAroundIndex(i);
            // after merging the current index may have changed/moved,
            // but tryMergeAroundIndex merges correctly for this index
        }
    }

    if (freed) std::cout << "Freed memory for PID " << pid << ".\n";
    else std::cout << "Free failed: PID " << pid << " not found.\n";

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

// void MemoryManager::compact() {
//     // simple compaction: merge all free blocks into one at the end
//     // Note: this does not relocate allocated blocks in this simulation
//     // because we don't simulate relocation; we only merge adjacent free blocks.
//     // For true compaction with relocation, you'd move allocated blocks and update start offsets.
//     for (size_t i = 0; i + 1 < blocks.size();) {
//         if (blocks[i].free && blocks[i+1].free) {
//             blocks[i].size += blocks[i+1].size;
//             blocks.erase(blocks.begin() + i + 1);
//         } else {
//             ++i;
//         }
//     }
// }

void MemoryManager::compact() {
    int currentPos = 0;
    std::vector<Block> newBlocks;

    // Slide all allocated blocks left
    for (auto &block : blocks) {
        if (!block.free) {
            Block newBlock = block;
            newBlock.start = currentPos;
            newBlocks.push_back(newBlock);
            currentPos += block.size;
        }
    }

    // Add one free block with remaining memory
    int freeSize = total_size - currentPos;
    if (freeSize > 0) {
        Block freeBlock;
        freeBlock.id = next_block_id++;
        freeBlock.start = currentPos;
        freeBlock.size = freeSize;
        freeBlock.free = true;
        freeBlock.owner_pid = -1;
        freeBlock.req_size = 0;
        newBlocks.push_back(freeBlock);
    }

    blocks = newBlocks;
    std::cout << "Memory compacted.\n";
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

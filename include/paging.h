#ifndef PAGING_H
#define PAGING_H

#include <vector>
#include <string>

enum class ReplacementPolicy {
    FIFO = 0,
    LRU  = 1,
    OPT  = 2
};

struct PageTableEntry {
    int frame;      // frame number if present, -1 if not present
    bool valid;     // present bit
    int last_used;  // timestamp for LRU (if used)
    PageTableEntry() : frame(-1), valid(false), last_used(-1) {}
};

struct PagingStats {
    int total_references = 0;
    int page_faults = 0;
    int replacements = 0;
    double hit_ratio() const {
        if (total_references == 0) return 0.0;
        return double(total_references - page_faults) / double(total_references);
    }
    double miss_ratio() const {
        if (total_references == 0) return 0.0;
        return double(page_faults) / double(total_references);
    }
};

class PagingSimulator {
public:
    // total_phys_bytes: physical memory size in bytes
    // page_size_bytes: size of one page / frame in bytes
    PagingSimulator(int total_phys_bytes, int page_size_bytes);

    // set reference string (virtual addresses). The driver will convert to page numbers.
    void set_reference_string(const std::vector<int>& virtual_page_refs);

    // Run the simulation for given replacement policy. If verbose=true prints step-by-step.
    PagingStats run(ReplacementPolicy policy, bool verbose = true);

    // helpers
    int page_size() const { return page_size_bytes; }
    int num_frames() const { return frames_count; }

private:
    int phys_size_bytes;
    int page_size_bytes;
    int frames_count;

    std::vector<int> refs; // sequence of virtual page numbers

    // internal helpers for policies
    PagingStats run_fifo(bool verbose);
    PagingStats run_lru(bool verbose);
    PagingStats run_opt(bool verbose);

    // disable copying unintendedly
    PagingSimulator(const PagingSimulator&) = delete;
    PagingSimulator& operator=(const PagingSimulator&) = delete;
};

#endif // PAGING_H

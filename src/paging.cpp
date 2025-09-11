#include "paging.h"

#include <iostream>
#include <queue>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <limits>
#include <sstream>

PagingSimulator::PagingSimulator(int total_phys_bytes, int page_size_bytes_)
  : phys_size_bytes(total_phys_bytes),
    page_size_bytes(page_size_bytes_)
{
    if (page_size_bytes <= 0) throw std::runtime_error("page_size must be > 0");
    frames_count = phys_size_bytes / page_size_bytes;
    if (frames_count <= 0) throw std::runtime_error("physical size smaller than page size -> 0 frames");
}

void PagingSimulator::set_reference_string(const std::vector<int>& virtual_page_refs) {
    refs = virtual_page_refs;
}

PagingStats PagingSimulator::run(ReplacementPolicy policy, bool verbose) {
    switch (policy) {
        case ReplacementPolicy::FIFO: return run_fifo(verbose);
        case ReplacementPolicy::LRU:  return run_lru(verbose);
        case ReplacementPolicy::OPT:  return run_opt(verbose);
    }
    return PagingStats();
}

// FIFO implementation
PagingStats PagingSimulator::run_fifo(bool verbose) {
    PagingStats stats;
    if (frames_count == 0) return stats;

    std::unordered_map<int,int> page_to_frame; // page -> frame index
    std::queue<int> fifo_queue;                // pages in frame order
    std::vector<int> frame_to_page(frames_count, -1);

    int next_frame = 0;

    for (size_t t = 0; t < refs.size(); ++t) {
        int page = refs[t];
        stats.total_references++;

        if (page_to_frame.find(page) != page_to_frame.end()) {
            if (verbose) std::cout << "Ref " << page << " -> HIT (frame " << page_to_frame[page] << ")\n";
            continue;
        }

        // page fault
        stats.page_faults++;
        if (verbose) std::cout << "Ref " << page << " -> PAGE FAULT. ";

        // if free frame available
        if ((int)page_to_frame.size() < frames_count) {
            // allocate next free frame (simple)
            int frame = next_frame++;
            frame_to_page[frame] = page;
            page_to_frame[page] = frame;
            fifo_queue.push(page);
            if (verbose) std::cout << "Loaded into free frame " << frame << "\n";
        } else {
            // need to evict FIFO
            int victim_page = fifo_queue.front(); fifo_queue.pop();
            int victim_frame = page_to_frame[victim_page];
            page_to_frame.erase(victim_page);

            // place new page
            frame_to_page[victim_frame] = page;
            page_to_frame[page] = victim_frame;
            fifo_queue.push(page);
            stats.replacements++;
            if (verbose) std::cout << "Evicted page " << victim_page << " from frame " << victim_frame
                                 << ", loaded page " << page << " into frame " << victim_frame << "\n";
        }
    }

    return stats;
}

// LRU implementation (using list + map of iterators)
PagingStats PagingSimulator::run_lru(bool verbose) {
    PagingStats stats;
    if (frames_count == 0) return stats;

    std::unordered_map<int,int> page_to_frame;
    std::vector<int> frame_to_page(frames_count, -1);

    std::list<int> lru_list; // front = most recently used, back = least recently used
    std::unordered_map<int, std::list<int>::iterator> page_to_iter;

    int frames_used = 0;

    for (size_t t = 0; t < refs.size(); ++t) {
        int page = refs[t];
        stats.total_references++;

        auto it = page_to_frame.find(page);
        if (it != page_to_frame.end()) {
            // hit: move to front (most recently used)
            if (verbose) std::cout << "Ref " << page << " -> HIT (frame " << it->second << ")\n";
            // move node to front
            lru_list.erase(page_to_iter[page]);
            lru_list.push_front(page);
            page_to_iter[page] = lru_list.begin();
            continue;
        }

        // miss / page fault
        stats.page_faults++;
        if (verbose) std::cout << "Ref " << page << " -> PAGE FAULT. ";

        if (frames_used < frames_count) {
            int frame = frames_used++;
            frame_to_page[frame] = page;
            page_to_frame[page] = frame;
            lru_list.push_front(page);
            page_to_iter[page] = lru_list.begin();
            if (verbose) std::cout << "Loaded into free frame " << frame << "\n";
        } else {
            // evict LRU -> back()
            int victim_page = lru_list.back();
            lru_list.pop_back();
            int victim_frame = page_to_frame[victim_page];
            page_to_frame.erase(victim_page);
            page_to_iter.erase(victim_page);

            // replace
            frame_to_page[victim_frame] = page;
            page_to_frame[page] = victim_frame;
            lru_list.push_front(page);
            page_to_iter[page] = lru_list.begin();
            stats.replacements++;
            if (verbose) std::cout << "Evicted page " << victim_page << " from frame " << victim_frame
                                 << ", loaded page " << page << " into frame " << victim_frame << "\n";
        }
    }

    return stats;
}

// OPT (Belady) implementation
// Needs entire reference string to compute "next use" for each position.
// We'll precompute for each position the next index a page will be used (or inf if not used).
PagingStats PagingSimulator::run_opt(bool verbose) {
    PagingStats stats;
    if (frames_count == 0) return stats;

    int n = (int)refs.size();

    // next_use[i] = next index after i where refs[i] occurs; we'll compute for all pages via last-seen map from right to left
    std::unordered_map<int,int> next_pos; // page -> next position index (from right)
    std::vector<int> next_use(n, std::numeric_limits<int>::max());

    for (int i = n-1; i >= 0; --i) {
        int p = refs[i];
        if (next_pos.find(p) != next_pos.end()) next_use[i] = next_pos[p];
        else next_use[i] = std::numeric_limits<int>::max(); // not used again
        next_pos[p] = i;
    }

    // frames state
    std::unordered_map<int,int> page_to_frame;
    std::vector<int> frame_to_page(frames_count, -1);
    std::vector<int> frame_nextuse(frames_count, std::numeric_limits<int>::max());
    int frames_used = 0;

    for (int i = 0; i < n; ++i) {
        int page = refs[i];
        stats.total_references++;

        if (page_to_frame.find(page) != page_to_frame.end()) {
            int fr = page_to_frame[page];
            frame_nextuse[fr] = next_use[i]; // update next use for that frame
            if (verbose) std::cout << "Ref " << page << " -> HIT (frame " << fr << ")\n";
            continue;
        }

        // page fault
        stats.page_faults++;
        if (verbose) std::cout << "Ref " << page << " -> PAGE FAULT. ";

        if (frames_used < frames_count) {
            int fr = frames_used++;
            frame_to_page[fr] = page;
            page_to_frame[page] = fr;
            frame_nextuse[fr] = next_use[i];
            if (verbose) std::cout << "Loaded into free frame " << fr << "\n";
        } else {
            // pick victim: the page whose next use is farthest in future (max next_use)
            int victim_frame = -1;
            int farthest = -1;
            for (int fr = 0; fr < frames_count; ++fr) {
                if (frame_nextuse[fr] == std::numeric_limits<int>::max()) {
                    // this page is never used again -- best victim
                    victim_frame = fr;
                    break;
                }
                if (frame_nextuse[fr] > farthest) {
                    farthest = frame_nextuse[fr];
                    victim_frame = fr;
                }
            }
            int victim_page = frame_to_page[victim_frame];
            page_to_frame.erase(victim_page);

            // replace
            frame_to_page[victim_frame] = page;
            page_to_frame[page] = victim_frame;
            frame_nextuse[victim_frame] = next_use[i];

            stats.replacements++;
            if (verbose) std::cout << "Evicted page " << victim_page << " from frame " << victim_frame
                                 << ", loaded page " << page << " into frame " << victim_frame << "\n";
        }
    }

    return stats;
}

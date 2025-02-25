#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define L1_ICACHE_SETS 16384
#define L1_DCACHE_SETS 16384
#define L1_ICACHE_WAYS 4
#define L1_DCACHE_WAYS 8
#define CACHE_LINE_SIZE 64

// MESI protocol states
typedef enum { INVALID, EXCLUSIVE, SHARED, MODIFIED } MESI_State;

// Structure representing a cache line
typedef struct {
    uint32_t tag;       // Tag for address mapping
    MESI_State state;   // MESI state
    bool valid;         // Valid bit
    int lru_counter;    // Counter for LRU replacement
} CacheLine;

// Structure representing an instruction cache set
typedef struct {
    CacheLine lines[L1_ICACHE_WAYS];
} ICacheSet;

// Structure representing a data cache set
typedef struct {
    CacheLine lines[L1_DCACHE_WAYS];
} DCacheSet;

// L1 instruction and data caches
ICacheSet icache[L1_ICACHE_SETS];
DCacheSet dcache[L1_DCACHE_SETS];

// Cache statistics
uint32_t cache_reads = 0, cache_writes = 0, cache_hits = 0, cache_misses = 0;

// Function to initialize caches
void initialize_cache() {
    for (int i = 0; i < L1_ICACHE_SETS; i++) {
        for (int j = 0; j < L1_ICACHE_WAYS; j++) {
            icache[i].lines[j].valid = false;
            icache[i].lines[j].state = INVALID;
            icache[i].lines[j].lru_counter = j;
        }
    }
    for (int i = 0; i < L1_DCACHE_SETS; i++) {
        for (int j = 0; j < L1_DCACHE_WAYS; j++) {
            dcache[i].lines[j].valid = false;
            dcache[i].lines[j].state = INVALID;
            dcache[i].lines[j].lru_counter = j;
        }
    }
}

// Function to update the LRU counters upon access
void update_lru(CacheLine *lines, int ways, int accessed_index) {
    int accessed_lru = lines[accessed_index].lru_counter;
    for (int i = 0; i < ways; i++) {
        if (lines[i].lru_counter < accessed_lru) {
            lines[i].lru_counter++;
        }
    }
    lines[accessed_index].lru_counter = 0;
}

// Function to print cache statistics
void print_statistics() {
    printf("Cache Reads: %u\n", cache_reads);
    printf("Cache Writes: %u\n", cache_writes);
    printf("Cache Hits: %u\n", cache_hits);
    printf("Cache Misses: %u\n", cache_misses);
    printf("Cache Hit Ratio: %.2f%%\n", (cache_hits * 100.0) / (cache_hits + cache_misses));
}


// Function to access the cache (read/write operations)
void access_cache(uint32_t address, int operation) {
    printf("You have given me case: %d\n", operation);

    bool is_write = (operation == 1);
    bool is_instruction = (operation == 2);
    
    uint32_t index = (address / CACHE_LINE_SIZE) % (is_instruction ? L1_ICACHE_SETS : L1_DCACHE_SETS);
    uint32_t tag = address / (CACHE_LINE_SIZE * (is_instruction ? L1_ICACHE_SETS : L1_DCACHE_SETS));
    
    CacheLine *lines = is_instruction ? icache[index].lines : dcache[index].lines;
    int ways = is_instruction ? L1_ICACHE_WAYS : L1_DCACHE_WAYS;
    
    if (operation != 9) {
      cache_reads += !is_write;
      cache_writes += is_write;
    }

    for (int i = 0; i < ways; i++) {
        if (lines[i].valid && lines[i].tag == tag) {
            if (operation != 9) {
              cache_hits++;
            }
            if (is_write) {
                lines[i].state = MODIFIED;
            }
            update_lru(lines, ways, i);
            if (operation != 9 || operation != 3 || operation != 4 || operation != 8) {
                return;
                // break;
            }
        }
    }
    
    if (operation != 9) {
      cache_misses++;
    } 
    // Finding the LRU cache line for replacement
    int lru_index = 0;
    for (int i = 0; i < ways; i++) {
        if (!lines[i].valid) {
            lru_index = i;
            break;
        }
        if (lines[i].lru_counter == ways - 1) {
            lru_index = i;
        }
    }
    
    // Write back if the line is modified before eviction
    if (lines[lru_index].valid && lines[lru_index].state == MODIFIED) {
        printf("Write to L2 %08X\n", address);
    }
    
    // Handle special operations
    switch (operation) {
        case 0: case 1: case 2:
            printf("Read from L2 %08X\n", address);
            lines[lru_index].tag = tag;
            lines[lru_index].valid = true;
            lines[lru_index].state = is_write ? MODIFIED : SHARED;
            update_lru(lines, ways, lru_index);
            break;
        case 3:
            for (int i = 0; i < L1_DCACHE_WAYS; i++) {
                if (dcache[index].lines[i].valid && dcache[index].lines[i].tag == tag) {
                    dcache[index].lines[i].valid = false;
                    printf("Invalidate command from L2 %08X\n", address);
                    fflush(stdout);
                    return;
                }
            }
            break;
        case 4:
            for (int i = 0; i < L1_DCACHE_WAYS; i++) {
                if (dcache[index].lines[i].valid && dcache[index].lines[i].tag == tag) {
                    printf("Data request from L2 %08X - Data Found\n", address);
                    fflush(stdout);
                    return;
                }
            }
            printf("Data request from L2 %08X - Data Not Found\n", address);
            fflush(stdout);
            break;
        // case 3: printf("Invalidate command from L2 %08X\n", address); break;
        // case 4: printf("Data request from L2 %08X\n", address); fflush(stdout); break;
        case 8: initialize_cache(); printf("Cache cleared and reset\n"); return;
        case 9: print_statistics(); fflush(stdout); break;
    }
}
// Function to process trace file and simulate cache behavior
void process_trace_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening trace file");
        return;
    }
    
    int operation;
    uint32_t address;
    while (fscanf(file, "%d %x", &operation, &address) == 2) {
        access_cache(address, operation);
    }
    fclose(file);
}

// Main function to initialize and run the simulation
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <trace_file>\n", argv[0]);
        return 1;
    }
    initialize_cache();
    process_trace_file(argv[1]);
    return 0;
}

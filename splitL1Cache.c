/*
ECE 585 Final Project

Amey, Anthony, Belaal, Eva, Sarath

3/8/25

*/

#include "cache.h"

// L1 instruction and data caches
ICacheSet icache[L1_CACHE_SETS];
DCacheSet dcache[L1_CACHE_SETS];

// Cache statistics
uint32_t cache_reads = 0, cache_writes = 0, cache_hits = 0, cache_misses = 0;

// Function to initialize caches
void initialize_cache()
{
    for (int i = 0; i < L1_CACHE_SETS; i++)
    {
        for (int j = 0; j < L1_ICACHE_WAYS; j++)
        {
            icache[i].lines[j].valid = false;
            icache[i].lines[j].state = INVALID;
            icache[i].lines[j].lru_counter = j;
        }
    }
    for (int i = 0; i < L1_CACHE_SETS; i++)
    {
        for (int j = 0; j < L1_DCACHE_WAYS; j++)
        {
            dcache[i].lines[j].valid = false;
            dcache[i].lines[j].state = INVALID;
            dcache[i].lines[j].lru_counter = j;
        }
    }
}

void print_cache_index(bool whichCache, uint32_t index)
{
    if (!whichCache)
    {
        printf("D-Cache Data - \n");
        DCacheSet tmp = dcache[index];
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            printf("Way: %d\n", i);
            printf("Valid: %b\t", tmp.lines[i].valid);
            printf("Tag: %03x\t", tmp.lines[i].tag);
            printf("lru_counter: %d\t", tmp.lines[i].lru_counter);
            printf("MESI State: %s\t", MESI_State_strings[tmp.lines[i].state]);
            printf("\n");
        }
    }
    else
    {
        printf("I-Cache Data - \n");
        ICacheSet tmp = icache[index];
        for (int i = 0; i < L1_ICACHE_WAYS; i++)
        {
            printf("Way: %d\n", i);
            printf("Valid: %b\t", tmp.lines[i].valid);
            printf("Tag: %03x\t", tmp.lines[i].tag);
            printf("lru_counter: %d\t", tmp.lines[i].lru_counter);
            printf("MESI State: %s\t", MESI_State_strings[tmp.lines[i].state]);
            printf("\n");
        }
    }
}

void update_lru(bool is_i_or_d, int way, uint32_t index)
{
    CacheLine *linePtr = is_i_or_d ? &icache[index].lines[way] : &dcache[index].lines[way];
    int cache_ways = is_i_or_d ? L1_ICACHE_WAYS : L1_DCACHE_WAYS;

    int accessed_lru_cnt = dcache[index].lines[way].lru_counter;
    for (int i = 0; i < cache_ways; i++)
    {
        if (linePtr->lru_counter > accessed_lru_cnt)
        {
            linePtr->lru_counter--;
        }
    }

    linePtr->lru_counter = cache_ways - 1;
}

void update_MESI(bool is_i_or_d, int way, uint32_t index, bool r_or_w)
{
    // Function to update MESI state
    CacheLine *linePtr = is_i_or_d ? &icache[index].lines[way] : &dcache[index].lines[way];
    switch (linePtr->state)
    {
    case MODIFIED:
        break;
    case EXCLUSIVE:
        if (r_or_w)
        {
            // This is read
            linePtr->state = EXCLUSIVE;
        }
        else
        {
            // This is write
            linePtr->state = MODIFIED;
        }
        break;
    case SHARED:
        break;
    case INVALID:
        if (r_or_w)
        {
            // This is read
            linePtr->state = EXCLUSIVE;
        }
        else
        {
            // This is write
            linePtr->state = MODIFIED;
        }
        break;
    default:
        break;
    }
}

bool write_d_cache(uint32_t index, uint32_t tag)
{
    // For handling Case 2
    bool isHit = false;
    for (int i = 0; i < L1_DCACHE_WAYS; i++)
    {
        // check if valid bit is 1
        if (dcache[index].lines[i].valid)
        {
            // Valid bit is set, now compare tags
            if (dcache[index].lines[i].tag == tag)
            {
                // Tag bit also matches, this means we have a Hit
                isHit = true;
                update_MESI(false, i, index, false);
                update_lru(false, i, index);
                break;
            }
        }
    }
    if (!isHit)
    {
        // Handle the miss
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            // Find the way which has lru_counter set to 0 i.e. it is LRU
            if (dcache[index].lines[i].lru_counter == 0)
            {
                dcache[index].lines[i].valid = true;
                dcache[index].lines[i].tag = tag;
                update_MESI(false, i, index, false);
                update_lru(false, i, index);
                break;
            }
        }
    }
    return isHit;
}

bool read_cache(bool is_i_or_d, uint32_t index, uint32_t tag)
{
    bool isHit = false;
    if (~is_i_or_d)
    {
        // This is D-Cache
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            // check if valid bit is 1
            if (dcache[index].lines[i].valid)
            {
                // Valid bit is set, now compare tags
                if (dcache[index].lines[i].tag == tag)
                {
                    // Tag bit also matches, this means we have a Hit
                    isHit = true;
                    update_MESI(is_i_or_d, i, index, true);
                    update_lru(is_i_or_d, i, index);
                    break;
                }
            }
        }
        if (!isHit)
        {
            // Handle the miss
            for (int i = 0; i < L1_DCACHE_WAYS; i++)
            {
                // Find the way which has lru_counter set to 0 i.e. it is LRU
                if (dcache[index].lines[i].lru_counter == 0)
                {
                    dcache[index].lines[i].valid = true;
                    dcache[index].lines[i].tag = tag;
                    update_MESI(is_i_or_d, i, index, true);
                    update_lru(is_i_or_d, i, index);
                    break;
                }
            }
        }
    }
    else
    {
        // This is I-Cache
        for (int i = 0; i < L1_ICACHE_WAYS; i++)
        {
            // check if valid bit is 1
            if (icache[index].lines[i].valid)
            {
                // Valid bit is set, now compare tags
                if (icache[index].lines[i].tag == tag)
                {
                    // Tag bit also matches, this means we have a Hit
                    isHit = true;
                    update_lru(is_i_or_d, i, index);
                    break;
                }
            }
        }
        if (!isHit)
        {
            // Handle the miss
            for (int i = 0; i < L1_ICACHE_WAYS; i++)
            {
                // Find the way which has lru_counter set to 0 i.e. it is LRU
                if (icache[index].lines[i].lru_counter == 0)
                {
                    icache[index].lines[i].valid = true;
                    icache[index].lines[i].tag = tag;
                    icache[index].lines[i].state = EXCLUSIVE;
                    update_lru(is_i_or_d, i, index);
                    break;
                }
            }
        }
    }
    return isHit;
}

// Function to access the cache (read/write operations)
void access_cache(uint32_t address, int operation)
{
    printf("\nYou have given me case: %d\n", operation);

    bool is_write = (operation == 1);
    bool is_i_or_d = (operation == 2);

    uint32_t index = (address / CACHE_LINE_SIZE) % L1_CACHE_SETS;
    uint32_t tag = address / (CACHE_LINE_SIZE * L1_CACHE_SETS);

    printf("Given Addr: %x\n", address);
    printf("Index: %x\n", index);
    printf("Tag: %x\n", tag);

    switch (operation)
    {
    case 0:
    case 2:
        cache_reads++;
        if (read_cache(is_i_or_d, index, tag))
        {
            cache_hits++;
        }
        else
        {
            cache_misses++;
        }
        break;
    case 1:
        cache_writes++;
        if (write_d_cache(index, tag))
        {
            cache_hits++;
        }
        else
        {
            cache_misses++;
        }
        break;
    case 9:
        print_cache_index(is_i_or_d, index);
        break;
    default:
        break;
    }

    print_cache_index(is_i_or_d, index);
}

// Function to process trace file and simulate cache behavior
void process_trace_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening trace file");
        return;
    }

    int operation;
    uint32_t address;
    while (fscanf(file, "%d %x", &operation, &address) == 2)
    {
        access_cache(address, operation);
    }
    fclose(file);
}

void print_stats()
{
    printf("\nFinal Stats - \n");
    printf("Cache Reads: %u\n", cache_reads);
    printf("Cache Writes: %u\n", cache_writes);
    printf("Cache Hits: %u\n", cache_hits);
    printf("Cache Misses: %u\n", cache_misses);
    printf("Cache Hit Ratio: %.2f%%\n", (cache_hits * 100.0) / (cache_hits + cache_misses));
}

// Main function to initialize and run the simulation
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <trace_file>\n", argv[0]);
        return 1;
    }
    initialize_cache();
    process_trace_file(argv[1]);
    print_stats();
    return 0;
}

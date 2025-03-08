/*
ECE 585 Final Project

Amey, Anthony, Belaal A, Eva V, Sarath

03/08/2025

*/

#include "cache.h"

// L1 instruction and data caches
ICacheSet icache[L1_CACHE_SETS];
DCacheSet dcache[L1_CACHE_SETS];

// Cache statistics
uint32_t cache_reads = 0, cache_writes = 0, cache_hits = 0, cache_misses = 0;
bool debug_mode = false;
int operation;
uint32_t address;

// Function to initialize caches
void initialize_cache()
{
    cache_reads = 0;
    cache_writes = 0;
    cache_hits = 0;
    cache_misses = 0;
    for (int i = 0; i < L1_CACHE_SETS; i++)
    {
        for (int j = 0; j < L1_ICACHE_WAYS; j++)
        {
            icache[i].lines[j].valid = false;
            icache[i].lines[j].state = INVALID;
            icache[i].lines[j].tag = 000;
            icache[i].lines[j].lru_counter = j;
        }
    }
    for (int i = 0; i < L1_CACHE_SETS; i++)
    {
        for (int j = 0; j < L1_DCACHE_WAYS; j++)
        {
            dcache[i].lines[j].valid = false;
            dcache[i].lines[j].state = INVALID;
            dcache[i].lines[j].tag = 000;
            dcache[i].lines[j].lru_counter = j;
        }
    }
}

void print_cache_index(bool whichCache, uint32_t index)
{

        printf("\n\nD-Cache Data - \n");
        DCacheSet tmp = dcache[index];
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            printf("Way: %d\n", i);
            printf("Valid: %s\t", tmp.lines[i].valid ? "true":"false");
            printf("Tag: %03x\t", tmp.lines[i].tag);
            printf("lru_counter: %d\t", tmp.lines[i].lru_counter);
            printf("MESI State: %s\t", MESI_State_strings[tmp.lines[i].state]);
            printf("\n");
        }

        printf("\n\nI-Cache Data - \n");
        ICacheSet tmp1 = icache[index];
        for (int i = 0; i < L1_ICACHE_WAYS; i++)
        {
            printf("Way: %d\n", i);
            printf("Valid: %s\t", tmp1.lines[i].valid ? "true":"false");
            printf("Tag: %03x\t", tmp1.lines[i].tag);
            printf("lru_counter: %d\t", tmp1.lines[i].lru_counter);
            printf("MESI State: %s\t", MESI_State_strings[tmp1.lines[i].state]);
            printf("\n");
        }
}

void update_lru(bool is_i_or_d, int way, uint32_t index)
{
    if (!is_i_or_d)
    {
        // This is D-Cache
        int accessed_lru_cnt = dcache[index].lines[way].lru_counter;
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            if (dcache[index].lines[i].lru_counter > accessed_lru_cnt)
            {
                dcache[index].lines[i].lru_counter--;
            }
        }
        dcache[index].lines[way].lru_counter = L1_DCACHE_WAYS - 1;
    }
    else
    {
        // This is I-Cache
        int accessed_lru_cnt = icache[index].lines[way].lru_counter;
        for (int i = 0; i < L1_ICACHE_WAYS; i++)
        {
            if (icache[index].lines[i].lru_counter > accessed_lru_cnt)
            {
                icache[index].lines[i].lru_counter--;
            }
        }
        icache[index].lines[way].lru_counter = L1_ICACHE_WAYS - 1;
    }
}

void update_MESI(bool is_i_or_d, int way, uint32_t index, bool r_or_w)
{
    // Function to update MESI state
    CacheLine *linePtr = is_i_or_d ? &icache[index].lines[way] : &dcache[index].lines[way];
    switch (linePtr->state)
    {
    case MODIFIED:
        if (r_or_w)
        {
            // This is read
            linePtr->state = MODIFIED;
        }
        else
        {
            // This is write
            linePtr->state = MODIFIED;
        }
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
        if (r_or_w)
        {
            // This is read
            linePtr->state = SHARED;
        }
        else
        {
            // This is write
            linePtr->state = MODIFIED;
        }
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

int get_victim_way(bool is_i_or_d, uint32_t index)
{
    if (!is_i_or_d)
    {
        // This is D-Cache
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            if (dcache[index].lines[i].valid == false)
                return i; // We found a way with valid bit set to false, so return
        }
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            if (dcache[index].lines[i].lru_counter == 0)
                return i; // We found a way which is LRU, so return
        }
    }
    else
    {
        // This is I-Cache
        for (int i = 0; i < L1_ICACHE_WAYS; i++)
        {
            if (icache[index].lines[i].valid == false)
                return i; // We found a way with valid bit set to false, so return
        }
        for (int i = 0; i < L1_ICACHE_WAYS; i++)
        {
            if (icache[index].lines[i].lru_counter == 0)
                return i; // We found a way which is LRU, so return
        }
    }
    return 0;
}

void handle_invalidate(uint32_t index, uint32_t tag)
{

    for (int i = 0; i < L1_DCACHE_WAYS; i++)
    {
        // check if valid bit is 1
        if (dcache[index].lines[i].valid)
        {
            // Valid bit is set, now compare tags
            if (dcache[index].lines[i].tag == tag)
            {   
                dcache[index].lines[i].state = INVALID;
                dcache[index].lines[i].valid = false;
            }
        }
    }
return;
}

void handle_RFO(uint32_t index, uint32_t tag)
{
    for (int i = 0; i < L1_DCACHE_WAYS; i++)
    {
        // check if valid bit is 1
        if (dcache[index].lines[i].valid)
        {
            // Valid bit is set, now compare tags
            if (dcache[index].lines[i].tag == tag)
            {
               
                if (dcache[index].lines[i].state == MODIFIED)
                {
                    printf("Write to L2 %x\n", (tag * L1_CACHE_SETS + index) * CACHE_LINE_SIZE);

                    dcache[index].lines[i].state = INVALID;
                    dcache[index].lines[i].valid = false;

                }
                else
                {
                    dcache[index].lines[i].state = INVALID;
                    dcache[index].lines[i].valid = false;
                }
            }
        }
    }
    return;
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
        // Debug print to L2
        if (debug_mode)
            printf("Read for Ownership from L2 %x\n", address);

        // Find which way to evict
        int evict_way = get_victim_way(false, index);
        if (debug_mode)
        {
            if (dcache[index].lines[evict_way].state == MODIFIED)
                printf("Write to L2 %x\n", (tag * L1_CACHE_SETS + index) * CACHE_LINE_SIZE);
        }
        dcache[index].lines[evict_way].valid = true;
        dcache[index].lines[evict_way].tag = tag;
        dcache[index].lines[evict_way].state = MODIFIED;
        update_lru(false, evict_way, index);
    }
    return isHit;
}

bool read_cache(bool is_i_or_d, uint32_t index, uint32_t tag)
{
    bool isHit = false;
    if (!is_i_or_d)
    {
        printf("I am in D-Cache read\n");
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
            // Debug print to L2
            if (debug_mode)
                printf("Read from L2 %x\n", address);

            // Handle the miss
            // Find which way to evict
            int evict_way = get_victim_way(false, index);
            if (debug_mode)
            {
                if (dcache[index].lines[evict_way].state == MODIFIED)
                    printf("Write to L2 %x\n", (tag * L1_CACHE_SETS + index) * CACHE_LINE_SIZE);
            }
            dcache[index].lines[evict_way].valid = true;
            dcache[index].lines[evict_way].tag = tag;
            dcache[index].lines[evict_way].state = EXCLUSIVE;
            update_lru(is_i_or_d, evict_way, index);
        }
    }
    else
    {
        printf("I am in I-Cache read\n");
        isHit = false;
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
                    update_MESI(is_i_or_d, i, index, true);
                    break;
                }
            }
        }
        if (!isHit)
        {
            // Handle the miss
            // Debug print to L2
            if (debug_mode)
                printf("Read from L2 %x\n", address);

            // Find which way to evict
            int evict_way = get_victim_way(true, index);
            if (debug_mode)
            {
                if (icache[index].lines[evict_way].state == MODIFIED)
                    printf("Write to L2 %x\n", (tag * L1_CACHE_SETS + index) * CACHE_LINE_SIZE);
            }

            icache[index].lines[evict_way].valid = true;
            icache[index].lines[evict_way].tag = tag;
            icache[index].lines[evict_way].state = EXCLUSIVE;
            update_lru(is_i_or_d, evict_way, index);
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

    printf("is_i_or_d: %s\n", is_i_or_d ? "true" : "false");

    switch (operation)
    {
    case 0:
        break;
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
    case 3:
        handle_invalidate(index,tag);
        printf("\n\nInvalidate Command Sent to all Processors.\n\n\n");
        break;
    case 4:
        handle_RFO(index,tag);
        break;

    case 8:
        initialize_cache();
        break;
    case 9:
        print_cache_index(is_i_or_d, index);
        break;
    default:
        break;
    }

    print_cache_index(is_i_or_d, index); //Delete me later
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
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <trace_file> <mode>\n", argv[0]);
        return 1;
    }

    debug_mode = argv[2];

    initialize_cache();
    process_trace_file(argv[1]);
    print_stats();
    return 0;
}

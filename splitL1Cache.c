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
        }
    }
}

void update_lru(bool is_i_or_d, int way, uint32_t index, uint32_t tag)
{
    if (~is_i_or_d)
    {
        int accessed_lru_cnt = dcache[index].lines[way].lru_counter;
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            if (dcache[index].lines[i].lru_counter > accessed_lru_cnt)
            {
                dcache[index].lines[i].lru_counter--;
            }
        }

        dcache[index].lines[way].lru_counter = L1_DCACHE_WAYS;
    }
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
                    update_lru(is_i_or_d, i, index, tag);
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
                    dcache[index].lines[i].state = EXCLUSIVE;
                    update_lru(is_i_or_d, i, index, tag);
                    break;
                }
            }
        }
        return isHit;
    }
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

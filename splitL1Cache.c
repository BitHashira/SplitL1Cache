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
uint32_t cache_reads_d = 0, cache_writes_d = 0, cache_hits_d = 0, cache_misses_d = 0;
uint32_t cache_reads_i = 0, cache_writes_i = 0, cache_hits_i = 0, cache_misses_i = 0;
bool debug_mode = false;
int operation;
uint32_t address;

// Function to initialize caches
void initialize_cache()
{
    cache_reads_d = 0;
    cache_writes_d = 0;
    cache_hits_d = 0;
    cache_misses_d = 0;
    cache_reads_i = 0;
    cache_writes_i = 0;
    cache_hits_i = 0;
    cache_misses_i = 0;
    for (int i = 0; i < L1_CACHE_SETS; i++)
    {
        for (int j = 0; j < L1_ICACHE_WAYS; j++)
        {
            icache[i].lines[j].valid = false;
            icache[i].lines[j].firstWrite = true;
            icache[i].lines[j].state = INVALID;
            icache[i].lines[j].tag = 000;
            icache[i].lines[j].byteOffset = 000;
            icache[i].lines[j].lru_counter = j;
        }
    }
    for (int i = 0; i < L1_CACHE_SETS; i++)
    {
        for (int j = 0; j < L1_DCACHE_WAYS; j++)
        {
            dcache[i].lines[j].valid = false;
            dcache[i].lines[j].firstWrite = true;
            dcache[i].lines[j].state = INVALID;
            dcache[i].lines[j].tag = 000;
            dcache[i].lines[j].byteOffset = 000;
            dcache[i].lines[j].lru_counter = j;
        }
    }
}

void print_cache_index(bool whichCache, uint32_t index)
{

    printf("\n\n+------------------------------+\n");
    printf("| D-Cache Data at Index = %x | \n", index);
    DCacheSet tmp = dcache[index];
    for (int i = 0; i < L1_DCACHE_WAYS; i++)
    {
        printf("+--------++-------------------------------------------------------------------+\n");
        printf("| Way: %d |", i);
        printf("| Valid: %5s  ", tmp.lines[i].valid ? "true" : "false");
        printf("| Tag: %03x  ", tmp.lines[i].tag);
        printf("| lru_cntr: %01d  ", tmp.lines[i].lru_counter);
        printf("| MESI State: %9s  |", MESI_State_strings[tmp.lines[i].state]);
        printf("\n");
    }
    printf("+--------++-------------------------------------------------------------------+\n");

    printf("+------------------------------+\n");
    printf("| I-Cache Data at Index = %x | \n", index);
    ICacheSet tmp1 = icache[index];
    for (int i = 0; i < L1_ICACHE_WAYS; i++)
    {
        printf("+--------++-------------------------------------------------------------------+\n");
        printf("| Way: %d |", i);
        printf("| Valid: %5s  ", tmp1.lines[i].valid ? "true" : "false");
        printf("| Tag: %03x  ", tmp1.lines[i].tag);
        printf("| lru_cntr: %01d  ", tmp1.lines[i].lru_counter);
        printf("| MESI State: %9s  |", MESI_State_strings[tmp1.lines[i].state]);
        printf("\n");
    }
    printf("+--------++-------------------------------------------------------------------+\n");
}

// void print_cache_index(bool whichCache, uint32_t index)
// {
//
//     printf("\n\nD-Cache Data - \n");
//     DCacheSet tmp = dcache[index];
//     for (int i = 0; i < L1_DCACHE_WAYS; i++)
//     {
//         printf("Way: %d\n", i);
//         printf("Valid: %s\t", tmp.lines[i].valid ? "true" : "false");
//         printf("Tag: %03x\t", tmp.lines[i].tag);
//         printf("lru_counter: %d\t", tmp.lines[i].lru_counter);
//         printf("MESI State: %s\t", MESI_State_strings[tmp.lines[i].state]);
//         printf("\n");
//     }
//
//     printf("\n\nI-Cache Data - \n");
//     ICacheSet tmp1 = icache[index];
//     for (int i = 0; i < L1_ICACHE_WAYS; i++)
//     {
//         printf("Way: %d\n", i);
//         printf("Valid: %s\t", tmp1.lines[i].valid ? "true" : "false");
//         printf("Tag: %03x\t", tmp1.lines[i].tag);
//         printf("lru_counter: %d\t", tmp1.lines[i].lru_counter);
//         printf("MESI State: %s\t", MESI_State_strings[tmp1.lines[i].state]);
//         printf("\n");
//     }
// }

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

bool snoop_processors(uint32_t tag, uint32_t index)
{
    // Simulate snooping by checking if another cache has the line in E or S
    for (int i = 0; i < L1_DCACHE_WAYS; i++)
    {
        if (dcache[index].lines[i].valid && dcache[index].lines[i].tag == tag)
        {
            if (dcache[index].lines[i].state == SHARED || dcache[index].lines[i].state == EXCLUSIVE)
            {
                return true; // Another processor has the line
            }
        }
    }
    return false; // No other processor has it
}

void update_MESI(bool is_i_or_d, int way, uint32_t index, bool r_or_w)
{
    // Function to update MESI state
    CacheLine *linePtr = is_i_or_d ? &icache[index].lines[way] : &dcache[index].lines[way];
    switch (linePtr->state)
    {
    case MODIFIED:
        // If it's modified, it stays modified unless we receive an invalidation
        // This is write
        if (!r_or_w)
        {
            linePtr->state = MODIFIED;
        }
        break;
    case EXCLUSIVE:
        if (r_or_w)
        {
            // If another processor reads, move to SHARED
            // bool another_processor_has_line = snoop_processors(linePtr->tag, index);
            // if (another_processor_has_line)
            // {
            // If we are in EXCLUSIVE and it is a READ to the same address, we assume it is snoop
            linePtr->state = SHARED;
            // }
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


bool write_d_cache(uint32_t index, uint32_t tag, uint8_t byteOffset)
{
    // For handling Case 2
    bool isHit = false;
    cache_writes_d++;
    for (int i = 0; i < L1_DCACHE_WAYS; i++)
    {
        // check if valid bit is 1
        if (dcache[index].lines[i].valid)
        {
            // Valid bit is set, now compare tags
            if (dcache[index].lines[i].tag == tag)
            {
                // Tag bit also matches, this means we have a Hit
                if (debug_mode)
                {
                    if (dcache[index].lines[i].firstWrite)
                    {
                        printf("Write to L2 %x\n", address);
                        dcache[index].lines[i].firstWrite = false;
                    }
                }
                isHit = true;
                cache_hits_d++;
                update_MESI(false, i, index, false);
                update_lru(false, i, index);
                break;
            }
        }
    }
    if (!isHit)
    {
        // Handle the miss
        cache_misses_d++;
        // Debug print to L2
        if (debug_mode)
            printf("Read for Ownership from L2 %x\n", address);

        // Find which way to evict
        int evict_way = get_victim_way(false, index);
        if (debug_mode)
        {
            if (dcache[index].lines[evict_way].firstWrite)
            {
                if (dcache[index].lines[evict_way].state == INVALID)
                    printf("Write to L2 %x\n", address);
                else
                    printf("Write to L2 %x\n", (dcache[index].lines[evict_way].tag * L1_CACHE_SETS + index) * CACHE_LINE_SIZE + dcache[index].lines[evict_way].byteOffset);
                dcache[index].lines[evict_way].firstWrite = false;
            }
            if (dcache[index].lines[evict_way].state == MODIFIED)
            {
                // printf("Write to L2 %x\n", address);
                printf("Write to L2 %x\n", (dcache[index].lines[evict_way].tag * L1_CACHE_SETS + index) * CACHE_LINE_SIZE + dcache[index].lines[evict_way].byteOffset);
            }
        }
        dcache[index].lines[evict_way].valid = true;
        dcache[index].lines[evict_way].tag = tag;
        dcache[index].lines[evict_way].byteOffset = byteOffset;
        dcache[index].lines[evict_way].state = MODIFIED;
        update_lru(false, evict_way, index);
    }
    return isHit;
}

bool read_cache(bool is_i_or_d, uint32_t index, uint32_t tag, uint8_t byteOffset)
{
    bool isHit = false;
    if (!is_i_or_d)
    {
        // printf("I am in D-Cache read\n");
        // This is D-Cache
        cache_reads_d++;
        for (int i = 0; i < L1_DCACHE_WAYS; i++)
        {
            // check if valid bit is 1      &&  compare tags
            if (dcache[index].lines[i].valid && dcache[index].lines[i].tag == tag)
            {
                // Tag bit also matches, this means we have a Hit
                isHit = true;
                cache_hits_d++;
                // If another processor reads this line and it's in EXCLUSIVE, move to SHARED
                // if (dcache[index].lines[i].state == EXCLUSIVE)
                // {
                //     bool another_processor_has_line = snoop_processors(tag, index);
                //     if (another_processor_has_line)
                //     {
                //         dcache[index].lines[i].state = SHARED; // Downgrade to SHARED
                //     }
                // }
                update_MESI(is_i_or_d, i, index, true);
                update_lru(is_i_or_d, i, index);
                break;
            }
        }
        if (!isHit)
        {
            // Handle the miss
            cache_misses_d++;
            // Debug print to L2
            if (debug_mode)
                printf("Read from L2 %x\n", address);

            // Determine if another processor has the line
            // bool another_processor_has_line = snoop_processors(tag, index);

            // Find which way to evict
            int evict_way = get_victim_way(false, index);
            if (debug_mode && dcache[index].lines[evict_way].state == MODIFIED)
            {
                // printf("Write to L2 %x\n", address);
                printf("Write to L2 %x\n", (dcache[index].lines[evict_way].tag * L1_CACHE_SETS + index) * CACHE_LINE_SIZE + dcache[index].lines[evict_way].byteOffset);
            }
            dcache[index].lines[evict_way].valid = true;
            dcache[index].lines[evict_way].tag = tag;
            dcache[index].lines[evict_way].byteOffset = byteOffset;
            // if (another_processor_has_line)
            // {
            //     dcache[index].lines[evict_way].state = SHARED;
            // }
            // else
            // {
            dcache[index].lines[evict_way].state = EXCLUSIVE;
            // }

            update_lru(is_i_or_d, evict_way, index);
        }
    }
    else
    {
        // printf("I am in I-Cache read\n");
        cache_reads_i++;
        isHit = false;
        // This is I-Cache
        for (int i = 0; i < L1_ICACHE_WAYS; i++)
        {
            // check if valid bit is 1      &&  compare tags
            if (icache[index].lines[i].valid && icache[index].lines[i].tag == tag)
            {
                // Tag bit also matches, this means we have a Hit
                isHit = true;
                cache_hits_i++;
                update_lru(is_i_or_d, i, index);
                update_MESI(is_i_or_d, i, index, true);
                break;
            }
        }
        if (!isHit)
        {
            // Handle the miss
            cache_misses_i++;
            // Debug print to L2
            if (debug_mode)
                printf("Read from L2 %x\n", address);

            // Find which way to evict
            int evict_way = get_victim_way(true, index);
            if (debug_mode && icache[index].lines[evict_way].state == MODIFIED)
            {
                printf("Write to L2 %x\n", (icache[index].lines[evict_way].tag * L1_CACHE_SETS + index) * CACHE_LINE_SIZE + icache[index].lines[evict_way].byteOffset);
            }

            icache[index].lines[evict_way].valid = true;
            icache[index].lines[evict_way].tag = tag;
            icache[index].lines[evict_way].byteOffset = byteOffset;
            icache[index].lines[evict_way].state = EXCLUSIVE;
            update_lru(is_i_or_d, evict_way, index);
        }
    }
    return isHit;
}

void handle_RFO(bool is_i_or_d, uint32_t index, uint32_t tag, uint8_t byteOffset)
{
    bool L2return = false;
    for (int i = 0; i < L1_DCACHE_WAYS; i++)
    {
        // check if valid bit is 1
        if (dcache[index].lines[i].valid)
        {
            // Valid bit is set, now compare tags
            if (dcache[index].lines[i].tag == tag && dcache[index].lines[i].state == MODIFIED)
            {
                if (debug_mode){
                    printf("Return Data to L2 %x\n", address);
                    dcache[index].lines[i].state = INVALID;
                    dcache[index].lines[i].valid = false;
                    L2return = true;
                    return;
                }
                else
                {
                    dcache[index].lines[i].state = INVALID;
                    dcache[index].lines[i].valid = false;
                }
            }
       
            break;
        }
    }
    // Read if address wasn't available for L2, load to L1
    if (!L2return){
        read_cache(is_i_or_d, index, tag, byteOffset);
    }
    

    return;
}

// Function to access the cache (read/write operations)
void access_cache(uint32_t address, int operation)
{
    if (debug_mode)
        printf("\nYou have given me case: %d\n", operation);

    bool is_write = (operation == 1);
    bool is_i_or_d = (operation == 2);

    uint32_t index = (address / CACHE_LINE_SIZE) % L1_CACHE_SETS;
    uint32_t tag = address / (CACHE_LINE_SIZE * L1_CACHE_SETS);
    uint8_t byteOffset = address & (CACHE_LINE_SIZE - 1);

    if (debug_mode)
    {
        printf("Given Addr: %x\n", address);
        printf("Index: %x\n", index);
        printf("Tag: %x\n", tag);
        // printf("is_i_or_d: %s\n", is_i_or_d ? "true" : "false");
    }

    switch (operation)
    {
    case 0:
        if (read_cache(is_i_or_d, index, tag, byteOffset))
        {
            if (debug_mode)
                printf("D-Cache Read HIT");
        }
        else
        {
            if (debug_mode)
                printf("D-Cache Read MISS");
        }
        break;
    case 2:
        if (read_cache(is_i_or_d, index, tag, byteOffset))
        {
            if (debug_mode)
                printf("I-Cache Read HIT");
        }
        else
        {
            if (debug_mode)
                printf("I-Cache Read MISS");
        }
        break;
    case 1:
        if (write_d_cache(index, tag, byteOffset))
        {
            if (debug_mode)
                printf("D-Cache Write HIT");
        }
        else
        {
            if (debug_mode)
                printf("D-Cache Write MISS");
        }
        break;
    case 3:
        handle_invalidate(index, tag);
        printf("\n\nInvalidate Command Sent to all Processors.\n\n\n");
        break;
    case 4:
        handle_RFO(is_i_or_d, index, tag, byteOffset);
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

    if (debug_mode)
        print_cache_index(is_i_or_d, index); // Delete me later
}

void process_trace_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening trace file");
        return;
    }

    char line[256]; // Buffer to read each line
    while (fgets(line, sizeof(line), file))
    {
        // Skip lines that start with `//` (comments)
        if (line[0] == '/' && line[1] == '/')
        {
            continue;
        }

        // Parse operation and address
        int operation;
        // uint32_t address;
        if (sscanf(line, "%d %x", &operation, &address) == 2)
        {
            access_cache(address, operation);
        }
    }

    fclose(file);
}

// Function to process trace file and simulate cache behavior
// void process_trace_file(const char *filename)
// {
//     FILE *file = fopen(filename, "r");
//     if (!file)
//     {
//         perror("Error opening trace file");
//         return;
//     }
//
//     while (fscanf(file, "%d %x", &operation, &address) == 2)
//     {
//         access_cache(address, operation);
//     }
//     fclose(file);
// }

void print_stats()
{
    printf("\nFinal Stats - \n");
    printf("D-Cache -\n");
    printf("D-Cache Reads: %u\n", cache_reads_d);
    printf("D-Cache Writes: %u\n", cache_writes_d);
    printf("D-Cache Hits: %u\n", cache_hits_d);
    printf("D-Cache Misses: %u\n", cache_misses_d);
    printf("D-Cache Hit Ratio: %.2f%%\n", (cache_hits_d * 100.0) / (cache_hits_d + cache_misses_d));

    printf("\nI-Cache -\n");
    printf("I-Cache Reads: %u\n", cache_reads_i);
    printf("I-Cache Writes: %u\n", cache_writes_i);
    printf("I-Cache Hits: %u\n", cache_hits_i);
    printf("I-Cache Misses: %u\n", cache_misses_i);
    printf("I-Cache Hit Ratio: %.2f%%\n", (cache_hits_i * 100.0) / (cache_hits_i + cache_misses_i));
}

// Main function to initialize and run the simulation
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <trace_file> <mode>\n", argv[0]);
        return 1;
    }

    debug_mode = atoi(argv[2]);
    printf("Running in Debug Mode: %b\n", debug_mode);

    initialize_cache();
    process_trace_file(argv[1]);
    print_stats();
    return 0;
}

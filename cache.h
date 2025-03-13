#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define L1_CACHE_SETS 16384
#define L1_CACHE_ADDR 32
#define L1_ICACHE_WAYS 4
#define L1_DCACHE_WAYS 8
#define CACHE_LINE_SIZE 64

#define L1_CACHE_INDEX __builtin_ctz(L1_CACHE_SETS)
#define L1_CACHE_OFFSET __builtin_ctz(CACHE_LINE_SIZE)
#define L1_CACHE_TAG (L1_CACHE_ADDR - L1_CACHE_INDEX - L1_CACHE_OFFSET)

// MESI protocol states
typedef enum
{
    MODIFIED,
    EXCLUSIVE,
    SHARED,
    INVALID
} MESI_State;

const char *MESI_State_strings[] = {
    "MODIFIED",
    "EXCLUSIVE",
    "SHARED",
    "INVALID"};

// Structure representing a cache line
typedef struct
{
    bool valid; // Valid bit
    bool firstWrite;
    // bool tag[L1_CACHE_TAG]; // Tag for address mapping
    uint32_t tag;        // Tag for address mapping
    uint8_t byteOffset;  // Byte-offset bits
    MESI_State state;    // MESI state
    uint8_t lru_counter; // Counter for LRU replacement
} CacheLine;

// Structure representing an instruction cache set
typedef struct
{
    CacheLine lines[L1_ICACHE_WAYS];
} ICacheSet;

// Structure representing a data cache set
typedef struct
{
    CacheLine lines[L1_DCACHE_WAYS];
} DCacheSet;

//
// int main(void)
// {
//     printf("L1_CACHE_OFFSET = %d\n", L1_CACHE_OFFSET); // 6
//     printf("L1_CACHE_INDEX  = %d\n", L1_CACHE_INDEX);  // 14
//     printf("L1_CACHE_TAG    = %d\n", L1_CACHE_TAG);    // 12
//     return 0;
// }
//
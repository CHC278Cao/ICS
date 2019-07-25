#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define false 0
#define true 1

typedef struct {
    _Bool valid;
    uint64_t tag;
    uint64_t time_counter;
}line;
typedef line* entry_of_lines;
typedef entry_of_lines* entry_of_sets;
typedef struct {
    int hit;
    int miss;
    int eviction;
}result;

void printHelpInfo();
entry_of_sets InitializeCache(uint64_t S, uint64_t E);
result HitMissEviction(entry_of_lines search_line, result result, uint64_t E, uint64_t tag, _Bool verbose);
result ReadAndTest(FILE* tracefile, entry_of_sets cache, uint64_t S, 
        uint64_t E, uint64_t s, uint64_t b, _Bool verbose);
void ReleaseMemory(entry_of_sets cache, uint64_t S, uint64_t E);

int main(int argc, char** argv) {
    result Result = {0, 0, 0};
    FILE* tracefile = NULL;
    entry_of_sets cache = NULL;

    _Bool verbose = false;
    uint64_t s = 0;
    uint64_t b = 0;
    uint64_t S = 0;
    uint64_t E = 0;

    char opt;

    while(-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))) {
        switch(opt) {
            case 'h': {
                          printHelpInfo();
                          exit(0);
                      }
            case 'v': {
                          verbose = 1;
                          break;
                      }
            case 's': {
                          if (atol(optarg) <= 0) {
                              printHelpInfo();
                              exit(EXIT_SUCCESS);
                          } else {
                              s = atol(optarg);
                              S = 1 << s;   // S = 2^s
                              break;
                          }
                      }
            case 'E': {
                          if (atol(optarg) <= 0) {
                              printHelpInfo();
                              exit(EXIT_FAILURE);
                          } else {
                              E = atol(optarg);
                              break;
                          }
                      }
            case 'b': {
                          if (atol(optarg) <= 0) {
                              printHelpInfo();
                              exit(EXIT_FAILURE);
                          } else {
                              b = atol(optarg);
                              break;
                          }
                      }
            case 't': {
                          if ((tracefile = fopen(optarg, "r")) == NULL) {
                              printf("Failed to open file");
                              exit(EXIT_FAILURE);
                          }
                          break;
                      }
            default: {
                         printHelpInfo();
                         exit(EXIT_FAILURE);
                     }
        }
    }

    if (s == 0 || b == 0 || E == 0 || tracefile == NULL) {
        printHelpInfo();
        exit(EXIT_FAILURE);
    }

    cache = InitializeCache(S, E);
    Result = ReadAndTest(tracefile, cache, S, E, s, b, verbose);
    ReleaseMemory(cache, S, E);
//    printf("hits: %d miss: %d eviction %d\n", Result.hit, Result.miss, Result.eviction);
    printSummary(Result.hit, Result.miss, Result.eviction);
    return 0;
}


entry_of_sets InitializeCache(uint64_t S, uint64_t E) {
    entry_of_sets cache;

    if ((cache = calloc(S, sizeof(entry_of_lines))) == NULL) {
        perror("Failed to calloc entry_of_sets");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < S; ++i) {
        if ((cache[i] = calloc(E, sizeof(line))) == NULL) {
            perror("Failed to calloc line in sets");
        }
    }

    return cache;
}

result HitMissEviction(entry_of_lines search_line, result Result, uint64_t E,
        uint64_t tag, _Bool verbose) {
    uint64_t oldest_time = UINT64_MAX;
    uint64_t youngest_time = 0;
    uint64_t oldest_block = UINT64_MAX;
    _Bool hit_flag = false;

    for (uint64_t i = 0; i < E; ++i) {
        if (search_line[i].tag == tag && search_line[i].valid) {
            if (verbose)
                printf(" hit\n");
            hit_flag = true;
            ++Result.hit;
            ++search_line[i].time_counter;
            break;
        }
    }

    if (!hit_flag) {
        if (verbose)
            printf(" miss");
        ++Result.miss;
        uint64_t i;
        for (i = 0; i < E; ++i) {
            // search for the oldest block to evict
            if (search_line[i].time_counter < oldest_time) {
                oldest_time = search_line[i].time_counter;
                oldest_block = i;
            }
            // search for the youngest block
            if (search_line[i].time_counter > youngest_time) {
                youngest_time = search_line[i].time_counter;
            }
        }
        
        // LRU algorithm to evict the oldest block and 
        // make it the newest one
        search_line[oldest_block].time_counter = youngest_time + 1;
        search_line[oldest_block].tag = tag;

        if (search_line[oldest_block].valid) {
            if (verbose)
                printf(" and eviction\n");
            ++Result.eviction;
        } else {
            if (verbose) 
                printf("\n");
            search_line[oldest_block].valid = true;
        }
    }

    return Result;
}

result ReadAndTest(FILE* tracefile, entry_of_sets cache, uint64_t S, 
        uint64_t E, uint64_t s, uint64_t b, _Bool verbose) {
    result Result = {0, 0, 0};
    char ch;
    uint64_t address;

    while((fscanf(tracefile, " %c %lx%*[^\n]", &ch, &address)) == 2) {
        if (ch == 'I') {
            continue;
        } else {
            // get the hex of the maximum of set, 0x11..1(s)
            uint64_t set_index_mask = (1 << s) - 1;
            // get the set number for the address
            uint64_t set_index = (address >> b) & set_index_mask;
            // get the tag of the address
            uint64_t tag = (address >> b) >> s;
            entry_of_lines search_line = cache[set_index];

            if (ch == 'L' || ch == 'S') {
                if (verbose)
                    printf("%c %lx", ch, address);
                Result = HitMissEviction(search_line, Result, E, tag, verbose);
            } else if (ch == 'M') { // 'M' will do 'load' and 'save' two instructions
                if (verbose)
                    printf("%c %lx", ch, address);
                Result = HitMissEviction(search_line, Result, E, tag, verbose);
                Result = HitMissEviction(search_line, Result, E, tag, verbose);
            } else {
                continue;
            }
        }
    }

    return Result;
}

void ReleaseMemory(entry_of_sets cache, uint64_t S, uint64_t E) {
    for (uint64_t i = 0; i < S; ++i) {
        free(cache[i]);
    }
    free(cache);
}

void printHelpInfo() {
    printf(":Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("-h               Print the help message.\n");
    printf("-v               Optional verbose flag.\n");
    printf("-s <s>           Number of set index bits.\n");
    printf("-E <E>           Number of lines per set.\n");
    printf("-b <b>           Number of block bits.\n");
    printf("-t <tracefile>   Name of the valgrind trace to reply\n");
    printf("Examples:\n");
    printf("./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("./csim-ref -v -s 4 -E 1 -b 4 -t traces/yi.trace\n");
}



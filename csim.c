// Name: Aichen Yao      Andrew ID: aicheny
#include "cachelab.h"
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// This file serves as a cache simulator, counting the number of misses, hits,
// evictions, dirty_bytes and dierty_evictions caused by a series of load and
// write instructions. The main function reads from a trace file that specifies
// the instruction type, address, and size of access; as it parses the commands
// my cache simulator judges if that would cause a miss, a hit, or an enviction
// and update the data along the code. In the end, the file displays the result
// by calling printSummary().
typedef struct {
    long validBit;
    long dirtyBit;
    long tagBits;
    long lruCounter;
} cacheLine;

typedef struct {
    cacheLine **lines;
} cacheSet;

// myCache points to an array of cacheSets, which contains a field of array of
// cacheLine structs

long hits, misses, evictions, dirty_bytes, dirty_evictions;
// update these throughout the code, and put them into the result struct in the
// end

cacheSet **buildCache(long s, long b, long E, long S) {
    // Initialize myCache. Build cacheSets, pointers to lines, and lines (all
    // fields start as 0). If any malloc fails, free all allocated memory and
    // exit.
    int i;
    int j;
    int tmp1;
    int tmp2;
    cacheSet **myCache = (cacheSet **)malloc(sizeof(cacheSet *) * S);
    if (myCache == NULL) {
        exit(0);
    }
    for (i = 0; i < S; i++) {
        myCache[i] = (cacheSet *)malloc(sizeof(cacheSet));
        if (myCache[i] == NULL) {
            for (tmp1 = 0; tmp1 < i; tmp1++) {
                free(myCache[tmp1]);
            }
            free(myCache);
            exit(0);
        }
        myCache[i]->lines = (cacheLine **)malloc(sizeof(cacheLine *) * E);
        if (myCache[i]->lines == NULL) {
            for (tmp1 = 0; tmp1 < i; tmp1++) {
                free(myCache[tmp1]->lines);
                free(myCache[tmp1]);
            }
            free(myCache[i]);
            free(myCache);
            exit(0);
        }
        for (j = 0; j < E; j++) {
            myCache[i]->lines[j] = malloc(sizeof(cacheLine));
            if (myCache[i]->lines[j] == NULL) {
                for (tmp1 = 0; tmp1 < i; tmp1++) {
                    for (tmp2 = 0; tmp2 < E; tmp2++) {
                        free(myCache[tmp1]->lines[tmp2]);
                    }
                    free(myCache[tmp1]->lines);
                    free(myCache[tmp1]);
                }
                free(myCache);
                exit(0);
            }
            myCache[i]->lines[j]->validBit = 0;
            myCache[i]->lines[j]->dirtyBit = 0;
            myCache[i]->lines[j]->tagBits = 0;
            myCache[i]->lines[j]->lruCounter = 0;
        }
    }
    return myCache;
}

long findSet(long address, long s, long b) {
    // Use bit manipulations to locate the set (row) we want to search on
    // in the cache.
    long mask = (1L << s) - 1;
    long base = address >> b;
    return base & mask;
}

long isHit(cacheSet **myCache, long address, long s, long b, long E,
           long operation, long totalAccesses) {
    // Judge if the given address would be a hit, return 1 if so.
    // if the operation was store, then change the matching lines' dirtyBit to 1
    long addressSet = findSet(address, s, b);
    long addressTag = address >> (s + b);
    int j;
    for (j = 0; j < E; j++) {
        cacheLine *current = myCache[addressSet]->lines[j];
        if ((current->validBit == 1) && (current->tagBits == addressTag)) {
            current->lruCounter = totalAccesses;
            if (operation == 1) {
                current->dirtyBit = 1;
            }
            return 1;
        }
    }
    return 0;
}

long isEvict(cacheSet **myCache, long address, long s, long b, long E) {
    // determine if this miss needs an eviction; if all lines are valid (full),
    // then it needs an eviction
    long addressSet = findSet(address, s, b);
    int j;
    for (j = 0; j < E; j++) {
        if (myCache[addressSet]->lines[j]->validBit == 0) {
            // return 0 (no eviction needed) if we found an empty block
            return 0;
        }
    }
    return 1;
}

void normalMiss(cacheSet **myCache, long address, long s, long b, long E,
                long totalAccesses, long operation) {
    // Handle a miss that does not require an eviction. Do not create a new line
    // Update the validBit, addressTag, lruCounter of the line to be filled.
    // If operation is a store, set its dirtyBit to 1.
    long addressSet = findSet(address, s, b);
    long addressTag = address >> (s + b);
    long nextEmptyIndex = 0; // first empty line in the set to be filled
    while ((myCache[addressSet]->lines[nextEmptyIndex]->validBit == 1)) {
        nextEmptyIndex += 1;
    }
    // update the next empty line in the set
    myCache[addressSet]->lines[nextEmptyIndex]->validBit = 1;
    myCache[addressSet]->lines[nextEmptyIndex]->tagBits = addressTag;
    myCache[addressSet]->lines[nextEmptyIndex]->lruCounter = totalAccesses;
    myCache[addressSet]->lines[nextEmptyIndex]->dirtyBit = 0;
    if (operation == 1) {
        myCache[addressSet]->lines[nextEmptyIndex]->dirtyBit = 1;
    }
    return;
}

void doEviction(cacheSet **myCache, long address, long s, long b, long E,
                long totalAccesses, long operation) {
    // Perform evictions if we need an eviction
    // Find the line to be evicted by finding the line in the set with least
    // lruCounter. Update each field of that line like what I did for a normal
    // miss.
    int j;
    long addressSet = findSet(address, s, b);
    long addressTag = address >> (s + b);
    int lruIndex = 0;
    long minCounter = myCache[addressSet]->lines[0]->lruCounter;
    long currentCounter = 0;
    long B = 1 << b;
    for (j = 0; j < E; j++) {
        currentCounter = myCache[addressSet]->lines[j]->lruCounter;
        if (currentCounter < minCounter) {
            minCounter = currentCounter;
            lruIndex = j;
        }
    }
    if (myCache[addressSet]->lines[lruIndex]->dirtyBit == 1) {
        // If the dirtyBit was originally 1, increment on dirty_evictions
        // dirty_evictions do not count for the time a dirty line gets evicted,
        // but the number of dirty bytes IN TOTAL that got evicted.
        dirty_evictions += B;
    }
    myCache[addressSet]->lines[lruIndex]->dirtyBit = 0;
    if (operation == 1) {
        myCache[addressSet]->lines[lruIndex]->dirtyBit = 1;
    }
    myCache[addressSet]->lines[lruIndex]->tagBits = addressTag;
    return;
}

void loadAndStore(cacheSet **myCache, long address, long s, long b, long E,
                  long totalAccesses, long operation) {
    // Core of the code, determine if it is a hit or a miss. And in case it is a
    // miss, determine if it needs an eviction.
    if (isHit(myCache, address, s, b, E, operation, totalAccesses)) {
        hits += 1;
        return;
    }
    misses += 1;
    if (isEvict(myCache, address, s, b, E)) {
        evictions += 1;
        doEviction(myCache, address, s, b, E, totalAccesses, operation);
        return;
    }
    if (isEvict(myCache, address, s, b, E) == 0) {
        normalMiss(myCache, address, s, b, E, totalAccesses, operation);
        return;
    }
    return;
}

int main(int argc, char *argv[]) {
    // Operate parsing; pass on commands to loadAndStore() to process each new
    // line of operation; free myCache completely in the end; store data into
    // result struct and call printSummary().
    long opt, help_mode, verbose_mode, s, E, b, S, B;
    verbose_mode = 0;
    help_mode = 0;
    b = 0;
    E = 0;
    s = 0;
    B = 0;
    char *traceFile; // the current trace file that we are processing
    traceFile = NULL;
    char type; // load or store (L/S)
    long address;
    int size;
    FILE *trace;
    cacheSet **myCache;
    csim_stats_t *result;
    while ((opt = (getopt(argc, argv, "hvs:E:b:t:"))) != -1) {
        switch (opt) {
        case 'h':
            help_mode = 1;
            break;
        case 'v':
            verbose_mode = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            traceFile = (char *)optarg;
            break;
        default:
            printf("invalid inputs\n");
            break;
        }
    }
    if (help_mode == 1) {
        printf("help\n"); //"call help to the system"
        exit(0);
    }
    if (verbose_mode == 1) {
        printf("get info\n"); //"used for debugging earlier"
        exit(0);
    }
    S = 1L << s;
    // there are a number of S sets
    myCache = buildCache(s, b, E, S);
    long totalAccesses = 0;
    // totalAccesses keep track of the number of operations in the trace file.
    // The least recently used line would have the smallest totalAccesses,
    // meaning it was last accessed earliest in its respective set.
    trace = fopen(traceFile, "r");
    while (fscanf(trace, "%c %lx, %d ", &type, &address, &size) >= 0)
    // get a whole line, cite C library
    { // parse the trace files and get the address in hex to pass
        // on to loadAndStore()
        totalAccesses += 1;
        printf("%c %lx, %d ", type, address, size);
        switch (type) {
        case 'L':
            loadAndStore(myCache, address, s, b, E, totalAccesses, 0);
            break;
        case 'S':
            loadAndStore(myCache, address, s, b, E, totalAccesses, 1);
            break;
        default:
            break;
        }
    }
    B = 1L << b;
    // Note: dirty_bytes, instead of dirty_bits
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            if (myCache[i]->lines[j]->dirtyBit == 1)
                dirty_bytes += B;
        }
    }
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            free(myCache[i]->lines[j]);
        }
        // free by levels, from innermost to outermost
        free(myCache[i]->lines);
        free(myCache[i]);
    }
    free(myCache);
    fclose(trace);
    result = (csim_stats_t *)malloc(sizeof(csim_stats_t));
    if (result == NULL) {
        exit(0);
    }
    result->hits = hits;
    result->misses = misses;
    result->evictions = evictions;
    result->dirty_bytes = dirty_bytes;
    result->dirty_evictions = dirty_evictions;
    printSummary(result);
    free(result);
    return 0;
}
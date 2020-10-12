// Name: Aichen Yao      Andrew ID: aicheny
#include "cachelab.h"
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    long validBit;
    long dirtyBit;
    long tagBits;
    long lruCounter;
} cacheLine;

typedef struct {
    cacheLine **lines;
} cacheSet;

// myCache is a 2D matrix where each row represents a set and each column
// in a row represents a line, set to NULL first

long hits, misses, evictions, dirty_bytes, dirty_evictions;

cacheSet **buildCache(long s, long b, long E, long S) {
    int i;
    int j;
    cacheSet **myCache = (cacheSet **)malloc(sizeof(cacheSet *) * S);
    if (myCache == NULL) {
        exit(0);
    }
    for (i = 0; i < S; i++) {
        myCache[i] = (cacheSet *)malloc(sizeof(cacheSet));
        if (myCache[i] == NULL) {
            exit(0);
        }
        myCache[i]->lines = (cacheLine **)malloc(sizeof(cacheLine *) * E);
        if (myCache[i]->lines == NULL) {
            exit(0);
        }
        for (j = 0; j < E; j++) {
            myCache[i]->lines[j] = malloc(sizeof(cacheLine));
            if (myCache[i]->lines[j] == NULL) {
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
    long mask = (1 << s) - 1;
    long base = address >> b;
    return base & mask;
}

void findEvictions(cacheSet **myCache, long addressSet, long E,
                   long operation) {
    // If we need an eviction, find the line that we need to evict and set its
    // validBit to 0. If the line was dirty, increment dirty_evictions.
    cacheLine *minLine;
    long minCounter = myCache[addressSet]->lines[0]->lruCounter;
    long minLineIndex = 0;
    long currentCounter;
    for (int j = 0; j < E; j++) {
        currentCounter = myCache[addressSet]->lines[j]->lruCounter;
        if (currentCounter < minCounter) {
            minCounter = currentCounter;
            minLineIndex = j;
        }
    }
    minLine = myCache[addressSet]->lines[minLineIndex];
    minLine->validBit = 0;
    if (minLine->dirtyBit == 1) {
        dirty_evictions += 1;
    }
    return;
}

void missAndEvict(cacheSet **myCache, long addressSet, long E, long operation) {
    // determine if an eviction is needed when the access is already a miss; if
    // so, call findEvictions to handle the eviction
    long validLines = 0;
    for (int j = 0; j < E; j++) {
        if (myCache[addressSet]->lines[j]->validBit == 1) {
            validLines += 1;
        }
    }
    if (validLines < E) {
        return;
    }
    evictions += 1;
    findEvictions(myCache, addressSet, E, operation);
    return;
}

void loadAndStore(cacheSet **myCache, long address, long s, long b, long E,
                  long totalAccesses, long operation) {
    //"heavy lifting", determine if it is a hit or a miss. If it is a miss, call
    // a series of functions to handle evictions and dirty bytes.
    long addressSet = findSet(address, s, b);
    long addressTag = address >> (s + b);
    for (int j = 0; j < E; j++) {
        cacheLine *current = myCache[addressSet]->lines[j];
        if ((current->validBit == 1) && (current->tagBits = addressTag)) {
            hits += 1;
            current->lruCounter = totalAccesses;
            if (operation == 1) {
                dirty_bytes += 1;
            }
        }
    }
    misses += 1;
    missAndEvict(myCache, addressSet, E, operation);
}

int main(int argc, char *argv[]) {
    long opt, help_mode, verbose_mode, s, E, b, S;
    char buffer[1000];
    char *traceFile; // the current trace file that we are processing
    char type;       // load or store (L/S)
    long address;
    long size;
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
        system("cat help_info"); //"call help to the system"
        exit(0);
    }
    if (verbose_mode == 1) {
        printf("information\n");
    }
    S = 1 << s;
    // there are a number of S sets
    myCache = buildCache(s, b, E, S);
    long totalAccesses = 0;
    trace = fopen(traceFile, "r");
    while (fgets(buffer, 1000, trace)) // get a whole line, cite C library
    { // parse the trace files and get the address in hex to pass
        // on to loadAndStore()
        sscanf(buffer, " %c, %ld, %ld", &type, &address, &size);
        totalAccesses += 1;
        switch (type) {
        // if it is a load, type is 0; if it is a store, type is 1
        case 'L':
            loadAndStore(myCache, address, s, b, E, totalAccesses, 0);
            break;
        case 'S':
            loadAndStore(myCache, address, s, b, E, totalAccesses, 1);
            break;
        }
    }
    for (int i = 0; i < S; i++) { // free each row of myCache
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
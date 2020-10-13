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


long isHit(cacheSet **myCache, long address, long s, long b, long E, 
long operation, long totalAccesses)
{
    //judge if the given address would be a hit, return 1 if so.
    //if it is a set, then change the matching lines' dirtyBit to 1
    long addressSet = findSet(address, s, b);
    long addressTag = address >> (s + b);
    int j;
    for (j = 0; j < E; j++) 
    {
        cacheLine *current = myCache[addressSet]->lines[j];
        printf("current->tagBits,%ld\n", current->tagBits);
        printf("addressTag,%ld\n", addressTag);
        if ((current->validBit == 1) && (current->tagBits == addressTag))
        {
            if (operation == 1)
            {
                current->dirtyBit = 1;
                current->lruCounter = totalAccesses;
            }
            return 1;
        } 
    }
    return 0;
}

long isEvict(cacheSet **myCache, long address, long s, long b, long E)
{
    //determine if this miss needs an eviction; if all lines are valid (full), 
    //then it needs an eviction
    long addressSet = findSet(address, s, b);
    int j;
    for (j = 0; j < E; j ++)
    {
        if (myCache[addressSet]->lines[j]->validBit == 0)
        {
            //return 0 if we found an empty block
           return 0;
        }
    }
    return 1;
}

void normalMiss(cacheSet **myCache, long address, long s, long b, long E, 
long totalAccesses, long operation)
{
    //handle a miss that does not require an eviction
    cacheLine* newLine = NULL;
    long addressSet = findSet(address, s, b);
    int j;
    for (j = 0; j < E; j ++)
    {
        if (myCache[addressSet]->lines[j]->validBit == 0)
        { 
            newLine = myCache[addressSet]->lines[j];
        }
    }
    newLine->validBit = 1;
    newLine->tagBits = address >> (s + b);
    printf("newLine->tagBits,%ld\n", newLine->tagBits);
    newLine->lruCounter = totalAccesses;
    if (operation == 1)
    {
        newLine->dirtyBit = 1;
    }
    // printf("finished normal miss\n");
    return;
}

void doEviction(cacheSet **myCache, long address, long s, long b, long E,
long totalAccesses, long operation)
{
    int j;
    long addressSet = findSet(address, s, b);
    int lruIndex = 0;
    long minCounter = myCache[addressSet]->lines[0]->lruCounter;
    long currentCounter;
    cacheLine* newLine = (cacheLine*) malloc(sizeof(cacheLine));
    if (newLine == NULL)
    {
        exit(0);
    }
    for (j = 0; j < E; j++) 
    {
        currentCounter = myCache[addressSet]->lines[j]->lruCounter;
        if (currentCounter < minCounter) 
        {
            minCounter = currentCounter;
            lruIndex = j;
        }
    }
    if (myCache[addressSet]->lines[lruIndex]->dirtyBit == 1)
    {
        dirty_evictions += 1;
    }
    newLine->validBit = 1;
    newLine->dirtyBit = 0;
    if (operation == 1)
    {
        newLine->dirtyBit = 1;
    }
    newLine->tagBits = address >> (s+b);
    printf("newLine->tagBits,%ld\n", newLine->tagBits);
    newLine->lruCounter = totalAccesses;
    myCache[addressSet]->lines[lruIndex] = newLine;
}

void loadAndStore(cacheSet **myCache, long address, long s, long b, long E,
long totalAccesses, long operation) 
{
    //"heavy lifting", determine if it is a hit or a miss. If it is a miss, call
    // a series of functions to handle evictions and dirty bytes.
    if (isHit(myCache, address, s, b, E, operation, totalAccesses))
    {
        hits += 1;
        return;
    }
    misses += 1;
    if (isEvict(myCache, address, s, b, E))
    {
        evictions += 1;
        doEviction(myCache, address, s, E, b, totalAccesses, operation);
        return;
    }
    if (isEvict(myCache, address, s, b, E) == 0)
    {
        normalMiss(myCache, address, s, b, E, totalAccesses, operation);
        return;
    }
    return;
}

int main(int argc, char *argv[]) {
    long opt, help_mode, verbose_mode, s, E, b, S;
    verbose_mode = 0;
    help_mode = 0;
    b = 0;
    E = 0;
    s = 0;
    char *traceFile; // the current trace file that we are processing
    traceFile = NULL;
    char type;       // load or store (L/S)
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
    S = 1 << s;
    // there are a number of S sets
    myCache = buildCache(s, b, E, S);
    long totalAccesses = 0;
    trace = fopen(traceFile, "r");
    while (fscanf(trace, "%c %lx, %d ", &type, &address, &size ) >= 0) 
    // get a whole line, cite C library
    { // parse the trace files and get the address in hex to pass
        // on to loadAndStore()
        long oldHits = hits;
        long oldMisses = misses;
        long olddirty_bytes = dirty_bytes;
        long oldevictions = evictions;
        long oldDirtyEvictions = dirty_evictions;
        totalAccesses += 1;
        // printf("before\n");
        printf("%c %lx, %d", type, address, size);
        // printf("after\n");
        switch (type) {
        // if it is a load, type is 0; if it is a store, type is 1
        case 'L':
            // printf("load\n");
            loadAndStore(myCache, address, s, b, E, totalAccesses,0);
            break;
        case 'S':
            // printf("store\n");
            loadAndStore(myCache, address, s, b, E, totalAccesses,1);
            break;
        default:
            // printf("default?\n");
            break;
        }
        if (verbose_mode)
        {
            if (oldHits != hits)
            {
                printf("hit ");
            }
            if (oldMisses != misses)
            {
                printf("miss ");
            }
            if (olddirty_bytes != dirty_bytes)
            {
                printf("hit ");
            }
            if (oldevictions != evictions)
            {
                printf( "eviction ");
            }
            if (oldDirtyEvictions != dirty_evictions)
            {
                printf("dirty_eviction ");
            }
        }
    }
    for (int i = 0; i < S; i++)
    {
        for (int j = 0; j < E; j++)
        {
            if (myCache[i]->lines[j]->dirtyBit == 1)
                dirty_bytes += 1;
        }
    }
    for (int i = 0; i < S; i++)
    {
        for (int j = 0; j < E; j++)
        {
            free(myCache[i]->lines[j]);
        }
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
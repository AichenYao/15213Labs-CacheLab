//Name: Aichen Yao      Andrew ID: aicheny
#include "cachelab.h"
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<limits.h>
#include<getopt.h>
#include<string.h> 
#define LONG_NUMBER 64


typedef struct{
    long validBit;
    long dirtyBit;
    long tagBits;
    long lruCounter;
} cacheLine;

//myCache is a 2D matrix where each row represents a set and each column
//in a row represents a line, set to NULL first

long hits, misses, evictions, dirty_bytes, dirty_evictions;

long findSet(long address, long s, long b)
//Use bit manipulations to locate the set (row) we want to search on
//in the cache. The output of this function is the setIndex.
{
    long mask = (1 << s) - 1;
    long base = address >> b;
    return base & mask;
}

long getTag(long address, long s, long b)
//Use bit manipulations to extract the tag bits from the address.
//We will use this to determine pinpoint the block in the target set (row)
{
    return address >> (s+b);
}

long findEvictions(cacheLine **myCache, long addressSet, long E)
{
    long minCounter = myCache[addressSet][0]->lruCounter;
    long minLine = 0;
    for (int j = 0; j < E; j++)
    {
        long currentCounter = myCache[addressSet][j]->lruCounter;
        if (currentCounter < minCounter)
        {
            minCounter = currentCounter;
            minLine = j;
        }
    }
    return minLine;
}

void getHit (cacheLine* current, long totalAccesses)
//Update the time of the line that was just hit
{
    current->lruCounter = totalAccesses;
}

void loadAndStore(long address, long s, long b, long E, long 
totalAccesses)
{
    long addressSet = findSet(address, s, b);
    long addressTag = getTag(address, s, b);
    for (int j = 0; j < E; j ++)
    {
        cacheLine* current = myCache[addressSet][j];
        if ((current->validBit == 1) && (current->tagBits = addressTag))
        {
            hits += 1;
            getHit(current, totalAccesses);
        }
    }
    misses += 1;
}

int main(int argc, char* argv[])
{
    long opt, help_mode, verbose_mode, s, E, b, S;
    char buffer[1000];
    char* traceFile; //the current trace file that we are processing
    char type;   //load or store (L/S)
    long address;
    long size;
    int i, j;
    FILE* trace;
    cacheLine **myCache;
    csim_stats_t* result;
    while ((opt = (getopt(argc, argv, "hvs:E:b:t:"))) != -1)
    {
        switch(opt)
        {
            case 'h':help_mode = 1;
					        break;
			case 'v':verbose_mode = 1;
					        break;
            case 's':s = atoi(optarg);
                            break;
            case 'E':E = atoi(optarg);
                            break;
            case 'b':b = atoi(optarg);
                            break;
            case 't':traceFile = (char *)optarg;
                            break;
            default:
                    printf("invalid inputs\n");
                    break;
        }
    }
	if (help_mode == 1)
	{
		system("cat help_info");//"call help to the system"
		exit(0);
	}
    if (verbose_mode == 1)
    {
        printf("information\n");
    }
    S = 1 << s;
    //there are a number of S sets
    myCache = (cacheLine **)malloc(sizeof(cacheLine*) * S);
    if (myCache == NULL)
    {
        exit(0);
    }
    for (i = 0; i < S; i++)
    {
        myCache[i] = (cacheLine*)malloc(sizeof(cacheLine) * E);
        //i is the row-index into myCache, each row represents a set,
        //and a set is made of E lines
        for (j = 0; j < E; j++)
        {
            myCache[i][j]->validBit = 0;
            myCache[i][j]->dirtyBit = 0;
            myCache[i][j]->tagBits = 0;
            myCache[i][j]->lruCounter = 0;                                                                                        
        }
    }
    trace = fopen(traceFile,"r");
    if (trace == NULL)
    {
        printf("invalid trace file\n");
        exit(0);
    }
    long totalAccesses = 0;
    while(fgets(buffer,1000,trace))  //get a whole line, cite C library
    {   //parse the trace files and get the address in hex to pass
        //on to loadAndStore()
        sscanf(buffer," %c, %ld, %ld", &type, &address, &size);
        totalAccesses += 1;
        switch(type)
        {
            case 'L':
                loadAndStore(address, s, b, E, totalAccesses);
                break;
            case 'S':
                loadAndStore(address, s, b, E, totalAccesses);
                break;
        }
    }
    for (int i = 0; i < S; i++)
    {   //free each row of myCache
        free(myCache[i]);
    }  
    free(myCache);
    fclose(trace);
    result = (csim_stats_t*)malloc(sizeof(csim_stats_t));
    if (result == NULL)
    {
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
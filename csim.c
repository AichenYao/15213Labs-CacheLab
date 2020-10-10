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
    long tagBits;
    long lruCounter;
}line_struct;

line_struct **myCache = NULL;
//myCache is a 2D matrix where each row represents a set and each column
//in a row represents a line, set to NULL first

char traceFile[1000];
char buffer[1000];

long t, s, b, S, E, help_mode, verbose_mode, hits, misses, evictions;
long dirty_bytes, dirty_evictions;

long findSet(long address, long s, long b)
//Use bit manipulations to locate the set (row) we want to search on
//in the cache. The output of this function is the setIndex.
{
    long mask = (1 << s) - 1;
    long base = address >> b;
    return base & mask;
}

long getTag(long address, long t, long s, long b)
//Use bit manipulations to extract the tag bits from the address.
//We will use this to determine pinpoint the block in the target set (row)
{
    long base = address >> (b + s);
    long mask = (1 << t) - 1;
    return base & mask;
}

void loadAndStore(long address, long t, long s, long b, long E)
{
    long addressSet = findSet(address, s, b);
    long addressTag = getTag(address, t, s, b);
    for (int j = 0; j < E; j ++)
    {
        line_struct* current = myCache[addressSet][j];
        if (current->validBit == 1)
            {
                if (current->tagBits == addressTag)
                {
                    hits += 1;
                }
            }
        misses += 1;
    }
}

int main(int argc, char* argv[])
{
    long opt;
    long temp;
    char type;
    long address;
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
            case 't':strcpy(traceFile, optarg);
                            break;
        }
    }
	if (help_mode == 1)
	{
		system("cat help_info");//"call help to the system"
		exit(0);
	}
    FILE* trace = fopen(traceFile,"r");
    if (trace == NULL)
    {
        fprintf(stderr,"invalid file!\n");
        exit(-1);
    }
    S = 1 << s;
    //there are a number of S sets
    myCache = (line_struct**) malloc(sizeof(line_struct*) * S);
    for(int i = 0; i < S; i++)
    {
        myCache[i] = (line_struct*)malloc(sizeof(line_struct) * E);
        //i is the row-index into myCache, each row represents a set,
        //and a set is made of E lines
    }
    for (int i = 0; i < S; i++)
    {
        for (int j = 0; j < E; j++)
        {
            myCache[i][j]->validBit = 0;
            //the validBit of all lines are 0 at first, "cold misses"
        }
    }
    while(fgets(buffer,1000,trace))  //get a whole line, cite C library
    {   //parse the trace files and get the address in hex to pass
        //on to loadAndStore()
        sscanf(buffer," %c, %ld, %ld", &type, &address, &temp);
        switch(type)
        {
            case 'L':loadAndStore(address, t, s, b, E);
            case 'S':loadAndStore(address, t, s, b, E);
                                    break;
        }
    }
    for (int i = 0; i < S; i++)
    {   //free each row of myCache
        free(myCache[i]);
    }  
    free(myCache);
    fclose(trace);
    return 0;
}
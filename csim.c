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
    long hits;
    long misses;
    long evictions;
    long dirty_bytes;
    long dirty_evictions;
}csim_stats_t;

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
long getTag(long address, long s, long b);

void loadAndStore(long address);


int main(int argc, char* argv[])
{
    long opt;
    long hits = 0;
    int misses = 0;
    long evictions = 0;
    long s;
    long S;
    long E;
    long b;
    long t;
    long help_mode;
    long verbose_mode;
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
            case 't':t = atoi(optarg);
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
    free(myCache);
    fclose(trace);
    return 0;

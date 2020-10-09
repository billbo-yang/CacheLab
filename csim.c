/**
 * William Yang
 * wbyang
 */
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <math.h>

#include "cachelab.h"

// use 64-bit variable for memory addresses
typedef unsigned long long int memAddr;

/* struct to hold cache parameters*/
typedef struct {
	int s; //2^s sets
	int b; //block size 2^b bytes
	int E; //number of lines per set
	int S; //number of sets (S=2^s)
	int B; //cacheline block size (B=2^b)
	char *trace_file; //trace file from valgrind

	// keeping track of stats
	int hits;
	int misses;
	int evicts;

} cParameters;

/* line struct to represent lines in cache */
typedef struct {
	int valid;
	memAddr tag;
	int timestamp;
} line;

/* struct to represent sets in cache */
typedef struct {
	line *lines;
} set;

/* struct to represent caches */
typedef struct {
	set *sets;
} cache;

/**
 * outputs usage information
 * takes command line arguments
 * returns nothing
 */
void usageInfo(char* argv[])
{
	printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
	printf("Options:\n");
	printf("  -h         Print this help message.\n");
	printf("  -v         Optional verbose flag.\n");
	printf("  -s <num>   Number of set index bits.\n");
	printf("  -E <num>   Number of lines per set.\n");
	printf("  -b <num>   Number of block offset bits.\n");
	printf("  -t <file>  Trace file.\n");
	printf("\nExamples:\n");
	printf("  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
	printf("  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
	exit(0);
}

/**
 * make a cache with the provided number of sets and lines
 * takes the number of sets and lines desired in the cache
 * returns a cache struct
 */
cache makeCache(long long numSets, int numLines)
{
	cache mcache;
	set mset;
	line mline;
	int setCount;
	int lineCount;

	//allocating appropriate memory for the cache sets
	mcache.sets = (set*) malloc(sizeof(set) * numSets);

	for (setCount = 0; setCount < numSets; setCount++)
	{
		//allocating appropriate memory for the cache lines
		mset.lines = (line*) malloc(sizeof(line) * numLines);

		//put each set into the cache
		mcache.sets[setCount] = mset;

		for(lineCount = 0; lineCount < numLines; lineCount++)
		{
			//initialize line parameters and put each line into the set
			mline.timestamp = 0;
			mline.valid = 0;
			mline.tag = 0;
			mset.lines[lineCount] = mline;
		}
	}

	return mcache;
}

/**
 * free up memory associated with provided cache
 * takes the cache that is to be deleted, the number of sets & lines in the cache
 * returns nothing
 */
void deleteCache(cache cache, long long numSets, int numLines)
{
	int setCount;

	for(setCount = 0; setCount < numSets; setCount++)
	{
		set set = cache.sets[setCount];

		//if the lines aren't empty, free up the memory associated with the lines of the cache
		if(set.lines != NULL) {
			free(set.lines);
		}
	}

	//if the set isn't empty, free the memory associated with the sets of the cache
	if(cache.sets != NULL)
	{
		free(cache.sets);
	}
}

/**
 * find the first available empty line in a provided set
 * takes the set in which the line is to be found, the number of lines inside the set
 * returns in index of the first empty line it finds
 */
int findEmptyLine(set tset, int numLines)
{
	int lineNum;
	line line;

	//go through all lines in the provided set
	for(lineNum = 0; lineNum < numLines; lineNum++)
	{
		line = tset.lines[lineNum];

		//if the line isn't valid then it is for all intents and purposes empty
		if(line.valid == 0)
		{
			return lineNum;
		}
	}

	//if there is no empty line, return error
	return -1;
}

/**
 * finds the least recently used line as well as populates the provided LMRULines array
 * takes the set in which the line is to be found,
 * the number of liens in the set,
 * and an array that holds the indices of the MRU line and LRU line
 *
 * returns the index of the LRU line
 */
int findLRULine(set set, int numLines, int LMRULines[])
{
	int LRU = set.lines[0].timestamp;
	int MRU = set.lines[0].timestamp;
	int LRUIndex = 0;

	int lineCount;
	line line;

	//go through all lines in the provided set and compare/replace line's timestamps
	for (lineCount = 1; lineCount < numLines; lineCount++)
	{
		line = set.lines[lineCount];

		//if LRU is more recently used than the current line
		if (LRU > line.timestamp)
		{
			LRUIndex = lineCount;
			LRU = line.timestamp;
		}

		//if MRU is less recently used than the current line
		if(MRU < line.timestamp)
		{
			MRU = line.timestamp;
		}
	}

	LMRULines[0] = LRU;
	LMRULines[1] = MRU;
	return LRUIndex;
}

/**
 * simulates sending an address into a cache with the provided parameters
 * takes the cache to be simulated on,
 * the parameters of said cache,
 * and the address being put into cache
 *
 * returns an updated set of cache parameters
 */
cParameters simCache(cache sim, cParameters params, memAddr address)
{
	int lineCount;
	int cacheFull = 1; //1 if full, 0 if not

	int numLines = params.E;
	int origNumHits = params.hits;

	int tagBits = 64 - (params.s + params.b);

	//finding the input tag and set index from the provided memory address
	memAddr tag = address >> (params.s + params.b);
	unsigned long long temp = address << (tagBits);
	unsigned long long setIndex = temp >> (tagBits+params.b);

	set simSet = sim.sets[setIndex];

	//go through each line in the set and check to see if there is a matching tag present
	for (lineCount = 0; lineCount < numLines; lineCount++)
	{
		line currLine = simSet.lines[lineCount];

		if(currLine.valid)
		{
			//if the current line's tag is the same as our input tag...
			if(currLine.tag == tag)
			{
				//increment parameters, timestamps (for LRU) and set equal
				params.hits++;
				currLine.timestamp++;

				simSet.lines[lineCount] = currLine;
			}
		}
		//if a line isn't valid then the cache isn't full
		else
		{
			cacheFull = 0;
		}
	}

	//if the number of hits was unchanged, then it's a miss
	if (origNumHits == params.hits)
	{
		params.misses++;
	}
	else
	{
		return params;
	}

	//if we make it this far, that means we've missed
	//must evict LRU line if necessary then write into cache

	//find LRU line
	int LMRULines[2];
	int LRULine;
	LRULine = findLRULine(simSet, params.E, LMRULines);

	//if cache is full, then evict and replace LRU line
	if(cacheFull)
	{
		params.evicts++;

		simSet.lines[LRULine].tag = tag;
		simSet.lines[LRULine].timestamp = LMRULines[1] + 1;
	}
	//if cache isn't full, then we can find a non-valid line and place it there
	else
	{
		int eLineIndex = findEmptyLine(simSet, params.E);

		simSet.lines[eLineIndex].tag = tag;
		simSet.lines[eLineIndex].valid = 1;
		simSet.lines[eLineIndex].timestamp = LMRULines[1] + 1;
	}

	return params;
}

/**
 * main function, runs everything
 * takes the number of command line arguments present,
 * and an array of the commands
 *
 * always returns 0 - but doesn't mean anything.
 */
int main(int argc, char** argv)
{
	cache cache;
	cParameters params;

    FILE* inputFile;
    char* input;
    char command;
    memAddr addr;
    int size;

    char c;
    while((c = getopt(argc, argv, "s:E:b:t:v:h")) != -1)
    {
    	if(c == 's')
    	{
			params.s = atoi(optarg);
    	}
    	else if(c == 'E')
    	{
			params.E = atoi(optarg);
    	}
    	else if(c == 'b')
    	{
			params.b = atoi(optarg);
    	}
    	else if(c == 't')
    	{
			input = optarg;
    	}
    	else if(c == 'h')
    	{
    		usageInfo(argv);
    		exit(0);
    	}
    	else
    	{
    		usageInfo(argv);
    		exit(1);
    	}
	}

	//if we're missing anything...
    if (params.s == 0 || params.E == 0 || params.b == 0 || input == NULL)
    {
    	printf("Missing required command line argument\n");
    	usageInfo(argv);
    	exit(1);
    }

    //compute B & S from provided information
    params.S = pow(2, params.s);
    params.B = 1 << params.b;

    //set all hits, misses, and evicts to zero
    params.hits = 0;
    params.misses = 0;
    params.evicts = 0;

    cache = makeCache(params.S, params.E);

    //open file
    inputFile = fopen(input, "r");

    if (inputFile != NULL)
    {
    	while(fscanf(inputFile, " %c %11llx,%d", &command, &addr, &size) == 3)
    	{
    		if (command == 'L')
    		{
    			params = simCache(cache, params, addr);
    		}
    		else if (command == 'S')
    		{
    			params = simCache(cache, params, addr);
    		}
    		else if (command == 'M')
    		{
    			params = simCache(cache, params, addr);
    			params = simCache(cache, params, addr);
    		}
    	}
    }

    printSummary(params.hits, params.misses, params.evicts);
    fclose(inputFile);

    return 0;
}

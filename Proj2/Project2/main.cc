/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

	printf("===== 506 Personal information =====\n");
	printf("Zong-Ru Li\n");
	printf("zli87\n");
	printf("Undergraduate Students? NO\n");
	//****************************************************//
	//**printf("===== Simulator configuration =====\n");**//
	//*******print out simulator configuration here*******//
	printf("===== 506 SMP Simulator configuration =====\n");
	printf("L1_SIZE: %d\n",cache_size);
	printf("L1_ASSOC: %d\n",cache_assoc);
	printf("L1_BLOCKSIZE: %d\n",blk_size);
	printf("NUMBER OF PROCESSORS: %d\n",num_processors);
	if( protocol == 0)
		printf("COHERENCE PROTOCOL: MSI\n");
	else if(protocol==1)
		printf("COHERENCE PROTOCOL: MESI\n");
	else if(protocol==2)
		printf("COHERENCE PROTOCOL: Dragon\n");
	printf("TRACE FILE: %s\n",fname+3);
 	//****************************************************//

	//*********************************************//
    //*****create an array of caches here**********//
	Cache** processors;
	
	processors = new Cache*[num_processors];
	for(int P=0; P < num_processors ;++P){
		if(protocol==0){			// MSI protocol
			processors[P] = (Cache*)new Cache_MSI(cache_size, cache_assoc, blk_size, P, num_processors);
		} else if(protocol==1){		// MESI protocol
			processors[P] = (Cache*)new Cache_MESI(cache_size, cache_assoc, blk_size, P, num_processors);
		} else if(protocol==2){
			processors[P] = (Cache*)new Cache_Dragon(cache_size, cache_assoc, blk_size, P, num_processors);
		}
	}	
	//*********************************************//	

	pFile = fopen (fname,"r");
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}	

	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//

	uint Pi;
	int i=0;
	ulong addr;
	char str[2];
	uchar op;

	while(fscanf(pFile, "%d %s %lx ", &Pi, str, &addr ) != EOF)
    {
		op = str[0];
		processors[Pi]->Request_Processor(addr,op, processors);
		++i;
	}
	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	for( Pi=0; Pi< (uint)num_processors ;Pi++){
		processors[Pi]->printStats();
	}
	//********************************//
	
}

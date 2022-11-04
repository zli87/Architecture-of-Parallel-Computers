/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>
#include <iomanip>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum BusRequest{
   None = 0,
	BusRd ,
	BusRdX,  
   BusUpgr,  // Bus Upgrade
   Flush,
   BusUpd   // Bus Update
};

enum CS{    // coherence states
	INVALID = 0,
	SHARE,   
	MODIF,   // Modified
   EXCLU,   // Exclusive
   SMOD,    // Shared Modified
   SCLN     // Shared Clean
};
/*===========================================*/

class cacheLine 
{
protected:
   ulong tag;
   CS Flags;   // 0:invalid, 1:shared, 2:modified, 3: exclusive, 4: Shared Modified, 5: Shared Clean
   ulong seq;     // time stamp for LRU
public:
   cacheLine()            { tag = 0; Flags = INVALID; }
   ulong getTag()         { return tag; }
   CS getFlags()			{ return Flags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(CS flags)			{  Flags = flags;}
   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }//useful function
   bool isValid()         { return ((Flags) != INVALID); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;

   //******///
   //add coherence counters here///
   ulong busReads,busReadXs,invalidations,interventions,memtrans,flushes;
   ulong C2Ctrans; ulong getC2Ctrans(){ return C2Ctrans; };
   ulong getInv(){return invalidations;}; ulong getint(){return interventions;};
   ulong getBusRds(){return busReads;}; ulong getBusRdXs(){ return busReadXs;};
   ulong getMemTrans(){return memtrans;}; ulong getFlush(){return flushes;};
   float getMissRate(){return float(100*(getRM()+getWM()))/(getReads()+getWrites());};
   ulong num_p,num_pmax;
   ulong getNumP(){return num_p;}; ulong getNumPMAX(){return num_pmax;};
   //******///

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  
     
    Cache(int,int,int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   virtual cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM(){return readMisses;} ulong getWM(){return writeMisses;} 
   ulong getReads(){return reads;}ulong getWrites(){return writes;}
   ulong getWB(){return writeBacks;}
   
   void writeBack(ulong)   {writeBacks++; memtrans++;}
   void updateLRU(cacheLine *);
   void Access(ulong,uchar);
   void printStats();
   //******///
   //add other functions to handle bus transactions///
   virtual void Request_Processor(ulong,uchar,Cache**& )=0;
   virtual void Request_Bus(ulong,BusRequest)=0;
   //******///
};

class Cache_MSI : public Cache{
public:
   Cache_MSI(int a,int s,int b, int p, int pmax):Cache(a, s, b,p,pmax){};
   void Request_Processor(ulong,uchar,Cache**&);
   void Request_Bus(ulong,BusRequest);
};

class Cache_MESI :public Cache{
public:
   Cache_MESI(int a,int s,int b, int p, int pmax):Cache(a, s, b,p,pmax){};
   void Request_Processor(ulong,uchar,Cache**&);
   void Request_Bus(ulong,BusRequest);
};

class Cache_Dragon :public Cache{
public:
   cacheLine *fillLine(ulong addr); 
   Cache_Dragon(int a,int s,int b, int p,int pmax):Cache(a, s, b, p, pmax){};
   void Request_Processor(ulong,uchar,Cache**&);
   void Request_Bus(ulong,BusRequest);
   
};

#endif

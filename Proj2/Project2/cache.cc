/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b, int p, int pmax )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   interventions = invalidations = 0;
   busReads = busReadXs = 0;
   memtrans = flushes = C2Ctrans=0;
   num_p = p;
   num_pmax = pmax;

   //*******************//
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr,uchar op)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
   		if(op == 'w') newline->setFlags(MODIF);    
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') line->setFlags(MODIF);
	}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getFlags() == MODIF) writeBack(addr);
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(SHARE);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats()
{ 
	//printf("===== Simulation results      =====\n");
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
   cout<<"============ Simulation results (Cache "<<getNumP()<<") ============"<<endl;
   cout<<"01. number of reads:				"<<getReads()<<endl;
   cout<<"02. number of read misses:			"<<getRM()<<endl;
   cout<<"03. number of writes:				"<<getWrites()<<endl;
   cout<<"04. number of write misses:			"<<getWM()<<endl;
   cout<<"05. total miss rate:				"<<fixed<<setprecision(2)<<getMissRate()<<"%"<<endl;
   cout<<"06. number of writebacks:			"<<getWB()<<endl;
   cout<<"07. number of cache-to-cache transfers:		"<<getC2Ctrans()<<endl;
   cout<<"08. number of memory transactions:		"<<getMemTrans()<<endl;
   cout<<"09. number of interventions:			"<<getint()<<endl;
   cout<<"10. number of invalidations:			"<<getInv()<<endl;
   cout<<"11. number of flushes:				"<<getFlush()<<endl;
   cout<<"12. number of BusRdX:				"<<getBusRdXs()<<endl;
}

void Cache_MSI::Request_Processor(ulong addr,uchar op,Cache**& processors){
   
   BusRequest msg;
   currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;

	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		cacheLine *newline = fillLine(addr);
   	if(op == 'w'){                // write miss 
         writeMisses++;
         newline->setFlags(MODIF);  
         msg = BusRdX;
         memtrans++;

      }else{                        // read miss
         readMisses++;
         msg = BusRd;
         memtrans++;
      }  
	}
	else        /* hit */
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w'){                // write hit

         if( line->getFlags() == SHARE ) {    // notice other Pro to invalidate
            msg = BusRdX;
            memtrans++;
         } else {           // only Pi itself has block, silent write
            msg = None;
         }
         line->setFlags(MODIF);
      }else{                        // read hit
         msg = None;
      }
	}
   if(msg == BusRdX)
      busReadXs++;

   // broadcast to others
   if( None != msg){
      for(uint Pi=0;Pi<getNumPMAX();++Pi){
         if (getNumP()!=Pi)  
           processors[Pi]->Request_Bus(addr,msg);
      }
   }// end if hit/miss
}

void Cache_MSI::Request_Bus(ulong addr,BusRequest msg){

   cacheLine * line = findLine(addr);
   ulong Flag;
   
   if( line == NULL ){
      // do nothing
   } else {
      Flag = line->getFlags();

      if(MODIF == Flag ){ 
         if ( msg == BusRd ){       // M -> S
            line->setFlags(SHARE);
            interventions++;
            flushes++;
            writeBack(addr);
         }
         else if( msg == BusRdX ){  // M -> I
            line->invalidate();
            invalidations++;
            flushes++;
            writeBack(addr);
         }
      } else if( SHARE == Flag){
         if ( msg == BusRd ){       // S -> S
            // do nothing
         }
         else if( msg == BusRdX ){  // S -> I
            line->invalidate();
            invalidations++;
         }
      }
   }
}

void Cache_MESI::Request_Processor(ulong addr,uchar op,Cache**& processors){

   BusRequest msg;
   currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;

	cacheLine * line = findLine(addr);

	if(line == NULL)/*miss*/
	{  
      // broadcast if other processor contained request block
      cacheLine * line_other = NULL;
      for(uint Pi=0;Pi<getNumPMAX();++Pi){
         if (getNumP()!=Pi){  
            line_other = processors[Pi]->findLine(addr);
            if(line_other != NULL) break;
         }
      }  
      if(line_other!=NULL) C2Ctrans++;    // if Pi get block from other Pro, cache to cache transfer
      else                 memtrans++;    // if Pi get block from memory

		cacheLine *newline = fillLine(addr);
   	if(op == 'w'){                // write miss 
         writeMisses++;
         newline->setFlags(MODIF);  
         msg = BusRdX;

      }else{                        // read miss, braodcast & check if other processor has block
         readMisses++;
         msg = BusRd;
         if(line_other == NULL)  
               newline->setFlags(EXCLU);
	   }
   }
	else        /* hit */
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w'){                // write hit

         if( line->getFlags() == SHARE ) { 
            msg = BusUpgr;
            //memtrans++;     
            // don't need to do memory transaction bacause memory knows processor already has blocks
            // memory knows this is BusUpgr not BusRdX
         }
         else {  msg = None;}

         line->setFlags(MODIF);

      }else{                        // read hit
         msg = None;
      }
	}
   if(msg == BusRdX)
      busReadXs++;

   // broadcast to other Processors
   if( None != msg){
      for(uint Pi=0;Pi<getNumPMAX();++Pi){
         if (getNumP()!=Pi){  
           processors[Pi]->Request_Bus(addr,msg);
        }
      }
   }// end if hit/miss

}

void Cache_MESI::Request_Bus(ulong addr,BusRequest msg){
   cacheLine * line = findLine(addr);
   CS Flag;
   
   if( line == NULL ){
      // do nothing
   } else {
      Flag = line->getFlags();

      if( Flag == MODIF ){ 
         if ( msg == BusRd ){       // M -> S
            // flush
            line->setFlags(SHARE);
            interventions++;
            flushes++;
            writeBack(addr);
         }
         else if( msg == BusRdX ){  // M -> I
            // flush
            line->invalidate();
            invalidations++;
            flushes++;
            writeBack(addr);
         }
      }else if( Flag == EXCLU ){
         if ( msg == BusRd ){       // E -> S
            // flush opt
            line->setFlags(SHARE);
            interventions++;
         }else if( msg == BusRdX ){ // E -> I
            // flush opt
            line->invalidate();
            invalidations++;
         }
      } else if( Flag == SHARE ){
         if ( msg == BusRd ){       // S -> S
            // flush opt
         }
         else if( msg == BusRdX ){  // S -> I
            // flush opt
            line->invalidate();
            invalidations++;
         } else if ( msg == BusUpgr ){    // S -> I
            // no response
            line->invalidate();
            invalidations++;
         }
      }
   }
}

void Cache_Dragon::Request_Processor(ulong addr,uchar op,Cache**& processors){
   BusRequest msg;
   CS Flag;

   currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;

	cacheLine * line = findLine(addr);
   cacheLine * line_other= NULL;
   bool COPIES_EXIST = false;          // block been cached in other processor

   // broadcast if other processor contained request block
   for(uint Pi=0;Pi<getNumPMAX();++Pi){
      if (getNumP()!=Pi){  
         line_other = processors[Pi]->findLine(addr);
         if(line_other != NULL) {
            COPIES_EXIST = true;
            break;
         }
      }
   }
	if(line == NULL)/*miss*/
	{
		cacheLine *newline = fillLine(addr);
   	if(op == 'w'){   // Pr Write Miss 
         writeMisses++;
         memtrans++;
         if( COPIES_EXIST ){  // if other processor has block
            newline->setFlags(SMOD);  
            msg = BusUpd;
            // broadcast BusRd first, later Pi will broadcast Bus Update
            for(uint Pi=0;Pi<getNumPMAX();++Pi){
               if (getNumP()!=Pi)
                  processors[Pi]->Request_Bus(addr,BusRd);
            }
         }else {              // if no one has block
            newline->setFlags(MODIF);  
            msg = BusRd;
         }             
      }else{   // Pr Read Miss
         readMisses++;
         memtrans++;
         if( COPIES_EXIST ){  // if other processor has block
            msg = BusRd;
            newline->setFlags(SCLN);
         }else {              // if no one has block
            msg = BusRd;
            newline->setFlags(EXCLU);
         }
	   }
   }
	else        /* hit */
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w'){                // write hit
         Flag = line->getFlags();  
         switch ( Flag )
         {
         case EXCLU:
            msg = None;
            line->setFlags(MODIF);     // E -> M
            break;
         case SCLN:
            msg = BusUpd;
            if(!COPIES_EXIST)  line->setFlags(MODIF);  // Sc -> M
            else                line->setFlags(SMOD);  // Sc -> Sm            
            break;
         case SMOD:
            msg = BusUpd;
            if(!COPIES_EXIST)  line->setFlags(MODIF); // Sm -> M
            break;
         case MODIF:
            msg = None;
            break;
         default:
            msg = None;
            break;
         } // end of switch
      }else{                        // read hit
         msg = None;
      }
	}

   // broadcast to others
   if( None != msg){
      for(uint Pi=0;Pi<getNumPMAX();++Pi){
         if (getNumP()!=Pi)
           processors[Pi]->Request_Bus(addr,msg);
      }
   } // end of if msg

}

void Cache_Dragon::Request_Bus(ulong addr,BusRequest msg){

   cacheLine * line = findLine(addr);
   CS Flag;
   
   if( line == NULL ){
      // do nothing
   } else {
      Flag = line->getFlags();
      switch( Flag ){
         case EXCLU:
               if( msg == BusRd ){           // E -> Sc
                  // no response
                  line->setFlags(SCLN);
                  interventions++;               
               }
               break;
         case SCLN:
               if( msg == BusRd ){           // Sc -> Sc
                  // no response
                  //line->setFlags(SCLN);
               }else if ( msg == BusUpd ){   // Sc -> Sc
                  // update
                  line->setFlags(SCLN);   
               }   
               break;
         case SMOD:
               if( msg == BusRd ){           // Sm -> Sm
                  // flush
                  //line->setFlags(SMOD);
                  flushes++;
               }else if ( msg == BusUpd ){   // Sm -> Sc
                  // Update
                  line->setFlags(SCLN);   
               }
               break;
         case MODIF: 
               if ( msg == BusRd ){          // M -> Sm
                  // flush
                  line->setFlags(SMOD);
                  flushes++;
                  interventions++;
               } // impossible be snooped bus update
               break;
         default: break;
      }// end of switch
   } // end of not NULL
}
cacheLine *Cache_Dragon::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getFlags() == MODIF) writeBack(addr);
   if(victim->getFlags() == SMOD) writeBack(addr);

   tag = calcTag(addr);   
   victim->setTag(tag);
   //victim->setFlags(SHARE);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}
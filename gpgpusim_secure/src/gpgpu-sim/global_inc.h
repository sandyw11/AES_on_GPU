#ifndef GLOBAL_INC_H
#define GLOBAL_INC_H
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <bitset>

extern FILE *outRedirFile;
#define foreach(ii, NN) for(int ii=0; ii<NN; ii++)
#include <assert.h>
//#define RUN_DEBUG_CODE
//Since all caches use LRU, avoid checking for it
//#define MAY_USE_NON_LRU 


static const int LOCAL_COUNTER_MAX=7; //decay counter defined by SpM
static const int BYTE_SIZE_IN_BITS=8;
static const int NORMAL_CACHE_LINE_SIZE=128; //in byte. A compile time constant for allocating arrays, etc.
static const int NORMAL_CACHE_LINE_SIZE_BIT=NORMAL_CACHE_LINE_SIZE*BYTE_SIZE_IN_BITS; //in bit. 

typedef std::bitset<NORMAL_CACHE_LINE_SIZE_BIT> CacheDataType; 
void get_data_at(unsigned char * mydata,  /*new_addr_type*/ unsigned long long dataaddr, int datasize);
void get_data_as_bitset(unsigned long long  dataaddr, int dataSize, std::bitset<NORMAL_CACHE_LINE_SIZE_BIT> & finalData);

inline int get_bit_zeros(CacheDataType data)
{
	int zeros = data.size()-data.count();
	return zeros;
}

inline int get_bit_ones(CacheDataType data)
{
	int ones = data.count();
	return ones;
}

inline int get_bit_toggles(CacheDataType olddata, CacheDataType newdata)
{
 CacheDataType tempdata = olddata ^ newdata;
 return tempdata.count();
}

//SpM BITFLIP
inline int get_num_flipped_bits(CacheDataType olddata, CacheDataType newdata, int & changeInLowerBits, int & changeInUpperBits)
{
 int HalfSize = NORMAL_CACHE_LINE_SIZE_BIT/2;
 for(int ii=0; ii< HalfSize; ii++)
 {
  changeInLowerBits += (olddata[ii]^newdata[ii] ==1);
 }
 for(int jj=HalfSize; jj<NORMAL_CACHE_LINE_SIZE_BIT; jj++)
 {
  changeInUpperBits += (olddata[jj]^newdata[jj] ==1);
 }
 CacheDataType tempdata = olddata ^ newdata;
 assert(tempdata.count() == changeInUpperBits+changeInLowerBits); 
 return tempdata.count(); 
}
inline std::string TrueOrFalseString(bool value)
{
  return value? "TRUE":"FALSE";
}

static const  int NARROW_WIDTH_BYTE =8;
 #ifndef unlikely
#define unlikely(x) (__builtin_expect(!!(x), 0))
#endif
#ifndef likely
#define likely(x) (__builtin_expect(!!(x), 1))
#endif

inline void assert_bw_exclusive(int low, int val, int high)
{
  assert(low< val);
  assert(val< high);
}
inline std::string getEnvVar( std::string const & key ) 
{
  char * val = getenv( key.c_str() );
   return val == NULL ? std::string("") : std::string(val);
}
inline void print_newline_after(int times)
{
  if(times==0) return;
  static int nP=0;
  nP++;
  if(nP%times==0)
    printf("\n");
}
///inline void printDataFull(uint8_t *data, int blkSize)
inline void printDataFull(unsigned  char *data, int blkSize)
{

 for (int i=blkSize-1; i>=0; i--)
 { for (int j=7; j>=0; j--) 
   std::cout<< ((((unsigned char *)data)[i] >> (j)) & 0x01) ;
 }
 std::cout<<" ";
}

inline int countOne(unsigned char *data, int blkSize)
{
 int nNonZeroFound =0;
 for (int i=blkSize-1; i>=0; i--)
 { for (int j=7; j>=0; j--)
    {
     int toPrint = ((((unsigned char *)data)[i] >> (j)) & 0x01);
     nNonZeroFound += toPrint==1? 1:0;
    }
 }
 return nNonZeroFound;
}

inline void narrowDataCompression(unsigned char *dataHere, int blkSize, int & whichCompr)
{
 int bitHere;
  whichCompr = 0; //"Uncompressed";
 for (int i=blkSize-1; i>=NARROW_WIDTH_BYTE; i--)
 { for (int j=7; j>=0; j--)
   {
     bitHere = ((((unsigned char *)dataHere)[i] >> (j)) & 0x01);
     if(bitHere==1)
       {
        return ;        //1 has been found in leading bits
       }
   }
 }
 whichCompr = 1; //"NarrowVal"; //at least it is narrow val

  for (int i=NARROW_WIDTH_BYTE-1; i>=0; i--)
 { for (int j=7; j>=0; j--)
   {
     bitHere = ((((unsigned char *)dataHere)[i] >> (j)) & 0x01);
     if(bitHere==1)
       {
        return ;
       }
   }
 }
 whichCompr =  2; //"AllZero"; //it is zero val
}
 

inline int  getDataWidth(unsigned char *dataHere, int blkSize)
{
 int bitHere;
 for (int i=blkSize-1; i>=0; i--)
 { for (int j=7; j>=0; j--)
   { 
     bitHere = ((((unsigned char *)dataHere)[i] >> (j)) & 0x01);
     if(bitHere==1)
       {
        return i+1;        //1 has been found in leading bits
       }
   }
 }
  return 0;
}

inline int  getDataWidth(CacheDataType myData)
{  
 int localIndex =-1;
 for (int i=NORMAL_CACHE_LINE_SIZE-1; i>=0; i--)
 { for (int j=7; j>=0; j--)
   {
     localIndex = i*BYTE_SIZE_IN_BITS+j;      
     if(myData.test(localIndex)==1)
       {
        return i+1;        //1 has been found in leading bits
       }
   }
 }
  return 0;
}
inline void  findBitDiff(unsigned char  * dataOld, const unsigned char * dataNew, unsigned blkSize, int & nBitsSame)
{
  int runningIndex =0;
  for (int i=0; i<blkSize; i++)
   {
     unsigned char oldDataHere = ((unsigned char *)dataOld)[i];
     unsigned char newDataHere = ((unsigned char *)dataNew)[i];
     for (int j=0; j<8; j++)
     {
       int OldBit__ = ((oldDataHere  >> j)& 0x01);
       int NewBit__ = ((newDataHere >> j) & 0x01);
       //assert(OldBit__==bitOld);
       //assert(NewBit__==bitNew);
       if(OldBit__ == NewBit__)
       {
        nBitsSame ++; 
       }
       runningIndex++;
     }
   }
}

inline bool seeWithinBlockSimilarity(unsigned char *dataHere, int blkSize)
{
 bool areAllSame=true; //has to be true SpM, proof by contradiction
 ///std::cout<<" came to seeWithinBlockSimilarity \n";
 int  size = blkSize;
 int step = 4;
 int nValues = size/step;
 long long unsigned * values = new long long unsigned[size / step];
 for (unsigned int i = 0; i < size / step; i++) {
  values[i] = 0; // Initialize all elements to zero.
 }
 for (unsigned i = 0; i < size; i += step) {
  for (unsigned j = 0; j < step; j++) {
    values[i / step] += (long long unsigned) ((unsigned char) dataHere[i+j]) << (8 * j);
  }
 }
#if 0
 std::cout<<" Printing raw data \n";
 // foreach(i, blkSize)
 //{ foreach (j,8)
   for (int i=blkSize-1; i>=0; i--)
  { for (int j=7; j>=0; j--)
    {
     int toPrint = ((((unsigned char *)dataHere)[i] >> (j)) & 0x01);
     std::cout<<toPrint;
    }
   if(i!=0 && (i+1)%4==0)
    std::cout<<"\n";
 }
 
#endif
 //assert(0);
 for(int qp=0; qp< nValues-1; qp++)
  {
   if( values[qp] !=values[qp+1])
     {
       areAllSame=false; 
       break;  
     }
  }
#if 0
 if(not areAllSame)
 {
  std::cout<<"NOTALLSAME ";
  foreach(qq, nValues)
  {
   std::cout<<" "<<values[qq];
   //std::cout<<" values["<<qq<<"] ="<<values[qq]<<" \t";
   //if(qq!=0 && qq%2==0)
   // std::cout<<"\n";  
  }
  std::cout<<"\n"; 
 }
#endif
 
  delete [] values;
  return areAllSame;
}

inline bool seeWithinBlockSimilarity(CacheDataType myData)
{
 bool areAllSame=true;
 int nValues = 1024/32;
 for(int qp=0; qp< nValues-1; qp++)
  {
    for(int i=0; i<32; i++)
      if(myData.test(qp*32+i) != myData.test( (qp+1)*32+i))
      {
       areAllSame=false;
       break;
     }
  }
 return areAllSame;
}
#endif


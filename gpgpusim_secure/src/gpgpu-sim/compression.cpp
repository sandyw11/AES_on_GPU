#include <iostream>
#include <vector>
#include "c-pack.c"

void test (void) {
     std::cout << "test" << std::endl;

}

long long unsigned * convertBuffer2Array (char * buffer, unsigned size, unsigned step)
{
      long long unsigned * values = new long long unsigned [size/step];
//      std::cout << std::dec << "ConvertBuffer = " << size/step << std::endl;
     //init 
     for (unsigned int i = 0; i < size / step; i++) {
          values[i] = 0;    // Initialize all elements to zero.
      }
      //SIM_printf("Element Size = %d \n", step);
      for ( unsigned i = 0; i < size; i += step ){
          for ( unsigned j = 0; j < step; j++){
              //SIM_printf("Buffer = %02x \n", (unsigned char) buffer[i + j]);
              values[i / step] += (long long unsigned)((unsigned char)buffer[i + j]) << (8*j);
              //SIM_printf("step %d value = ", j);
              //printLLwithSize(values[i / step], step);  
          }
          //std::cout << "Current value = " << values[i / step] << std::endl;
          //printLLwithSize(values[i / step], step);
          //SIM_printf("\n");
      }
      //std::cout << "End ConvertBuffer = " << size/step << std::endl;
      return values;
}

///
/// Check if the cache line consists of only zero values
///
bool isZeroPackable ( long long unsigned * values, unsigned size){
  bool nonZero = 0;
  for (unsigned int i = 0; i < size; i++) {
      if( values[i] != 0){
          nonZero = 1;
          break;
      }
  }
  return !nonZero;
}

///
/// Check if the cache line consists of only same values
///
bool isSameValuePackable ( long long unsigned * values, unsigned size){
  bool notSame = 0;
  for (unsigned int i = 0; i < size; i++) {
      if( values[0] != values[i]){
          notSame = 1;
          break;
      }
  }
  return !notSame;
}

///
/// Check if the cache line values can be compressed with multiple base + 1,2,or 4-byte offset 
/// Returns size after compression 
///
unsigned doubleExponentCompression ( long long unsigned * values, unsigned size, unsigned blimit, unsigned bsize){
  unsigned long long limit = 0;
  //define the appropriate size for the mask
  switch(blimit){
    case 1:
      limit = 56;
      break;
    case 2:
      limit = 48;
      break;
    default:
      std::cout << "Wrong blimit value = " <<  blimit << std::endl;
      exit(1);
  }
  // finding bases: # BASES
  // find how many elements can be compressed with mbases
  unsigned compCount = 0;
  for (unsigned i = 0; i < size; i++) {
         if( (values[0] >> limit) ==  (values[i] >> limit))  {
             compCount++;
         }
  }
  //return compressed size
  if(compCount != size )
     return size * bsize;
  return size * bsize - (compCount - 1) * blimit;
}


///
/// Check if the cache line values can be compressed with multiple base + 1,2,or 4-byte offset 
/// Returns size after compression 
///
unsigned multBaseCompression ( long long unsigned * values, unsigned size, unsigned blimit, unsigned bsize){
  unsigned long long limit = 0;
  unsigned BASES = 2;
  //define the appropriate size for the mask
  switch(blimit){
    case 1:
      limit = 0xFF;
      break;
    case 2:
      limit = 0xFFFF;
      break;
    case 4:
      limit = 0xFFFFFFFF;
      break;
    default:
      std::cout << "Wrong blimit value = " <<  blimit << std::endl;
      exit(1);
  }
  // finding bases: # BASES
  std::vector<unsigned long long> mbases;
  mbases.push_back(values[0]); //add the first base
  for (unsigned i = 0; i < size; i++) {
      for(unsigned j = 0; j <  mbases.size(); j++){
         if( llabs((long long int)(mbases[j] -  values[i])) > limit ){
             mbases.push_back(values[i]); // add new base
         }
     }
     if(mbases.size() >= BASES) //we don't have more bases
       break;
  }
  // find how many elements can be compressed with mbases
  unsigned compCount = 0;
  for (unsigned i = 0; i < size; i++) {
      //ol covered = 0;
      for(unsigned j = 0; j <  mbases.size(); j++){
         if( llabs((long long int)(mbases[j] -  values[i])) <= limit ){
             compCount++;
             break;
         }
     }
  }
  //return compressed size
  unsigned mCompSize = blimit * compCount + bsize * BASES + (size - compCount) * bsize;
  //if(mCompSize > 58)
  //  return _blockSize;
  //SIM_printf("%d-bases bsize = %d osize = %d CompCount = %d CompSize = %d\n", BASES, bsize, blimit, compCount, mCompSize);
  return mCompSize;
}

///
/// Check if the cache line values can be compressed with multiple base + 1,2,or 4-byte offset 
/// Returns size after compression 
///
unsigned singleBaseCompression ( long long unsigned * values, unsigned size, unsigned blimit, unsigned bsize){
  unsigned long long limit = 0;
  unsigned BASES = 1;
  //define the appropriate size for the mask
  switch(blimit){
    case 1:
      limit = 0xFF;
      break;
    case 2:
      limit = 0xFFFF;
      break;
    case 4:
      limit = 0xFFFFFFFF;
      break;
    default:
      std::cout << "Wrong blimit value = " <<  blimit << std::endl;
      exit(1);
  }
  // finding bases: # BASES
  std::vector<unsigned long long> mbases;
  mbases.push_back(values[0]); //add the first base
  // find how many elements can be compressed with mbases
  unsigned compCount = 0;
  for (unsigned i = 0; i < size; i++) {
      //ol covered = 0;
      for(unsigned j = 0; j <  mbases.size(); j++){
         if( llabs((long long int)(mbases[j] -  values[i])) <= limit ){
             compCount++;
             break;
         }
     }
  }
  //return compressed size
  if (size != compCount)
     return size * bsize;	  
  unsigned mCompSize = blimit * compCount + bsize * BASES;
  //if(mCompSize > 58)
  //  return _blockSize;
  //SIM_printf("%d-bases bsize = %d osize = %d CompCount = %d CompSize = %d\n", BASES, bsize, blimit, compCount, mCompSize);
  return mCompSize;
}


unsigned BDICompressSingle (char * buffer, unsigned _blockSize)
{
  //char * dst = new char [_blockSize];
  long long unsigned * values = convertBuffer2Array( buffer, _blockSize, 8);
  unsigned bestCSize = _blockSize;
  unsigned currCSize = _blockSize;
  if( isZeroPackable( values, _blockSize / 8))
      bestCSize = 1;
  if( isSameValuePackable( values, _blockSize / 8))
      currCSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = singleBaseCompression( values, _blockSize / 8, 1, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = singleBaseCompression( values, _blockSize / 8, 2, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = singleBaseCompression( values, _blockSize / 8, 4, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  values = convertBuffer2Array( buffer, _blockSize, 4);
  if( isSameValuePackable( values, _blockSize / 4))
      currCSize = 4;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = singleBaseCompression( values, _blockSize / 4, 1, 4);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = singleBaseCompression( values, _blockSize / 4, 2, 4);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  values = convertBuffer2Array( buffer, _blockSize, 2);
  currCSize = singleBaseCompression( values, _blockSize / 2, 1, 2);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  //exponent base compression
  values = convertBuffer2Array( buffer, _blockSize, 8);
  currCSize = doubleExponentCompression( values, _blockSize / 8, 2, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = doubleExponentCompression( values, _blockSize / 8, 1, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  
  delete [] values;
  //delete [] buffer;
  buffer = NULL;
  values = NULL;
  //SIM_printf(" BestCSize = %d \n", bestCSize);
  return bestCSize;

}

unsigned BDICompress4BOnly (char * buffer, unsigned _blockSize)
{
  //char * dst = new char [_blockSize];
  long long unsigned * values = convertBuffer2Array( buffer, _blockSize, 8);
  unsigned bestCSize = _blockSize;
  unsigned currCSize = _blockSize;
  if( isZeroPackable( values, _blockSize / 8))
      bestCSize = 1;
  if( isSameValuePackable( values, _blockSize / 8))
      currCSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 8, 1, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 8, 2, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize =  multBaseCompression( values, _blockSize / 8, 4, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  values = convertBuffer2Array( buffer, _blockSize, 4);
  if( isSameValuePackable( values, _blockSize / 4))
      currCSize = 4;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 4, 1, 4);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 4, 2, 4);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  //exponent base compression
  values = convertBuffer2Array( buffer, _blockSize, 8);
  currCSize = doubleExponentCompression( values, _blockSize / 8, 2, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = doubleExponentCompression( values, _blockSize / 8, 1, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  
  delete [] values;
  //delete [] buffer;
  buffer = NULL;
  values = NULL;
  //SIM_printf(" BestCSize = %d \n", bestCSize);
  return bestCSize;

}

unsigned BDICompress8BOnly (char * buffer, unsigned _blockSize)
{
  //char * dst = new char [_blockSize];
  long long unsigned * values = convertBuffer2Array( buffer, _blockSize, 8);
  unsigned bestCSize = _blockSize;
  unsigned currCSize = _blockSize;
  if( isZeroPackable( values, _blockSize / 8))
      bestCSize = 1;
  if( isSameValuePackable( values, _blockSize / 8))
      currCSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 8, 1, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 8, 2, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize =  multBaseCompression( values, _blockSize / 8, 4, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  //exponent base compression
  values = convertBuffer2Array( buffer, _blockSize, 8);
  currCSize = doubleExponentCompression( values, _blockSize / 8, 2, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = doubleExponentCompression( values, _blockSize / 8, 1, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  
  delete [] values;
  //delete [] buffer;
  buffer = NULL;
  values = NULL;
  //SIM_printf(" BestCSize = %d \n", bestCSize);
  return bestCSize;

}

unsigned BDICompressSpecial (char * buffer, unsigned _blockSize, unsigned& baseSize)
{
  //char * dst = new char [_blockSize];
  long long unsigned * values = convertBuffer2Array( buffer, _blockSize, 8);
  unsigned bestCSize = _blockSize;
  unsigned currCSize = _blockSize;
  baseSize = 8;
  if( isZeroPackable( values, _blockSize / 8))
  {	  
      bestCSize = 1;
      baseSize = 0;
  }
  if( isSameValuePackable( values, _blockSize / 8))
  {	  
      currCSize = 8;
      baseSize = 8;
  }
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 8, 1, 8);
  if(bestCSize > currCSize) baseSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 8, 2, 8);
  if(bestCSize > currCSize) baseSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize =  multBaseCompression( values, _blockSize / 8, 4, 8);
  if(bestCSize > currCSize) baseSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  values = convertBuffer2Array( buffer, _blockSize, 4);
  if( isSameValuePackable( values, _blockSize / 4))
  {	  
      currCSize = 4;
      baseSize = 4;
  }
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 4, 1, 4);
  if(bestCSize > currCSize) baseSize = 4;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 4, 2, 4);
  if(bestCSize > currCSize) baseSize = 4;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  values = convertBuffer2Array( buffer, _blockSize, 2);
  currCSize = multBaseCompression( values, _blockSize / 2, 1, 2);
  if(bestCSize > currCSize) baseSize = 2;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  //exponent base compression
  values = convertBuffer2Array( buffer, _blockSize, 8);
  currCSize = doubleExponentCompression( values, _blockSize / 8, 2, 8);
  if(bestCSize > currCSize) baseSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = doubleExponentCompression( values, _blockSize / 8, 1, 8);
  if(bestCSize > currCSize) baseSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  
  delete [] values;
  //delete [] buffer;
  buffer = NULL;
  values = NULL;
  //SIM_printf(" BestCSize = %d \n", bestCSize);
  return bestCSize;

}

unsigned BDICompress (char * buffer, unsigned _blockSize)
{
  //char * dst = new char [_blockSize];
  long long unsigned * values = convertBuffer2Array( buffer, _blockSize, 8);
  unsigned bestCSize = _blockSize;
  unsigned currCSize = _blockSize;
  if( isZeroPackable( values, _blockSize / 8))
      bestCSize = 1;
  if( isSameValuePackable( values, _blockSize / 8))
      currCSize = 8;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 8, 1, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 8, 2, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize =  multBaseCompression( values, _blockSize / 8, 4, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  values = convertBuffer2Array( buffer, _blockSize, 4);
  if( isSameValuePackable( values, _blockSize / 4))
      currCSize = 4;
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 4, 1, 4);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = multBaseCompression( values, _blockSize / 4, 2, 4);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  values = convertBuffer2Array( buffer, _blockSize, 2);
  currCSize = multBaseCompression( values, _blockSize / 2, 1, 2);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  delete [] values;
  //exponent base compression
  values = convertBuffer2Array( buffer, _blockSize, 8);
  currCSize = doubleExponentCompression( values, _blockSize / 8, 2, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  currCSize = doubleExponentCompression( values, _blockSize / 8, 1, 8);
  bestCSize = bestCSize > currCSize ? currCSize: bestCSize;
  
  delete [] values;
  //delete [] buffer;
  buffer = NULL;
  values = NULL;
  //SIM_printf(" BestCSize = %d \n", bestCSize);
  return bestCSize;

}

unsigned FPCCompress(char * buffer, unsigned size ){
   long long unsigned * values = convertBuffer2Array(buffer, size*4, 4);
   unsigned compressable = 0;
   for (unsigned int i = 0; i < size; i++) {
     
     //SIM_printf("c_size = %llx \n", compressable);
     //SIM_printf("V = %llx \n", values[i]);
     // 000
     if(values[i] == 0){
        compressable += 0;//SIM_printf("000\n ");
        continue;
     }
     // 001 010
     if(abs((int)(values[i])) <= 0xFF){
        compressable += 1;//SIM_printf("001\n ");
        continue;
     }
     // 011
     if(abs((int)(values[i])) <= 0xFFFF){
        compressable += 2;//SIM_printf("011\n ");
        continue;
     }
     //100  
     if(((values[i]) & 0xFFFF) == 0 ){
        compressable += 2;//SIM_printf("100\n ");
        continue;
     }
     //101
     if( abs((int)((values[i]) & 0xFFFF)) <= 0xFF
         && abs((int)((values[i] >> 16) & 0xFFFF)) <= 0xFF){
        compressable += 2;//SIM_printf("101\n ");
        continue;
     }
     //110
     unsigned byte0 = (values[i]) & 0xFF;
     unsigned byte1 = (values[i] >> 8) & 0xFF;
     unsigned byte2 = (values[i] >> 16) & 0xFF;
     unsigned byte3 = (values[i] >> 24) & 0xFF;
     if(byte0 == byte1 && byte0 == byte2 && byte0 == byte3){
        compressable += 1; //SIM_printf("110\n ");
        continue;
     }
     //111
     compressable += 4;
     //SIM_printf("111\n ");
   }
   delete [] values;
   //6 bytes for 3 bit per every 4-byte word in a 64 byte cache line
   unsigned compSize = compressable + size * 3 / 8;
   if(compSize < size * 4)
      return compSize;
   else
      return size * 4;
}


unsigned CPack(unsigned char * buffer, unsigned blockSize)
{
   DirectoryEntry * directory = new DirectoryEntry[blockSize/4 * 8];
   //printf("Init directory size: %d \n", blockSize);
   initialize_dic(directory);
   bool zeroBlock = false;
   long long unsigned * values = convertBuffer2Array((char *) buffer, blockSize, 4);
	int types[6] = {0,0,0,0,0,0};
   unsigned compSize = calculate_compressed_size_c_pack(directory, blockSize/4, values, &zeroBlock,types);
   delete [] values;
   delete [] directory;
	printf("****** ----- *******\n");
	for(int i = 0; i < 6; i++)
		printf("%d,",types[i]);
	printf("\n");
   if(zeroBlock)
       return 1;
   else
       return (compSize + 7)/8; // convert size into bits
   //delete [] directory;
}


unsigned GeneralCompress (char * buffer, unsigned _blockSize, unsigned compress)
{   // compress is the actual compression algorithm
   switch (compress)
   {
        case 0:
           return _blockSize;
           break; 
        case 1:	
   	   return BDICompress(buffer, _blockSize);
           break;
        case 2:
           //std::cout << "block-size: " << _blockSize << "\n"; 
           return FPCCompress(buffer, _blockSize/4);
           break;
        case 3:
        {
           unsigned BDISize = BDICompress(buffer, _blockSize);
           unsigned FPCSize = FPCCompress(buffer, _blockSize/4);
           if(BDISize <= FPCSize) {
		printf("BDI\n");
              return BDISize;
		}
	   else
	{
		printf("FPC\n");
              return FPCSize;
	}
           break;
        }
	case 4:
	{
	   unsigned CPackSize = CPack((unsigned char *)buffer, _blockSize);
	   return CPackSize;
          // return _blockSize;
	   break;
	}
	case 5:
	{
	   unsigned BDISize = BDICompress(buffer, _blockSize);
           unsigned FPCSize = FPCCompress(buffer, _blockSize/4);
	   unsigned CPackSize = CPack((unsigned char *)buffer, _blockSize);
	
           if(BDISize <= FPCSize && BDISize <= CPackSize)
              return BDISize;
           if(FPCSize <= BDISize && FPCSize <= CPackSize)
	      return FPCSize;
	   if(CPackSize <= BDISize && CPackSize <= FPCSize)
	      return CPackSize;
	}
         case 6:	
   	   return BDICompressSingle(buffer, _blockSize);
           break;
         case 7:	
   	   return BDICompress4BOnly(buffer, _blockSize);
           break;
         case 8:	
   	   return BDICompress8BOnly(buffer, _blockSize);
           break;
        

	      
        default:
          std::cout << "Unknown compression code: " << compress << "\n";
          exit(1);         	
   }
}

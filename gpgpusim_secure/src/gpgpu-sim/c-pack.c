#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

static unsigned numEntries = 32;
using namespace std;

struct DirectoryEntry {
  bool m_valid;
  unsigned int m_entry;
  int m_cnt;
};


bool match_dic_mmmm(DirectoryEntry* directory, const unsigned int & w)
{
  for (int i=0; i<numEntries; i++){
    if ((directory[i]).m_valid && ((directory[i]).m_entry == w))
      return true;
  }
  return false;
}

bool match_dic_mmmx(DirectoryEntry* directory, const unsigned int & w)
{
  for (int i=0; i<numEntries; i++){
    if ((directory[i]).m_valid && (((directory[i]).m_entry & 0xFFFFFF00) == (w & 0xFFFFFF00)))
      return true;
  }
  return false;

}

bool match_dic_mmxx(DirectoryEntry* directory, const unsigned int & w)
{
  for (int i=0; i<numEntries; i++){
    if ((directory[i]).m_valid && (((directory[i]).m_entry & 0xFFFF0000) == (w & 0xFFFF0000)))
      return true;
  }
  return false;

}

bool insert_dic(DirectoryEntry* directory, const unsigned int & w)
{
  int count = 0;
  for (int i=0; i<numEntries; i++){
    if((directory[i]).m_valid)
      count++;
  }

  //search for an invalid entry
  for (int i=0; i<numEntries; i++){
    if (!((directory[i]).m_valid)) {
      (directory[i]).m_valid = true;
      (directory[i]).m_entry = w;
      (directory[i]).m_cnt = count;
      return true;
    }
  }

  bool test = false;
  //replace the oldest one
  for (int i=0; i<numEntries; i++){
    if ((directory[i]).m_cnt == 0) { //the oldest one
      (directory[i]).m_valid = true;
      (directory[i]).m_entry = w;
      (directory[i]).m_cnt = count;
      test = true;
    }
  }

  //update counters
  for (int i=0; i<numEntries; i++){
    (directory[i]).m_cnt = (directory[i]).m_cnt - 1;
  }

  if (!test)
    cout << "ERROR: in insert directory, it could not find the oldest entry" << endl;

  return true;
}

void initialize_dic(DirectoryEntry* directory)
{
  for (int i=0; i<numEntries; i++){
    directory[i].m_valid = false;
    directory[i].m_cnt = 0;
  }
}



int calculate_compressed_size_c_pack(DirectoryEntry* directory, int numwords, unsigned long long * words, bool* zero_block, int *type){
      *zero_block = true;
      numEntries = numwords;
      for (int i=0; i<numwords; i++){
        if((words[i])){
	  *zero_block = false;
	}
      }
	if(*zero_block) 
		type[0] += numwords;
      int lineBits = 0;

      if (!(*zero_block)) { //calculate compressed data size
        int wordindex = 0;
        while (wordindex < numwords) {
          //'zzzz' --> 2 bits
          if ((words[wordindex]) == 0) {  // zero run
            lineBits += 2;
		
          }

	  else if (((words[wordindex]) & 0xFFFFFF00) == 0) { //'zzzx' --> 12
            lineBits += 12;
		type[1] += 1;
          } else if (match_dic_mmmm(directory, (words[wordindex]))) { // 'mmmm' --> 6
            lineBits += 6;
		type[2] += 1;
          } else if (match_dic_mmmx(directory, (words[wordindex]))) { // 'mmmx' --> 16
		type[3] += 1;
            lineBits += 16;
          } else if (match_dic_mmxx(directory, (words[wordindex]))) { // 'mmxx' --> 24
		type[4] += 1;
            lineBits += 24;
          } else { //'xxxx' --> 34
		type[5] += 1;
            lineBits += 34;
            insert_dic(directory, (words[wordindex]));
          }
          wordindex ++;
        }
      }

   if (lineBits > numwords * 4 * 8)
       return numwords * 4 * 8;

   return lineBits;
}


#include "gpu-sim.h"
#include "gpu-misc.h"
#include "dram.h"
#include "mem_latency_stat.h"
#include "dram_sched.h"
#include "mem_fetch.h"
#include "l2cache.h"
#include "../cuda-sim/memory.h"

extern unsigned char *global_memory; // Pointer to memory image of data: need it for compression.

// data value analysis
void dram_req_t::data_value_analysis(mem_fetch *data){
  const memory_config* m_config = data->get_mem_config();

  assert(m_config->dva);    
  char * mydata = new char[data->get_data_size()];
  new_addr_type address_limit;

  address_limit = data->get_addr() + data->get_data_size(); 
  
  if(address_limit <= 0x100000000){
    ((memory_space_impl<16*1024> *)global_memory) -> read((data->get_addr()>> 7) << 7, data->get_data_size(), mydata);
  }
  else {
    printf("out of memory bound of 4gb\n");
  }

  convertData2Array(mydata, data->get_data_size(), 8);

  delete [] mydata;
}

long long unsigned * dram_req_t::convertData2Array (char * buffer, unsigned size, unsigned step) {
    long long unsigned * values = new long long unsigned [size/step];
    for (unsigned int i = 0; i < size / step; i++) {
        values[i] = 0;    // Initialize all elements to zero.
    }
    for ( unsigned i = 0; i < size; i += step ){
        for ( unsigned j = 0; j < step; j++){
            printf("Buffer = %02x, Size = %u \n", (unsigned char) buffer[i + j], size);
            values[i / step] += (long long unsigned)((unsigned char)buffer[i + j]) << (8*j);
        }
        // std::cout << "Current value = " << values[i / step] << std::endl;

    }
    return values;
}

void dram_t::analyze_bit_toggles(mem_fetch *newdata) {
        CacheDataType myNewData;
        get_data_as_bitset(newdata->get_addr(), NORMAL_CACHE_LINE_SIZE,  myNewData);

	// for dram: bit toggles is defined as number of zeros transferred (As per defination of hpca paper)
	// this defination is weird but anyways...
	
	int bit_toggles = get_bit_toggles(myOldData, myNewData);
	int bit_diff, rounded_bit_diff;
	myOldData = myNewData;
	
	int bitzeros = get_bit_zeros(myNewData);
	int bitones = get_bit_ones(myNewData);
	
	tot_bit_toggles += bit_toggles;
	tot_bit_zeros += bitzeros;
	tot_bit_ones += bitones;

	bit_diff = abs(bitzeros-bitones);
	rounded_bit_diff = (bit_diff + 50) / 100 * 100;
	assert((rounded_bit_diff/100) <= 10);
	hist_bit_diff[rounded_bit_diff/100]++;

	tot_bit_abs_diff += bit_diff;
}

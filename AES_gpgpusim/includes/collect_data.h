#ifndef COLLECT_DATA_H
#define COLLECT_DATA_H

#define KB 1024
#define NUM_KEY 16
#define CACHE_LINE_SIZE 64
#define L1_CACHE_SIZE 32*KB
#define AVG_SAMPLE_SIZE 0x100
#define CLIP_RATIO 2

#ifdef __i386__
#  define RDTSC_DIRTY "%eax", "%ebx", "%ecx", "%edx"
#elif __x86_64__
#  define RDTSC_DIRTY "%rax", "%rbx", "%rcx", "%rdx"
#else
# error unknown platform
#endif

#define RDTSC_START(cycles)                                \
	do {                                                   \
		register unsigned cyc_low;               \
		asm volatile("CPUID\n\t"                           \
				"RDTSC\n\t"                           \
				"mov %%eax, %0\n\t"                   \
				: "=r" (cyc_low)     \
				:: RDTSC_DIRTY);                      \
		(cycles) = cyc_low;   \
	} while (0)

#define RDTSC_STOP(cycles)                                 \
	do {                                                   \
		register unsigned cyc_low;               \
		asm volatile("RDTSCP\n\t"                          \
				"mov %%eax, %0\n\t"                   \
				"CPUID\n\t"                           \
				: "=r" (cyc_low)     \
				:: RDTSC_DIRTY);                      \
		(cycles) = cyc_low;   \
	} while(0)

int touch(char*, int len);

struct timing_pair{
	unsigned int time;
	char cipher[16];	
};

#endif

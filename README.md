# AES-on-GPU

This project contains the experiment results of three benchmarks, AES application in the GPGPU-sim, cache micro benchmark, and the AES benchmark used in the paper.


1.AES application in the GPGPU-sim
The configuration of this application uses L1 and L2 caches. 

Different values of the first key byte are set, including 0, 1, 2, 3, 4, 5, 6, 15, 31, 63, 127, 255. Coalescing means using the original AES application in the GPGPU-sim with coalescing. No coalescing means disabling the entire coalescing in the AES application. In the dynamic coalescing, a variable is set to be 16, which means 16 threads are using coalescing and the other 16 threads are without coalescing. 

In the comparison of the results, the L2 cache miss rate is vary for different values of the first key byte with coalescing, but is stable without coalescing. The results will be different from each other when running the AES application using dynamic coalescing several times. Same phenomenon can be seen in the results of global memory write.

The result of experiments without using L1 and L2 caches will be added.


2.Cache micro benchmark
The configuration of the cache micro benchmark does not use L1 and L2 caches. 

As the number of threads increasing, the cycle of the benchmark increases with coalescing, while the cycle of the benchmark is stable without coalescing. However, the execution time(2609) without coalescing is much longer than using 32 threads with coalescing(1875). Same phenomenon can be seen in the results of global memory read(100 vs 65). The global memory write is stable both with and without coalescing. 

In the results of PTX files of using 32 threads with and without coalescing, it can be seen that the global memory read increases from 1 to 32 in line 96, 73, 83, 90. These lines cause the large gap of the execution time between using 32 threads with coalescing and without coalescing.

In the simple version of cache micro benchmark, the cycle of using 32 threads is 1504 with coalescing, and is 1777 without coalescing. From the PTX results, it can be seen that the global memory read increases from 1 to 32 in line 80, which causes the large gap of the execution time between using 32 threads with coalescing(1504) and without coalescing(1777). Only in line 77, the global memory read is both 32 with and without coalescing. It can be concluded that only disabling coalescing in line 77 and using coalescing in other lines can reduce the  
large gap. In the result of dis-77, it can be seen that the cycle is 1504, which is same as the execution time between using 32 threads with coalescing.


3.AES benchmark of the paper
The configuration of the AES benchmark of the paper does not use L1 and L2 caches. 

32 threads are used during the execution of AES benchmark.

In the PTX file, it showed that there are 16 global memory accesses in each round during the AES encryption. Totally, there are 160 global memory accesses in the 10 rounds during the execution of AES every time. However, the global memory accesses in Round 1 to Round 9 may be overlapped because of the dependence of different cypher text bytes. So I disable coalescing of the 16 global memory accesses in the last round, and compare the results of execution time and global read. 

In the Comp file, it can be seen that the cycle(266820) of disabling coalescing of the last round slightly increases comparing to the cycle(264761) using coalescing. Same phenomenon can be seen in the results of global read, which increases from 2774 to 3062. In comparison with the cycle(313558) and the read(14656) of the experimental results totally without coalescing, the large gap of the execution time between using 32 threads with coalescing and without coalescing is reduced by only disabling coalescing in the last round.

In the Read file, 256 values of the first key byte are set, from 0 to 255. The corresponding results of global read are compared between using coalescing, without coalescing and disabling coalescing in the last round. The correlation between different values of the first key byte and the global read using coalescing is about 0.11. Since the global read without coalescing is stable, its correlation with different values is 0. After disabling coalescing in the last round, the correlation is reduced to 0.009, which is only 8% of using coalescing. In this way, the correlation of different input values of the encryption key and global memory accesses are destroyed, which can protect AES encryption from attack of correct guessing the encryption key.

In the Uniquecache file, the 12th byte of encrypt key is set to 12. I produce the scatter diagram of the unique cache line requests in the last round and the total execution time for 100 samples and get the fitting result using matlab, which is the Figure 7 in the paper. The fitting line has a slope of 7.79 cycles and offset of 264679 cycles. This demonstrates that the execution time of AES encryption has linear correlation with the unique cache line requests in the last round. 

heap_PQ.h  		---> contains the implementation for Heap Based Implementation of Concurrent PQ
skiplist_PQ.h  	---> contains the implementation for Skiplist Based Implementation of Concurrent PQ

ParDijk-heap.cpp 		---> parallel implementation of Dijkstra's SSSP algorithm using Heap-Based Concurrent PQ
ParDijk-skiplist.cpp 	---> parallel implementation of Dijkstra's SSSP algorithm using Skiplist-Based Concurrent PQ

core-ops-heap.cpp		---> Test run code for calculating Core Operations performance
core-ops-skiplist.cpp

Compiler Instructions:
-----------------------

g++ -std=c++14 <cpp> -fpermissive -lpthread


Detailed Instructions:
---------------------

In ParDijk.cpp and ParDijk-skip.cpp
		-- the graphs generated can be controlled by the parameters |V| and |E|
				- in code (MAX_VERTICES(400 -default), MAX_EDGES(400*50 -default))
		-- No of threads is given as a cmdline argument (./a.out 10)

In core-ops-<>.cpp
		-- Run to get the avg. time taken to execute the extractMin, insert operations of PQ 
			(When No of nodes in PQ = (approx) 10^5) with N threads.
		-- N and K are given as cmdline arguments (./a.out 10 100)
				- N is no of threads , K is no of operations executed by each thread
				- each operation executed is choosen randomly between insert() and extractMin()
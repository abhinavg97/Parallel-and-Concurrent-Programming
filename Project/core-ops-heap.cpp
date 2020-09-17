// linux lib
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

// C++ lib
#include <vector>
#include <random>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <cmath>
#include <fstream>
#include <climits>
#include <mutex>          // std::mutex

#include "heap_PQ.h"
using namespace std;
using namespace std::chrono;

int n, k, lamda1, lamda2;
volatile long long totalInsertTime = 0, totalExtractTime = 0;
volatile int WorstInsertTime = INT_MIN, WorstExtractTime = INT_MIN;
int totalInserts = 0, totalExtracts = 0;

mutex lock1;

double ran_exp(float lamda)
{
	default_random_engine generate;
	exponential_distribution<double> distribution(lamda);
	return distribution(generate);
}

void testCS(int threadId)
{
	int first=0;
	int second = 0;
	for(int i=0;i<k;++i)
	{
		// cout<<flush<<threadId<<" "<<i<<endl;
		if(((rand()%2==0 || first == 0) && (second>0) && (totalInserts - totalExtracts < 1000)) || ((totalExtracts - totalInserts) >1000) )
		{	
		// if(1){
			first++;
			auto start = high_resolution_clock::now();
			insert(rand()%INT_MAX, Data(threadId));
			auto stop = high_resolution_clock::now(); 
			auto duration = duration_cast<microseconds>(stop - start);
			int time_taken = duration.count();
			lock1.lock();
			totalInsertTime += time_taken;
			if(time_taken > WorstInsertTime)
				WorstInsertTime = time_taken;
			totalInserts++;
			lock1.unlock();
		}
		else
		{
			second++;
			auto start = high_resolution_clock::now();
			extractMin();
			auto stop = high_resolution_clock::now(); 
			auto duration = duration_cast<microseconds>(stop - start);
			int time_taken = duration.count();
			lock1.lock();
			totalExtractTime += time_taken;
			if(time_taken > WorstExtractTime)
				WorstExtractTime = time_taken;	
			totalExtracts++;
			lock1.unlock();
		}
	}
}

void process()
{
	int i, val01 = 0, val02 = 0, val1 = 1, val2 = 2, val3 = 3;

	srand(time(0));     // seed to generate different random numbers in each run

	for(int i=0;i<100000;++i)
		insert(rand()%INT_MAX, Data(i));
	thread th[n];

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId] = thread(testCS, threadId);
	}

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId].join();
	}

	cout<<"totalInserts = " << totalInserts<<endl;
	cout<<"totalExtracts = " << totalExtracts<<endl;
	cout<<"The average time taken by " << n << " threads to insert is " << totalInsertTime/(totalInserts) << "us" << endl;
	cout<<"The worst-case time taken by a thread to insert is " << WorstInsertTime << "us" << endl;
	// cout<<"The average time taken by " << n << " threads to extractMin is " << totalExtractTime/(totalExtracts) << "us" << endl;
	// cout<<"The worst-case time taken by a thread to extractMin is " << WorstExtractTime << "us" << endl;
}
	

int main(int argc, char *argv[])
{
	// ifstream in("inp-params.txt");

	// if(in.is_open())
	// {
	// 	in>>n>>k;
	// 	process();
	// }

	// in.close();
	n = atoi(argv[1]);
	k = atoi(argv[2]);
	process();
	return 0;
}
#include<iostream>
#include<stdlib.h>
#include<thread>
#include<fstream>
#include<mutex>
#include<unistd.h>
#include<vector>
#include<time.h>
#include<chrono>
#include<cmath>
#include<string>
#include<atomic>
#include<bits/stdc++.h>

using namespace std;
using namespace std::chrono;

int n, k, lamda;
FILE *fd;

double ran_exp(float lamda)
{
	default_random_engine generate;
	exponential_distribution<double> distribution(lamda);
	return distribution(generate);
}

atomic<int> shVar;
volatile long long totalTime = 0;

void testAtomic(int threadId)
{
	tm *ltm;						// to log the time
	time_t now;						
	int lVar;                       // local variable

	for(int i=0;i<k;++i)
	{
		int action = rand()%2;    // randomly choose whether to read or write

		now = time(0);
		ltm = localtime(&now);
		fprintf(fd, "%dth action requested at %d:%d:%d by thread %d\n", i, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, threadId);
		
		auto start = high_resolution_clock::now();
		
		if(action == 0)      
		{
			lVar = atomic_load(&shVar);
			fprintf(fd, "Value read by thread %d: %d\n", threadId, lVar);
		}
		else // Write action
		{
			lVar = k*threadId;  // the value written by each thread is unique
			atomic_store(&shVar, lVar);
			fprintf(fd, "Value written by thread %d: %d\n", threadId, lVar);
		}

		auto stop = high_resolution_clock::now(); 
		auto duration = duration_cast<microseconds>(stop - start);
		totalTime += duration.count();

		now = time(0);
		ltm = localtime(&now);
		fprintf(fd, "%dth action completed at %d:%d:%d by thread %d\n", i, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, threadId);
		
		int t1 = ran_exp(lamda);
		usleep(t1*1000); 			// Simulate performing some other operations.
	}
}

void process()
{

	atomic_init(&shVar, 0); // Atomic shared variable initialized to 0

	fd = fopen (string("Atomic_CPP.log").c_str(),"w+");

	thread th[n];

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId] = thread(testAtomic, threadId);
	}

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId].join();
	}

	cout<<"The average time taken by " << n << " threads to enter CS using CPP Atomic variable is " << totalTime/(n*k) << "us" << endl;
	fclose(fd);
}

int main(int argc, char *argv[])
{
	ifstream in("inp-params.txt");

	if(in.is_open())
	{
		in>>n>>k>>lamda;
		process();
	}

	in.close();
	return 0;
}

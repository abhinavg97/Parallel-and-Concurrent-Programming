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
#include<bits/stdc++.h>

using namespace std;
using namespace std::chrono;

int n, k, lamda1, lamda2;
FILE *fd;

double ran_exp(float lamda)
{
	default_random_engine generate;
	exponential_distribution<double> distribution(lamda);
	return distribution(generate);
}

class Backoff
{
	int minDelay, maxDelay;
	int limit;

public:
	Backoff(int min, int max)
	{
		minDelay = min;
		maxDelay = max;
		limit = minDelay;
	}

	void backoff()
	{
		int delay = rand()%limit;
		limit = min(maxDelay, 2*limit);
		usleep(delay*1000);                // sleep for a random time if collision occurs
	}
};

class BackoffLock
{
private:
	atomic<bool> state;
	int MIN_DELAY;
	int MAX_DELAY;

public:
 	BackoffLock()
 	{
 		MIN_DELAY = 1;
 		MAX_DELAY = 63; // 6 collisions
 		atomic_init(&state, false);
 	}

	void lock()
	{
		Backoff backoff = Backoff(MIN_DELAY, MAX_DELAY);

 		while(true)
 		{
 			while(state.load()) {};
 			if(!state.exchange(true))
 				return;
 			else
 				backoff.backoff();
 		}	
 	}
	
	void unlock()
	{
		state.store(false);
	}

};

BackoffLock* volatile Test;
volatile long long totalTime = 0;
volatile int WorstTime = INT_MIN;

void testCS(int threadId)
{
	tm *ltm;						// to log the time
	time_t now;						

	for(int i=0;i<k;++i)
	{

		now = time(0);
		ltm = localtime(&now);
		fprintf(fd, "%dth CS Request at %d:%d:%d by thread %d\n", i, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, threadId);

		auto start = high_resolution_clock::now();
		Test->lock();
		auto stop = high_resolution_clock::now(); 
		auto duration = duration_cast<milliseconds>(stop - start);
		int time_taken = duration.count();
		totalTime += time_taken;
		if(time_taken > WorstTime)
			WorstTime = time_taken;

		now = time(0);
		ltm = localtime(&now);
		fprintf(fd, "%dth CS Entry at %d:%d:%d by thread %d\n", i, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, threadId);

		int t1 = ran_exp(lamda1);
		usleep(t1*1000); 			// Represents thread's local processing within CS

		Test->unlock();

		now = time(0);
		ltm = localtime(&now);
		fprintf(fd, "%dth CS Exit at %d:%d:%d by thread %d\n", i, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, threadId);
		
		int t2 = ran_exp(lamda2);
		usleep(t2*1000); 			// Represents thread's local processing outside CS
	}
}

void process()
{
	srand(time(0));     // seed to generate different random numbers in each run

	// Declare a lock object which is accessed from all the threads
	Test = new BackoffLock();

	fd = fopen (string("BackoffLock.log").c_str(),"w+");

	thread th[n];

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId] = thread(testCS, threadId);
	}

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId].join();
	}

	cout<<"The average time taken by " << n << " threads to enter CS using Backoff lock mechanism is " << totalTime/(n*k) << "ms" << endl;
	cout<<"The worst-case time taken by a thread to enter CS using Backoff lock mechanism is " << WorstTime << "ms" << endl;

	fclose(fd);
}

int main(int argc, char *argv[])
{
	ifstream in("inp-params.txt");

	if(in.is_open())
	{
		in>>n>>k>>lamda1>>lamda2;
		process();
	}

	in.close();
	return 0;
}
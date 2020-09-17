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

int n, k, lamda;
FILE *fd;

double ran_exp(float lamda)
{
	default_random_engine generate;
	exponential_distribution<double> distribution(lamda);
	return distribution(generate);
}

// #define RegularSRSWRegister int; // we assume cpp variables are SRSW regular 

class StampedValueRegularSRSW
{

public:
	long stamp;
	int value;

	//initial value with zero timestamp
	StampedValueRegularSRSW()
	{
		stamp = 0;
		value = 0;
	}

	//later values with timestamp provided
	StampedValueRegularSRSW(long stamp1, int value1)
	{
		stamp = stamp1;
		value = value1;
	}

	static StampedValueRegularSRSW max(StampedValueRegularSRSW x, StampedValueRegularSRSW y)
	{
		if(x.stamp > y.stamp)
			return x;
		else
			return y;
	}

	// static StampedValueRegularSRSW MIN_VALUE = new StampedValueRegularSRSW();
};


class AtomicSRSWRegister
{
public:
	long lastStamp;
	StampedValueRegularSRSW lastRead;
	StampedValueRegularSRSW r_value; // regular SRSW timestamp-value pair

	AtomicSRSWRegister()
	{
		r_value = StampedValueRegularSRSW();
		lastRead = StampedValueRegularSRSW();
		lastStamp = 0;
		lastRead = r_value;
	}

	int read()
	{
		StampedValueRegularSRSW value = r_value;
		StampedValueRegularSRSW last = lastRead;
		StampedValueRegularSRSW result = result.max(value, last);
		lastRead = result;
		return result.value;
	}

	void write(int v)
	{
		long stamp = lastStamp + 1;
		r_value = StampedValueRegularSRSW(stamp, v);
		lastStamp = stamp;
	}

	~AtomicSRSWRegister()
	{
		;
	}
};

class StampedValueAtomicSRSW
{

public:
	AtomicSRSWRegister *stamp;
	AtomicSRSWRegister *value;

	//initial value with zero timestamp
	StampedValueAtomicSRSW()
	{
		stamp = new AtomicSRSWRegister();
		value = new AtomicSRSWRegister();
		stamp->write(0);
		value->write(0);
	}

	//later values with timestamp provided
	StampedValueAtomicSRSW(long stamp1, int value1)
	{
		stamp = new AtomicSRSWRegister();
		value = new AtomicSRSWRegister();
		stamp->write(stamp1);
		value->write(value1);
	}

	static StampedValueAtomicSRSW max(StampedValueAtomicSRSW x, StampedValueAtomicSRSW y)
	{
		if((x.stamp)->read() > (y.stamp)->read())
			return x;
		else
			return y;
	}

	// static StampedValueAtomicSRSW MIN_VALUE = new StampedValueAtomicSRSW();
};


class AtomicMRSWRegister
{

private:
	StampedValueAtomicSRSW **a_table; // each entry is SRSW atomic

public:
	long lastStamp;
	int readers;

	AtomicMRSWRegister(int readers)
	{
		this->readers = readers;
		lastStamp = 0;
		a_table = new StampedValueAtomicSRSW* [readers];

		for(int i = 0;i<readers;++i)
		{
			a_table[i] = new StampedValueAtomicSRSW [readers];
		}

		StampedValueAtomicSRSW value = StampedValueAtomicSRSW();
		
		for(int i=0;i<readers;++i)
		{
			for(int j=0;j<readers;++j)
			{
				a_table[i][j] = value;
			}
		}
	}

	int read(int threadId)
	{
		int me = threadId;
		StampedValueAtomicSRSW max1 = a_table[me][me];
		for(int i=0;i<this->readers;++i)
		{
			max1 = max1.max(max1, a_table[i][me]);
		}

		for(int i=0;i<this->readers;++i)
		{
			if(i==me) 
				continue;
			a_table[me][i] = max1;
		}
		return (max1.value)->read();
	}

	void write(int v)
	{

		long stamp = lastStamp + 1;
		lastStamp = stamp;
		StampedValueAtomicSRSW value = StampedValueAtomicSRSW(stamp, v);
		for(int i =0;i<this->readers;++i)
		{
			a_table[i][i] = value;
		}
	}
	~AtomicMRSWRegister()
	{
		// delete[] a_table;
	}
};

class StampedValueAtomicMRSW
{

public:
	AtomicMRSWRegister *stamp;
	AtomicMRSWRegister *value;


	StampedValueAtomicMRSW()
	{
		;
	}
	//initial value with zero timestamp
	StampedValueAtomicMRSW(int capacity)
	{
		stamp = new AtomicMRSWRegister(capacity);
		value = new AtomicMRSWRegister(capacity);
		stamp->write(0);
		value->write(0);
	}

	//later values with timestamp provided
	StampedValueAtomicMRSW(long stamp1, int value1, int capacity)
	{
		stamp = new AtomicMRSWRegister(capacity);
		value = new AtomicMRSWRegister(capacity);
		stamp->write(stamp1);
		value->write(value1);
	}

	static StampedValueAtomicMRSW max(StampedValueAtomicMRSW x, StampedValueAtomicMRSW y, int threadId)
	{
		if((x.stamp)->read(threadId) > (y.stamp)->read(threadId))
			return x;
		else
			return y;
	}
	~StampedValueAtomicMRSW()
	{
		// delete[] stamp;
		// delete[] value;
	}
};


class AtomicMRMWRegister
{

private:
	StampedValueAtomicMRSW *a_table; // array of atomic MRSW registers
	int capacity;
public:
	
	AtomicMRMWRegister(int capacity)
	{
		this->capacity = capacity;
		a_table = new StampedValueAtomicMRSW[capacity];
		StampedValueAtomicMRSW value = StampedValueAtomicMRSW(capacity);
		for(int j=0; j < capacity ; j++)
		{
			a_table[j] = value;
		}
	}

	void write(int value, int threadId)
	{
		int me = threadId;
		StampedValueAtomicMRSW max1 = StampedValueAtomicMRSW(this->capacity);
		for(int i=0; i<this->capacity; ++i)
		{
			max1 = max1.max(max1, a_table[i], threadId);
		}
		a_table[me] = StampedValueAtomicMRSW((max1.stamp)->read(threadId) + 1, value, this->capacity);
	}

	int read(int threadId)
	{
		StampedValueAtomicMRSW max1 = StampedValueAtomicMRSW(this->capacity);
		for(int i =0; i<this->capacity; ++i)
		{
			max1 = max1.max(max1, a_table[i], threadId);
		} 
		return (max1.value)->read(threadId);
	}

	~AtomicMRMWRegister()
	{
		// delete[] a_table;
	}
};


AtomicMRMWRegister* volatile shVar;
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
			lVar = shVar->read(threadId);
			fprintf(fd, "Value read by thread %d: %d\n", threadId, lVar);
		}
		else // Write action
		{
			lVar = k*threadId;  // the value written by each thread is unique
			shVar->write(lVar, threadId);
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

	shVar = new AtomicMRMWRegister(n); // Atomic shared variable with capacity n initialized to 0

	fd = fopen (string("Atomic_Custom.log").c_str(),"w+");

	thread th[n];

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId] = thread(testAtomic, threadId);
	}

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId].join();
	}

	cout<<"The average time taken by " << n << " threads to enter CS using custom Atomic variable is " << totalTime/(n*k) << "us" << endl;
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

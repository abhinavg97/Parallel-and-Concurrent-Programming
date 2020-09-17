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

int n, k, mewW, mewS;
FILE *fd;

double ran_exp(int lamda)
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

	~AtomicSRSWRegister(){};
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

	~StampedValueAtomicSRSW(){};
};

class AtomicMRSWRegister
{

private:
	StampedValueAtomicSRSW **a_table; // each entry is SRSW atomic

public:
	long lastStamp;
	int readers;

	AtomicMRSWRegister(){};

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
	~AtomicMRSWRegister(){};
};

class StampedSnapAtomicMRSW
{
public:
	AtomicMRSWRegister stamp;
	AtomicMRSWRegister value;
	AtomicMRSWRegister *snap;

	StampedSnapAtomicMRSW(){};

	StampedSnapAtomicMRSW(int capacity)
	{
		stamp = AtomicMRSWRegister(capacity+1);
		value = AtomicMRSWRegister(capacity+1);
		stamp.write(0);
		value.write(0);
		snap = NULL;
	}

	StampedSnapAtomicMRSW(long label, int value1, int* snap1, int capacity)
	{
		stamp = AtomicMRSWRegister(capacity+1);
		value = AtomicMRSWRegister(capacity+1);
		stamp.write(label);
		value.write(value1);
		snap = new AtomicMRSWRegister[capacity];

		for(int i=0; i<capacity; ++i)
		{
			snap[i] = AtomicMRSWRegister(capacity+1);
			snap[i].write(snap1[i]);
		}
	}

	~StampedSnapAtomicMRSW(){};
};


class WFSnapshot
{
private:
	StampedSnapAtomicMRSW *a_table;   // array of atomic MRSW registers
	int capacity;

public:
	WFSnapshot(int capacity)
	{
		this->capacity = capacity;
		a_table = new StampedSnapAtomicMRSW[capacity];

		for(int i=0; i<capacity; i++)
		{
			a_table[i] = StampedSnapAtomicMRSW(capacity+1);
		}
	}

private:
	StampedSnapAtomicMRSW* collect()
	{
		StampedSnapAtomicMRSW *copy = new StampedSnapAtomicMRSW[this->capacity];
		
		for(int j=0; j<this->capacity; ++j)
			copy[j] = a_table[j];            // deep copy

		return copy;
	}

public:
	void update(int value, int threadId)
	{
		int me = threadId;
		int* snap = scan(threadId);
		StampedSnapAtomicMRSW oldValue = a_table[me];
		StampedSnapAtomicMRSW newValue = StampedSnapAtomicMRSW(oldValue.stamp.read(threadId)+1, value, snap, this->capacity);
		a_table[me] = newValue; 
	}

	int* scan(int threadId)
	{
		StampedSnapAtomicMRSW *oldCopy;
		StampedSnapAtomicMRSW *newCopy;

		bool moved[this->capacity] = {false};
		oldCopy = collect();

		collect:
		while(true)
		{
			newCopy = collect();
			for(int j=0; j<this->capacity; ++j)
			{
				if(oldCopy[j].stamp.read(threadId) != newCopy[j].stamp.read(threadId))
				{
					if(moved[j])
					{
						int* returnVal = new int[this->capacity];
						for(int i=0; i<this->capacity; ++i)
						{
							returnVal[i] = newCopy[j].snap[i].read(threadId);
						}
						return returnVal;
					}
					else
					{
						moved[j] = true;
						delete[] oldCopy;           // delete oldCopy before assigning it to newCopy
						oldCopy = newCopy;
						goto collect;
					}
				}
			}
			int* result = new int[this->capacity];
			for(int j=0; j<this->capacity; ++j)
				result[j] = newCopy[j].value.read(threadId);
			return result;
		}
	}

	~WFSnapshot()
	{
		delete[] a_table;
	}
};


// Declare an array of size n consisting of shared atomic registers
WFSnapshot* volatile snap_array;

volatile long long totalTime = 0;

// A variable to inform the writer threads they have to terminate
atomic<bool> term;

void writer(int threadId)
{
	tm *ltm;						// to log the time
	time_t now;						

	int v;
	int t1;

	while(!term) 							  // Execute unil the term flag is set to true
	{
		v = rand()%INT_MAX;  						  // get random integer value		
		snap_array->update(v, threadId);            // update the location of threadId in the array 

		// record system time and the value v in a local log
		now = time(0);
		ltm = localtime(&now);
		fprintf(fd, "Thr%d's write of %d at %d:%d:%d\n", threadId, v, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
		
		t1 = ran_exp(mewW);
		usleep(t1); 
	}
}

void snapshot(int threadId)
{
	int i = 0;
	int t2;

	tm *ltm;						// to log the time
	time_t now;		

	while(i < k)
	{
		// beginCollect = system time before the ith snapshot collection
		auto beginCollect = high_resolution_clock::now();

		int* collected;
		// collect snapshot;
		collected = snap_array->scan(threadId);
		// endCollect = system time after the ith snapshot collection
		auto endCollect = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(endCollect - beginCollect);
		totalTime += duration.count();

		// store the ith snapshot and timeCollect
		now = time(0);
		ltm = localtime(&now);
		fprintf(fd, "Snapshot Thr's snapshot: ");  
		for(int i=0; i<n; ++i)
			fprintf(fd, "t%d - %d ", i, collected[i]);
		fprintf(fd, "which finished at %d:%d:%d\n", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
		
		t2 = ran_exp(mewS);
		usleep(t2);

		i++;
	}
}

void snap_test()
{
	// Initialization of shared variables
	snap_array = new WFSnapshot(n);

	atomic_init(&term, false);

	fd = fopen (string("Atomic_MRSW_snapshot.log").c_str(),"w+");

	thread th[n+1];

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId] = thread(writer, threadId);
	}

	th[n] = thread(snapshot, n);

	th[n].join();

	term = true;   // Inform all the writer threads that they have to terminate

	for(int threadId=0; threadId<n; ++threadId)
	{
		th[threadId].join();
	}

	cout << "The average time taken by the snapshot algorithm to complete for mewS/mewW = " << float(mewS)/mewW << " for MRSW registers is " << totalTime/(n*k) << " us" << endl;
	fclose(fd);
}

int main(int argc, char* argv[])
{
	ifstream in("inp-params.txt");

	if(in.is_open())
	{
		in>>n>>mewW>>mewS>>k;
		snap_test();
	}

	in.close();
	return 0;
}
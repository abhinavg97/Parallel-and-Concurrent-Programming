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
	~AtomicMRSWRegister()
	{
		delete[] a_table;
	}
};


class StampedValueAtomicMRSW
{

public:
	AtomicMRSWRegister *stamp;
	AtomicMRSWRegister *value;

	StampedValueAtomicMRSW(){};

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
	~StampedValueAtomicMRSW(){};
};


class AtomicMRMWRegister
{

private:
	StampedValueAtomicMRSW *a_table; // array of atomic MRSW registers
	int capacity;
public:
	
	AtomicMRMWRegister(){};

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

	~AtomicMRMWRegister(){};
};


class StampedSnapAtomicMRMW
{
public:
	AtomicMRMWRegister stamp;
	AtomicMRMWRegister value;
	AtomicMRSWRegister **snap;            

	StampedSnapAtomicMRMW(){};

	StampedSnapAtomicMRMW(int capacity, int threadId)
	{
		stamp = AtomicMRMWRegister(capacity+1);
		value = AtomicMRMWRegister(capacity+1);
		stamp.write(0, threadId);
		value.write(0, threadId);
		snap = NULL;
	}

	StampedSnapAtomicMRMW(long label, int value1, int* snap1, int capacity)
	{
		stamp = AtomicMRMWRegister(capacity+1);
		value = AtomicMRMWRegister(capacity+1);

		snap = new AtomicMRSWRegister*[capacity];

		for(int i=0; i<capacity; ++i)
		{
			stamp.write(label, i);
			value.write(value1, i);
			snap[i] = new AtomicMRSWRegister(capacity+1);
			snap[i]->write(snap1[i]);
		}
	}

	~StampedSnapAtomicMRMW(){};
};

class WFSnapshot
{
private:
	StampedSnapAtomicMRMW *a_table;   // array of atomic MRMW registers
	int capacity;

public:
	WFSnapshot(int capacity)
	{
		this->capacity = capacity;
		a_table = new StampedSnapAtomicMRMW[capacity];

		for(int i=0; i<capacity; i++)
		{
			a_table[i] = StampedSnapAtomicMRMW(capacity+1, i);
		}
	}

private:
	StampedSnapAtomicMRMW* collect()
	{
		StampedSnapAtomicMRMW *copy = new StampedSnapAtomicMRMW[this->capacity];
		
		for(int j=0; j<this->capacity; ++j)
			copy[j] = a_table[j];            // deep copy

		return copy;
	}

public:
	void update(int value, int threadId)
	{
		int me = threadId;
		int* snap = scan(threadId);
		StampedSnapAtomicMRMW oldValue = a_table[me];
		StampedSnapAtomicMRMW newValue = StampedSnapAtomicMRMW(oldValue.stamp.read(threadId)+1, value, snap, this->capacity);
		a_table[me] = newValue; 
	}

	int* scan(int threadId)
	{
		StampedSnapAtomicMRMW *oldCopy;
		StampedSnapAtomicMRMW *newCopy;

		vector<int> can_help;
		oldCopy = collect();

		int flag=0;
		while(true)
		{
			newCopy = collect();
			for(int x=0; x<this->capacity; ++x)
			{
				if(oldCopy[x].stamp.read(threadId) != newCopy[x].stamp.read(threadId))
				{
					flag=1;
				}
			}

			if(flag==0)
			{
				int* result = new int[this->capacity];
				for(int j=0; j<this->capacity; ++j)
					result[j] = newCopy[j].value.read(threadId);
				return result;
			}
			
			for(int w=0; w<this->capacity; ++w)
			{
				if(oldCopy[w].stamp.read(threadId) != newCopy[w].stamp.read(threadId))
				{
					if(find(can_help.begin(), can_help.end(), w) != can_help.end())
					{
						int *returnVal = new int[this->capacity];
						for(int i=0; i<this->capacity; ++i)
						{
							returnVal[i] = a_table[w].value.read(threadId);
						}
						return returnVal;
					}
					else
					{
						can_help.push_back(w);
					}
				}
			}
			delete[] oldCopy;
			oldCopy = newCopy;
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

	fd = fopen (string("Atomic_MRMW_snapshot.log").c_str(),"w+");

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

	cout << "The average time taken by the snapshot algorithm to complete for mewS/mewW = " << float(mewS)/mewW << " for MRMW registers is " << totalTime/(n*k) << " us" << endl;
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
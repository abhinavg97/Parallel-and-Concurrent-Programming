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

int n, m;
long long N;
long long counter = 1;
mutex lock1;            // one for the shared counter
vector<vector<long long>> primes;


// standard algorithm for checking if a number is prime
void isPrime(long long num, int id)
{
	bool res=true;
	// Corner cases 
    if (num <= 1)  res = false; 
    else if (num <= 3)  res = true; 
  
    // This is checked so that we can skip  
    // middle five numbers in below loop 
    else if (num%2 == 0 || num%3 == 0) res = false; 
  	else
  	{
	    for (long long i=5; i*i<=num; i=i+6) 
	    {
	        if (num%i == 0 || num%(i+2) == 0) 
	        {
	           res = false;
	           break;
	       	}
	    }   		
  	}

  	if(res==true)
  		primes[id].push_back(num);
}

void work(int id)
{
	long long number_to_check;
	while(1)
	{
		lock1.lock(); 	// locking so that the shared counter can be accesses only by a single thread at once
		if(counter==N)
		{
			lock1.unlock();        // unlocking as soon as we reach the end, all the remaining threads read this value, since a single thread only updates counter at once
			break;
		}
		else
		{
			number_to_check = counter;   // noting the number_to_check and incrementing the counter
			counter++;
			lock1.unlock();              // unlocking the counter after we note its value before checking for primality
			isPrime(number_to_check, id);
		}
	}
}

void process()
{
	vector<long long> primes_found;
	thread worker[m];
	primes.resize(m);

	auto start = high_resolution_clock::now(); 

	for(int i=0;i<m;++i)
		worker[i] = thread(work, i);

	for(int i=0;i<m;++i)
		worker[i].join();

	auto stop = high_resolution_clock::now(); 
	auto duration = duration_cast<milliseconds>(stop - start); 
	cout<<"Time taken "<<duration.count()<<"ms\n";


	for(int i=0;i<m;++i)
		for(auto &j:primes[i])
			primes_found.push_back(j);

	sort(primes_found.begin(), primes_found.end());

	FILE *fd, *fd2;						
	fd = fopen (string("Primes-DAM.txt").c_str(), "w+");
	fd2 = fopen(string("Times.txt").c_str(), "a");

	fprintf(fd2, "Time taken for DAM for n = %d, m = %d is %lldms\n", n, m, (long long)duration.count());
	
	for(auto &i:primes_found)
		fprintf(fd, "%lld ",i);
}

int main()
{
	ifstream in("inp-params.txt");

	if(in.is_open())
	{
		in>>n>>m;
		N = pow(10, n);
		process();
	}

	in.close();
	return 0;
}
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
	for(number_to_check = id ; number_to_check<=(N/2);number_to_check+=m) // checking only odd numbers
	{
		if((2*number_to_check)+1>N)   // eliminating the last few numbers in the above iteration which are greater than N
			break;
		isPrime((2*number_to_check)+1, id);
	}
}

void process()
{
	vector<long long> primes_found;
	thread worker[m];
	primes.resize(m);
	
	auto start = high_resolution_clock::now(); 
	
	primes_found.push_back(2);  // push back the only even prime	

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
	sort(primes_found.begin(), primes_found.end());		

	fd = fopen (string("Primes-SAM2.txt").c_str(), "w+");
	fd2 = fopen(string("Times.txt").c_str(), "a");
	
	fprintf(fd2, "Time taken for SAM2 for n = %d, m = %d is %lldms\n", n, m, (long long)duration.count());
	
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
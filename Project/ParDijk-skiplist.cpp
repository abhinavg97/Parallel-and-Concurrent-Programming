#include <iostream>
#include <stdlib.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <vector>
#include <climits>
#include "skiplist_PQ.h"

using namespace std;
using namespace std::chrono;
// #define TNUM 10
#define MAX_VERTICES 400
#define MAX_EDGES 400*50
#define WT_LIMIT 100000
#define MAXWEIGHT 200

int TNUM;
mutex* DLock;
vector<struct Node *> Offer;
mutex* OfferLock;
vector<bool> done;
vector<int> D;
vector<vector<int> > w;
vector<vector<int> > G_ed;

void publishOfferNoMP(int v, int vd)
{
	int *vdata = new int(vd);
	Offer[v] = Insert(v, vdata);
}

void relax(int v, int vd)
{
	OfferLock[v].lock();
	if(vd < D[v])
	{
		struct Node *vo = Offer[v];
		if(vo == NULL)
		{
			int *vdata = new int(vd);
			Offer[v] = Insert(v, vdata);
		}
		else if(vd < vo->key)
			publishOfferNoMP(v, vd);
	}
	OfferLock[v].unlock();
}

void parallelDijkstra(vector<vector<int>> &E, int tid)
{
	while (!done[tid])
	{
		bool explore;
		struct Node *o = DeleteMin();
		if (o != NULL )
		{
			int u = *(int *)o->value;
			int d = o->key;
			DLock[u].lock();
			if (d < D[u])
			{
				D[u] = d;
				explore = true;
			}
			else
				explore = false;
			
			DLock[u].unlock();
			if(explore)
			{
				for(int u=0;u<E.size();++u)
				{
					for(auto v:E[u])
					{
						int vd = d + w[u][v];
						relax(v, vd);
					}
				}
			}
		}
		else
		{
			done[tid] = true;
			int i = 0;
			while (done[i] && i<TNUM)
				i++;
			if (i == TNUM )
				return;
			done[tid] = false;
		}
	}
}

void Graph(vector<vector<int>> &E)
{
	int V = E.size();

	done.resize(TNUM);
	for(int i=0;i<TNUM;++i)
		done[i] = false;

	D.resize(V);
	for(int i=0;i<V;++i)
		D[i] = INT_MAX;

	Offer.resize(V, NULL);
	DLock = new mutex[V];
	OfferLock = new mutex[V];
}
  
// Define the number of runs for the test data 
// generated 
#include <bits/stdc++.h>
#define RUN 1

int gen_graph() 
{
    set<pair<int, int>> container; 
    set<pair<int, int>>::iterator it; 
  
    // Uncomment the below line to store 
    // the test data in a file 
    // freopen("Test_Cases_Undirected_Weighted_Graph.in", 
    //          "w", stdout); 
  
    //For random values every time 
    srand(time(NULL)); 
  
    int NUM;    // Number of Vertices 
    int NUMEDGE; // Number of Edges 
  
    for (int i=1; i<=RUN; i++) 
    { 
        NUM = MAX_VERTICES; 
  
        // Define the maximum number of edges of the graph 
        // Since the most dense graph can have N*(N-1)/2 edges 
        // where N =  nnumber of vertices in the graph 
        NUMEDGE = MAX_EDGES; 

        G_ed.resize(NUM);
  
        // First print the number of vertices and edges 
        // printf("%d %d\n", NUM, NUMEDGE); 
  
        // Then print the edges of the form (a b) 
        // where 'a' is connected to 'b' 
        for (int j=1; j<=NUMEDGE; j++) 
        { 
            int a = rand() % NUM; 
            int b = rand() % NUM; 
            pair<int, int> p = make_pair(a, b); 
            pair<int, int> reverse_p = make_pair(b, a); 
  
            // Search for a random "new" edge everytime 
            // Note - In a tree the edge (a, b) is same 
            // as the edge (b, a) 
            while (container.find(p) != container.end() || 
                    container.find(reverse_p) != container.end()) 
            { 
                a = rand() % NUM; 
                b = rand() % NUM; 
                p = make_pair(a, b); 
                reverse_p = make_pair(b, a); 
            } 
            container.insert(p);
        }

        w.resize(NUM);
        for(int i = 0; i < NUM; i++) {
        	w[i].resize(NUM, WT_LIMIT);
        }
  
        for (it=container.begin(); it!=container.end(); ++it) 
        { 
            int wt = 1 + rand() % MAXWEIGHT; 
            w[it->first][it->second] = wt;
            w[it->second][it->first] = wt;
            G_ed[it->first].push_back(it->second);
            G_ed[it->second].push_back(it->first);
        }
  
        container.clear(); 
    } 
  
    // Uncomment the below line to store 
    // the test data in a file 
    // fclose(stdout); 
    return(0); 
}

void testCS(int thID) {
	parallelDijkstra(G_ed, thID);
}

int main(int argc, char* argv[]) {

	TNUM = atoi(argv[1]);
	gen_graph();
	Graph(G_ed);

	int val01 = 1, val02 = 2;

	// skip-list PQ init
	head = CreateNode(maxLevel, INT_MIN, &val01);
	tail = CreateNode(maxLevel, INT_MAX, &val02);
	for(int i = 0; i < maxLevel; i++) {
		head->next[i] = tail;
	}
	tail->prev = head;


	// SSSP
	thread th[TNUM];

	auto start = high_resolution_clock::now();

	for(int threadId=0; threadId<TNUM; ++threadId)
	{
		th[threadId] = thread(testCS, threadId);
	}

	for(int threadId=0; threadId<TNUM; ++threadId)
	{
		th[threadId].join();
	}

	auto stop = high_resolution_clock::now(); 
	auto duration = duration_cast<microseconds>(stop - start);
	int time_taken = duration.count();

	cout << "time-taken --> " << time_taken << " us" << endl;
}
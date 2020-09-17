/*
common headers required by "heap based priority-queue implementation"
*/

/*
Bhanu Prakash Thandu
cs15btech11037
*/

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
#include <mutex>          // std::mutex

using namespace std;
using namespace std::chrono;

#define Length 500000	// size of array used for heap

class Data {
public:
	int val;
public:
	Data() {}
	Data(int val_): val(val_) {}
};

class Element {
public:
	uint key;
	Data data;
	int pos;
	bool up;
public:
	Element() {}
	Element(int key_, Data data_, int pos_): key(key_), data(data_), pos(pos_), up(false) {}
};

inline bool operator ==(const Data &a, const Data &b) { return a.val == b.val; }
inline bool operator !=(const Data &a, const Data &b) { return !(a == b); }
inline bool operator ==(const Element &a, const Element &b) {
	return a.key == b.key && a.pos == b.pos && a.data == b.data;
}
inline bool operator !=(const Element &a, const Element &b) { return !(a == b); }


int Last = 0;	// index of last node in heap (priority queue)

Element *A[Length + 1] = {NULL};
mutex L[Length + 1];

void swap(int i, int j) {
	Element *temp = A[i];
	A[i] = A[j];
	A[j] = temp;

	A[i]->pos = i;
	A[j]->pos = j;
}

int leftChild(int i) {
	return 2*i;
}

int rightChild(int i) {
	return 2*i + 1;
}

int parent(int i) {
	return i/2;
}

void lock(mutex &mtx) {
	mtx.lock();
}

void unlock(mutex &mtx) {
	mtx.unlock();
}

bool tryLock(mutex &mtx) {
	return mtx.try_lock();
}

// bubble down
void bubbleDown(Element *e) {
	int min = e->pos;
	int i, l, r;
	do {
		i = min;
		l = leftChild(i);
		r = rightChild(i);

		if (l <= Last) {
			lock(L[l]);
			lock(L[r]);

			if (A[l] != NULL) {
				if (A[l]->key < A[i]->key) {
					min = l;
				}
				if (A[r] != NULL && A[r]->key < A[min]->key) {
					min = r;
				}
				if (i != min) {
					if (min == l) {	// is this min == l or i == l?
						unlock(L[r]);
					}
					else {
						unlock(L[l]);
					}
					swap(i, min);
					unlock(L[i]);
				}
			}
		}
	}
	while(i != min);
	unlock(L[i]);
}

// lock element (node)
bool lockElement(Element *e) {
	while(true) {
		int i = e->pos;
		if (i == -1) {
			return false;
		}
		if (tryLock(L[i])) {
			if (i == e->pos) {
				return true;
			}
			unlock(L[i]);
		}
	}
}

// bubble up
void bubbleUp(Element *e) {
	int i = e->pos;
	bool iLocked = true;
	bool parLocked = false;

	while(1 < i) {
		int par = parent(i);
		parLocked = tryLock(L[par]);

		if (parLocked) {
			if (!A[par]->up) {
				if (A[i]->key < A[par]->key) {
					swap(i, par);
				}
				else {
					A[i]->up = false;
					unlock(L[i]);
					unlock(L[par]);
					return;
				}
			}
			else {
				unlock(L[par]);
				parLocked = false;
			}
		}
		unlock(L[i]);
		iLocked = false;

		if (parLocked) {
			i = par;
			iLocked = true;
		}
		else {
			iLocked = lockElement(e);
			i = e->pos;
		}
	}
	e->up = false;

	if (iLocked) {
		unlock(L[e->pos]);
	}
}

// peek the element (node) with min priority
Element *peek() {
	Element *ret;
	lock(L[1]);
	ret = A[1];
	unlock(L[1]);
	return ret;
}

// extract element (node) with min priority
Element *extractMin() {
	lock(L[1]);
	Element *min = A[1];
	int ls = Last;

	if (ls == 0) {
		unlock(L[1]);
		return NULL;
	}
	A[1]->pos = -1;
	if (ls == 1) {
		Last = 0;
		A[1] = NULL;
		unlock(L[1]);
	}
	else {
		lock(L[ls]);
		A[1] = A[ls];
		A[1]->pos = 1;
		A[ls] = NULL;
		Last = ls - 1;
		unlock(L[ls]);

		if (ls == 2) {
			unlock(L[1]);
		}
		else {
			bubbleDown(A[1]);
		}
	}
	return min;
}

// insert a new element (node)
Element *insert(int key, Data data) {
	lock(L[1]);
	if (Last == Length) {
		unlock(L[1]);
		return NULL;
	}
	Element *e = new Element(key, data, Last + 1);

	if (Last == 0) {
		e->up = false;
		A[1] = e;
		Last = 1;
		unlock(L[1]);
	}
	else {
		lock(L[Last + 1]);
		e->up = true;
		A[Last + 1] = e;
		Last = Last + 1;
		unlock(L[1]);
		bubbleUp(e);
	}
	return e;
}

// change key (priority) of a given element (node)
bool changeKey(Element *e, int k) {
	while(lockElement(e)) {
		if (e->up) {
			unlock(L[e->pos]);
		}
		else {
			if (k < e->key) {
				e->up = true;
				e->key = k;
				bubbleUp(e);
			}
			else if (k > e->key) {
				e->key = k;
				bubbleDown(e);
			}
			else {
				unlock(L[e->pos]);
			}
			return true;
		}
	}
}

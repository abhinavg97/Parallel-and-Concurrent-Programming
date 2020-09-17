#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
// #include<random.h>


		
struct Node
{
	int key, level, validLevel;
	void *value;
	struct Node** next;
	struct Node* prev;
};

// Allocates a new node from the memory pool and returns a corresponsing pointer with a reference 
// count of one.
struct Node* MALLOC_NODE();
// Atomically de-references the given link and increases the reference counter for the corresponding 
// node. In case the deletion mark of the link is set returns NULL.
struct Node* READ_NODE(struct Node* address);
// Increases the reference counter for the corresponding given node.
struct Node* COPY_NODE(struct Node* node);
// Decrements the reference counter on the corresponding given node. If the reference counter reaches 
// zero, the function will recursively call RELEASSE_NDOE on the nodes that this node has owned 
// pointers to (i.e. the prev pointer), and then it reclaims the node.
void RELEASE_NODE(struct Node* node);
struct Node* CreateNode(int level, int key, void* value);
struct Node* ReadNext(struct Node** node1, int level);
struct Node* ScanKey(struct Node** node1, int level, int key);
struct Node* Insert(int key, void* value);
void RemoveNode(struct Node *node, struct Node **prev, int level);
struct Node* DeleteMin();
struct Node* HelpDelete(struct Node* node, int level);
bool IS_MARKED(struct Node* ptr);
struct Node* GET_MARKED(struct Node* ptr);
struct Node* GET_UNMARKED(struct Node* ptr);


void PrintNodeInfo(struct Node* node)
{
	int i;
	
	if(!node)
	{
		// printf("node is NULL\n");
		return;
	}
	
	// printf("Pieces of a node\n");
	// printf("Key %d, Level %d, Valid Level %d\n", node->key, node->level, node->validLevel);
	// printf("Value %d, boolean %d\n", *(int*)GET_UNMARKED(node->value), IS_MARKED(node->value));
	// for(i = 0; i < node->level; i++)
		// // printf("Link pointer %d, boolean %d\n", (int)node->next[i].p, node->next->d);
	// // printf("Node prev pointer %d\n\n", (int)node->prev);
}


void PrintList(struct Node* node)
{
	struct Node *node2 = node;
	
	// printf("\nPrinting List of nodes\n");
	do
	{
		PrintNodeInfo(node2);
		node2 = GET_UNMARKED(node2->next[0]);
	} while(node2 != NULL);
}


int maxLevel = 3;	// I have arbitrarily choosen 3. Idk how it should be set. maxLevel is used in the psuedo code
// Global variables
struct Node *head, *tail;

struct Node* CreateNode(int level, int key, void* value)
{
	// printf("create node\n");
	
	struct Node* node = MALLOC_NODE();  //need this for safe pointer handling
	node->prev = NULL;
	node->validLevel = 0;
	node->level = level;
	node->key = key;
	node->next = (struct Node**)malloc(sizeof(struct Node*) * level);
	node->value = value;
	return node;
}

bool IS_MARKED(struct Node* ptr)
{
	return (bool)(((long long)((void *)ptr))&1);
}

struct Node* ReadNext(struct Node** node1, int level)
{
	// printf("read next\n");
	// Local variables (for all functions/procedures)
	struct Node *node2;

	if(IS_MARKED((*node1)->value) == true)
		*node1 = HelpDelete(*node1, level);
	
	node2 = READ_NODE((*node1)->next[level]);
	
	while(node2 == NULL)
	{
		*node1 = HelpDelete(*node1, level);
		node2 = READ_NODE((*node1)->next[level]);
	}
	return node2;
}

struct Node* ScanKey(struct Node** node1, int level, int key)
{
	// printf("scankey\n");
	struct Node *node2;

	node2 = ReadNext(node1, level);
	while(node2->key < key)
	{
		RELEASE_NODE(*node1);
		*node1 = node2;
		node2 = ReadNext(node1, level);
	}
	return node2;
}

struct Node* Insert(int key, void* value)
{
	// printf("insert\n");
	// Local variables (for all functions/procedures)
	struct Node *newNode, *savedNodes[maxLevel];
	struct Node *node1, *node2, *prev, *last;
	int i;

	int level = rand()%maxLevel;
	
	newNode = CreateNode(level, key, value);
	COPY_NODE(newNode);
	node1 = COPY_NODE(head);
	
	for(i = maxLevel-1; i >= 1; i--)
	{
		node2 = ScanKey(&node1, i, key);
		RELEASE_NODE(node2);
		if(i < level)
			savedNodes[i] = COPY_NODE(node1);
	}
	
	while(true)
	{
		node2 = ScanKey(&node1, 0, key);
		void * value2 = node2->value;
		
		if(IS_MARKED(node2->value) == false && node2->key == key)
		{
			   if(__sync_bool_compare_and_swap(&(node2->value), value2, value))
			   {
				   RELEASE_NODE(node1);
				   RELEASE_NODE(node2);
				   for(i = 1; i <= level-1; i++)
					   RELEASE_NODE(savedNodes[i]);
				   RELEASE_NODE(newNode);
				   RELEASE_NODE(newNode);
				   return newNode;
			   }
			   else
			   {
				   RELEASE_NODE(node2);
				   continue;
			   }
		}
		
		newNode->next[0] = node2;
		
		RELEASE_NODE(node2);
		
		if(__sync_bool_compare_and_swap(&node1->next[0], node2, newNode))
		{
			RELEASE_NODE(node1);
			break;
		}
		// Back-Off
	}
	
	for(i = 1; i <= level-1; i++)
	{
		newNode->validLevel = i;
		node1 = savedNodes[i];
		
		while(true)
		{
			node2 = ScanKey(&node1, i, key);
			newNode->next[i] = node2;
			
			RELEASE_NODE(node2);
			
			if(IS_MARKED(newNode->value) == true || __sync_bool_compare_and_swap(&node1->next[i], node2, newNode))
			{
				RELEASE_NODE(node1);
				break;
			}
			// Back-Off
		}
	}
	
	newNode->validLevel = level;
	
	if(IS_MARKED(newNode->value) == true)
		newNode = HelpDelete(newNode, 0);
	
	RELEASE_NODE(newNode);
	
	return newNode;
}

void RemoveNode(struct Node *node, struct Node **prev, int level)
{
	// printf("remove node\n");
	// Local variables (for all functions/procedures)
	struct Node *last;
	
	while(true)
	{
		if(node->next[level] == 1)
			break;
		
		last = ScanKey(prev, level, node->key);
		RELEASE_NODE(last);
		
		if(last != node || node->next[level] == 1)
			break;
		
		if(__sync_bool_compare_and_swap(&(*prev)->next[level], node, GET_UNMARKED(node->next[level])))
		{
			node->next[level] = 1;
			break;
		}
		
		if(node->next[level] == 1)
			break;
		
		//Back-Off
	}
}

struct Node* DeleteMin()
{
	// printf("delete min\n");
	// Local variables (for all functions/procedures)
	struct Node *newNode, *savedNodes[maxLevel];
	struct Node *node1, *node2, *prev, *last;
	void *value;
	int i;
	
	prev = COPY_NODE(head);
	
	while(true)
	{
		// Checking if the head is the same as the tail
		node1 = ReadNext(&prev,0);
		if(node1 == tail)
		{
			RELEASE_NODE(prev);
			RELEASE_NODE(node1);
			return NULL;
		}
retry:
		value = node1->value;
		
		if(IS_MARKED(value) == false)
		{
			if(__sync_bool_compare_and_swap(&node1->value, value, GET_MARKED(value)))
			{
				node1->prev = prev;
				break;
			}
			else
				goto retry;
		}
		else if(IS_MARKED(value) == true)
			node1 = HelpDelete(node1, 0);
		RELEASE_NODE(prev);
		prev = node1;
	}
	
	for(i = 0; i <= node1->level-1; i++)
	{
		 do
		 {
			 node2 = node1->next[i];
			 // Until d = true or CAS
		 } while((IS_MARKED(node2) != true) && !(__sync_bool_compare_and_swap(&node1->next[i], node2, GET_MARKED(node2))));
	}
	
	prev = COPY_NODE(head);
	for(i = node1->level-1; i >= 0; i--)
		RemoveNode(node1, &prev, i);
		
	RELEASE_NODE(prev);
	RELEASE_NODE(node1);
	RELEASE_NODE(node1); /*Delete the node*/

	return node1;
}

struct Node* GET_MARKED(struct Node* ptr)
{
	return (struct Node*)((void*)((long long)((void*)ptr) | 1));
}

struct Node* GET_UNMARKED(struct Node* ptr) {
	return (struct Node*)((void*)((long long)((void*)ptr) & (~1)));
}

struct Node* HelpDelete(struct Node* node, int level)
{
	// printf("help delete\n");
	struct Node *node2, *prev;
	int i;
		
	for(i = level; i <= node->level-1; i++)
	{
		do
		{
			node2 = node->next[i];
		} while(IS_MARKED(node2) != true && !(__sync_bool_compare_and_swap(&node->next[i], node2, GET_MARKED(node2))));
	}
	
	prev = node->prev;
	
	if(!prev || level >= prev->validLevel)
	{
		prev = COPY_NODE(head);
		
		for(i = maxLevel-1; i >= level; i--)
		{
			node2 = ScanKey(&prev, i, node->key);
			RELEASE_NODE(node2);
		}
	}
	else
		COPY_NODE(prev);
	
	RemoveNode(node, &prev, level);
	RELEASE_NODE(node);
	
	return prev;
}

//INCORRECT IMPLEMENTATIONS OF MEMORY MANAGEMENT
struct Node* MALLOC_NODE()
{
	return (struct Node*)malloc(sizeof(struct Node));
}

struct Node* READ_NODE(struct Node* address)
{
	/* De-reference the pointer and increase the reference counter for the corresponding node. In
	   case the pointer is marked, NULL is returned */
	if(IS_MARKED(address))
		return NULL;
	
	return GET_UNMARKED(address);
}

struct Node* COPY_NODE(struct Node* node)
{
	/* Increase the reference counter for the corresponding given node */
	return node;
}

void RELEASE_NODE(struct Node* node)
{
	/* Decrement the reference counter on the corresponding
	given node. If the reference count reaches zero, then call
	RELEASE_NODE on the nodes that this node has owned
	pointers to, then reclaim the node */
	return;
}
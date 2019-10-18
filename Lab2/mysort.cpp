#include <iostream>
#include <iomanip>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <atomic>

#include "mcslock.hpp"
using namespace std;
struct timespec start_time, end_time;

/*----------------------------------
 *Structure to distibute values in
 *bucket as well as sorting each
 *----------------------------------*/
struct Bucket_Nodes 
{ 
	int data;  
	struct Bucket_Nodes *next; 
};
/*-----------------------------------
 *Structure to store values of the 
 *bucket i.e. no of buckets.
 *-----------------------------------*/
struct bucket_pos
{
	int position;
};
/* Global varaibles for bucket sort*/
int NUM_THREADS;
int INTERVAL;
int bucket_value;
int array_element;
string lock_select;
string barrier_type;
/*Global Variables for merge sort*/
int merge_implement;
int *array_sort;
void BucketSort(int arr[]);
struct Bucket_Nodes *Bucket_InsertSort(struct Bucket_Nodes *list);
int Bucket_Number(int value);
struct Bucket_Nodes **buckets;
pthread_mutex_t locker;
atomic<bool> flag = {false};
constexpr size_t CACHELINE_SIZE = 64;
/*------------------------------------------------------
 *@Class : TasSpinLock
 *@Function : This implements the test and set lock which
			  is an atomic lock which implements FIFO and
			  has cache miss rate. The class has two void
			  functions Enter and Leave which is lock and
			  unlock respectively.
 *------------------------------------------------------*/
class TasSpinLock
{
public:
    inline void Enter()
    {
        // atomic_bool::exchange() returns previous value of Locked
        while (Locked.exchange(true, std::memory_order_acquire) == true);
    }
 
    inline void Leave()
    {
        Locked.store(false, std::memory_order_release);
    }
 
private:
    alignas(CACHELINE_SIZE) std::atomic_bool Locked = {false};
};
static_assert(sizeof(TasSpinLock) == CACHELINE_SIZE, "");
/*-------------------------------------------------------------------
 *@Class : TTasSpinLock
 *@Function : This implements the testand test and set lock which is 
			  an atomic lock which implements LIFO and has low cache 
			  miss rate. The class has two void functions Enter and 
			  Leave which is lock and unlock respectively.
 *-------------------------------------------------------------------*/
class TTasSpinLock
{
public:
	inline void Enter()
	{
		do
			WaitUntilLockIsFree();
		while (Locked.exchange(true, std::memory_order_acquire) == true);
	}
 
	inline void WaitUntilLockIsFree() const
	{
		while (Locked.load(std::memory_order_relaxed) == true);
	}
	inline void Leave()
    {
        Locked.store(false, std::memory_order_release);
    }
private:
    std::atomic_bool Locked = {false};
};
static_assert(sizeof(TasSpinLock) == CACHELINE_SIZE, "");
/*-------------------------------------------------------------------
 *@Class : TicketSpinLock
 *@Function : This implements the ticket lock which is an atomic lock 
			  which implements FIFO and has high cache miss rate. The 
			  class has two void functions Enter and Leave which is 
			  lock and unlock respectively.
 *-------------------------------------------------------------------*/
class TicketSpinLock
{
public:
    inline void Enter()
    {
        const auto myTicketNo = NextTicketNo.fetch_add(1, std::memory_order_relaxed);
 
        while (ServingTicketNo.load(std::memory_order_acquire) != myTicketNo)
            asm("pause");
    }
 
    inline void Leave()
    {
        // We can get around a more expensive read-modify-write operation
        // (std::atomic_size_t::fetch_add()), because no one can modify
        // ServingTicketNo while we're in the critical section.
        const auto newNo = ServingTicketNo.load(std::memory_order_relaxed)+1;
        ServingTicketNo.store(newNo, std::memory_order_release);
    }
 
private:
    std::atomic_size_t ServingTicketNo = {0};
    std::atomic_size_t NextTicketNo = {0};
};

/*--------------------------------------------
 *@Class : MutexLock
 *@Function :  This class is a classic mutex
			   lock and uses the inbuilt mutex
			   functions lock and unlock.
 *--------------------------------------------*/
class MutexLock
{
public:
	
		inline void Enter()
		{
			pthread_mutex_lock(&locker);
		}
		inline void Leave()
		{
			pthread_mutex_lock(&locker);
		}
	
};

/*----------------------------------------------------
 *@Class : Barrier 
 *@Function : This implements sense reversal barrier
			  which waits for other threads and to be
			  completed and reverts back and forth
			  between true or false for the sense.
 *----------------------------------------------------*/
class Barrier
{
	int cnt = 0;
	atomic<int> sense={0};
	int N = NUM_THREADS;
	MutexLock locker;	
	public:
	
	void wait()
	{
		
		thread_local bool my_sense =0;
		if(my_sense == 0)
		{
				my_sense =1;
		} //flip sense
		else 
			my_sense = 0;

		locker.Enter();
		cnt++;
		if(cnt==N)
		{
			cnt=0;
			locker.Leave();
			sense.store(my_sense);
		}
		else
		{
			locker.Leave();
			while(sense.load()!=my_sense)
			{
				exit(0);
			}
			}
	}
};

/*-------------------------------------------------
 *@Class : mcs_lock
 *@Function : This implements the mcs lock which 
			  uses atomic nodes as well as boolean
			  functions where the lock is acquired
			  when the node value is true and then
			  unlocked when the value changes from 
			  true to false.
 *-------------------------------------------------*/
namespace sync{
	
	inline void mcs_lock::Enter(Node* myNode) 
	{
		
		
		myNode->next.store(nullptr , memory_order_relaxed);
		myNode->wait.store(true);
		Node* oldTail = tail.exchange(myNode);
		
		if(oldTail != NULL )
		{
			myNode->wait.store(true,memory_order_relaxed);
			oldTail->next.store(myNode);
			while(myNode->wait.load())
			{
				asm("pause");
			}
		}
	}
	inline void mcs_lock::Leave(Node* myNode) 
	{
		
		if (myNode->next.load() == nullptr)
		{
			Node *new_tail = myNode;
			if (tail.compare_exchange_strong(new_tail, nullptr))
            return;

			while (myNode->next.load() == nullptr)
                 asm("pause");
    }

		myNode->next.load()->wait = false;
	}
}
/*-------------------------
 *-------------------------*/

/*-----------------------
 * 
 *Bucket sort algorithm 
 *
 *-----------------------*/

/*-------------------------------------------------------
 *@Function : void (int [])
 *@Input : This function takes an array as an input
 *		   and allocates memory to buckets which is
 *		   the structure of the bucket mulitplied to
 *		   the number of threads. Then the elements 
 *		   are inserted into the bucket in a sequential
 *		   manner i.e. as a list.
 *@Return Value : This function returns a null value
 *-------------------------------------------------------*/

void BucketSort(int arr[])
{	
	int i,j;
	  

	/* allocate memory for array_bucket of pointers to the buckets */
	buckets = (struct Bucket_Nodes **)malloc(sizeof(struct Bucket_Nodes*) * NUM_THREADS); 

	/* initialize pointers to the buckets */
	for(i = 0; i < NUM_THREADS;++i) {  
		buckets[i] = NULL;
	}

	/* put items into the buckets */
	for(i = 0; i < array_element; ++i) {	
		struct Bucket_Nodes *current;
		int pos = Bucket_Number(arr[i]);
		current = (struct Bucket_Nodes *) malloc(sizeof(struct Bucket_Nodes));
		current->data = arr[i]; 
		current->next = buckets[pos];  
		buckets[pos] = current;
	}

	return;
}


/*--------------------------------------------------------------------------
 *@Function : struct Bucket_Nodes *Bucket_InsertSort(struct Bucket_Nodes *)
 *@Input : This function takes the Bucket_Nodes struct pointer and then adds
		   the element into the given list with the first element having a
		   default NULL. Then the incoming element is checked if the value
		   is greater than the first element and each element is added in
		   this way compared to the previous element and will be added in 
		   the back if it is greater.
 *@Return Value : Returns the structure with the updated values.
 *--------------------------------------------------------------------------*/
 
struct Bucket_Nodes *Bucket_InsertSort(struct Bucket_Nodes *list)
{	
	struct Bucket_Nodes *k,*nodeList;
	/* need at least two items to sort */
	if(list == 0 || list->next == 0) 
	{ 
		return list; 
	}
	
	nodeList = list; 
	k = list->next; 
	nodeList->next = 0; /* 1st node is new list */
	while(k != 0) 
	{	
		struct Bucket_Nodes *ptr;
		/* check if insert before first */
		if(nodeList->data > k->data)  { 
			struct Bucket_Nodes *tmp;
			tmp = k;  
			k = k->next; 
			tmp->next = nodeList;
			nodeList = tmp; 
			continue;
		}

		for(ptr = nodeList; ptr->next != 0; ptr = ptr->next) 
		{
			if(ptr->next->data > k->data) break;
		}

		if(ptr->next!=0)
		{  
			struct Bucket_Nodes *tmp;
			tmp = k;  
			k = k->next; 
			tmp->next = ptr->next;
			ptr->next = tmp; 
			continue;
		}
		else
		{
			ptr->next = k;  
			k = k->next;  
			ptr->next->next = 0; 
			continue;
		}
	}
	return nodeList;
}


/*--------------------------------------
 *@Function : int Bucket_Number(int)
 *@Input : This takes the number of 
		   elements in array.
 *@Return :  Returns the bucket number.
 *--------------------------------------*/
 
int Bucket_Number(int value)
{
	return value/INTERVAL;
}


/*-------------------------------------------------------
 *@Function : void *bucket_sort(void *)
 *@Input : This function takes input as void pointer and
		   locks the thread for avoiding data race. This
		   updates the position of the bucket i.e. bucket 
		   number to determine the number of buckets and 
		   is given as a argument to the threads.
 *@Return Value : Returns 0 on success.
 *-------------------------------------------------------*/
 
void *buck_sort(void *args)
{
	int position;
	if(barrier_type=="--bar=pthread")
	{
		if(lock_select=="--lock=tas")
		{
			TasSpinLock lock_1;
			clock_gettime(CLOCK_REALTIME,&start_time);
			lock_1.Enter();
			struct bucket_pos *bucket_pos=args;
			position=bucket_pos->position;
			buckets[position] = Bucket_InsertSort(buckets[position]);
			lock_1.Leave();
			clock_gettime(CLOCK_REALTIME,&end_time);
		}
		if(lock_select=="--lock=ttas")
		{
			TTasSpinLock lock_1;
			clock_gettime(CLOCK_REALTIME,&start_time);
			lock_1.Enter();
			struct bucket_pos *bucket_pos=args;
			position=bucket_pos->position;
			buckets[position] = Bucket_InsertSort(buckets[position]);
			lock_1.Leave();
			clock_gettime(CLOCK_REALTIME,&end_time);
		}
		else if(lock_select=="--lock=ticket")
		{
			TicketSpinLock lock_1;
			clock_gettime(CLOCK_REALTIME,&start_time);
			lock_1.Enter();
			struct bucket_pos *bucket_pos=args;
			position=bucket_pos->position;
			buckets[position] = Bucket_InsertSort(buckets[position]);
			lock_1.Leave();
			clock_gettime(CLOCK_REALTIME,&end_time);
		}
		else if(lock_select=="--lock=mcs")
		{
			sync::mcs_lock lock_1;
			sync::Node *myNode;
			clock_gettime(CLOCK_REALTIME,&start_time);
			lock_1.Enter(myNode);
			struct bucket_pos *bucket_pos=args;
			position=bucket_pos->position;
			buckets[position] = Bucket_InsertSort(buckets[position]);
			lock_1.Leave(myNode);
			clock_gettime(CLOCK_REALTIME,&end_time);
		}
		else if(lock_select=="--lock=pthread")
		{
			MutexLock lock_1;
			clock_gettime(CLOCK_REALTIME,&start_time);
			lock_1.Enter();
			struct bucket_pos *bucket_pos=args;
			position=bucket_pos->position;
			buckets[position] = Bucket_InsertSort(buckets[position]);
			lock_1.Leave();
			clock_gettime(CLOCK_REALTIME,&end_time);
		}
	}
	else if(barrier_type=="--bar=sense")
	{
			Barrier bar;
			clock_gettime(CLOCK_REALTIME,&start_time);
			struct bucket_pos *bucket_pos=args;
			position=bucket_pos->position;
			buckets[position] = Bucket_InsertSort(buckets[position]);
			clock_gettime(CLOCK_REALTIME,&end_time);
			bar.wait();
	}
	return;
}	
void print_message()
{
	cout << "First Argument: --name\n" 
	     << "This prints the name of the person whose code this is\n\n"
	     << "Second Argument: inputfile\n"
	     << "Prints the sorted elements on the terminal"
	     <<" " << "if no other arguments are present\n\n"
	     << "First Argument : inputfile \n"
	     << "Second Argument : -o \n"
	     << "Third Argument : outputfile \n"
	     << "This will put the sorted elements in the outputfile\n\n"
	     << "Fourth Argument : -t\n"
	     << "Fifth Argument : int value\n"
	     << "This will be the number of threads to be used\n\n"
	     << "Sixth Argument: --alg=(either bk for bucket algorithm or"
	     << "ms for mergesort\n"
	     << "This will decide what algorithm to use\n\n"	
	     << endl;

}
int main(int argc, char *argv[])
{
	struct thread_merge *thread_merge;
	ifstream given_file;
	string arg_1=argv[1];
	string arg_6=argv[6];
	string arg_7=argv[7];
	lock_select=argv[9];
	int i,j;
	if(argc ==2 && arg_1 == "--name")
	{
		cout << "Vignesh Iyer\n" <<endl;
		return 1;
	}
	else if(argc < 8 && argc < 2)
	{
		if(arg_1 == "--name")
		{
			cout << "Vignesh Iyer\n" <<endl;
			print_message();
			return 1;
		}
		else
		{	
			print_message();
			return 1;
		}
	}
	else if(argc==9)
	{
		if(arg_1 == "--name")
		{
			cout << "Vignesh Iyer\n" <<endl;
			return 1;
		}
		
		/*----------------------------------
		 *This function is of type ifstream
		 *where the input is argv[1]
		 *----------------------------------*/
			 
		given_file.open(argv[2]);
			
		if (!given_file)
		{
			cerr << "Unable to find input file Bye\n";
			exit(-1);   
		}
		float number_read;
		while(given_file >> number_read)
		{
			
			array_element++;
		}
		given_file.close();
		given_file.open(argv[2]);
		int array_bucket[array_element];
		array_element =0;
		while(given_file >> number_read)
		{
			
			array_bucket[array_element]=number_read;
			array_element++;
		}
		given_file.close();
				/*--------------------------------------
		 *This argument i.e. argv[5] determines
		 *the number of threads.
		 *--------------------------------------*/
			 
		sscanf(argv[5],"%d",&NUM_THREADS);
		pthread_t threads[NUM_THREADS];
		
		/*-----------------------------------------
		 *Based on the value of the arg_6 which is
		 *arg{6] the algorithm type is decided.
		 *-----------------------------------------*/
		if(arg_7 =="--alg=bk")
		{
			if(array_element%NUM_THREADS==0)
			{
				INTERVAL=(int)array_element/NUM_THREADS;
			}
			else
			{
				INTERVAL=(int)array_element/(NUM_THREADS-1);		
			}			
			struct bucket_pos *bucket_pos;
			struct bucket_pos *bucket_pos_1[NUM_THREADS];
			
			/*Allocating memory to each structure which equal to the number of threads*/
			for(int i=0;i<NUM_THREADS;i++)
				bucket_pos_1[i]=malloc(sizeof(*bucket_pos_1)*2);
			
			/*Assigning buckets to each elements*/
			BucketSort(array_bucket);
			
			/*Creation of threads*/
			for ( i = 0; i < NUM_THREADS; i++) {
				bucket_pos=bucket_pos_1[i];
				bucket_pos->position=i;
				pthread_create(&threads[i], NULL, buck_sort,bucket_pos );
			}
			
			/*Joining all the threads*/
			for ( i = 0; i < NUM_THREADS; i++)
				pthread_join(threads[i], NULL);
			
			/*Joining all the buckets and putting in the same array*/
			for( j =0, i = 0; i < NUM_THREADS; ++i) {	
				struct Bucket_Nodes *node;
				node = buckets[i];
				while(node) {
					array_bucket[j++] = node->data;
					node = node->next;
				}
			}
			
			/*Saving the sorted elements in a .txt file*/
			ofstream output_file;
			output_file.open(argv[4]);
			
			if(output_file.is_open())
			{
				
				for(int i=0;i<=array_element-1;i++)
				{
					output_file << array_bucket[i] << "\n";
				}
				output_file << endl;
				output_file.close();
				
				
			}
			/*Free the memory allocated*/
			
			for( i = 0; i < NUM_THREADS;++i) {	
				struct Bucket_Nodes *node;
				node = buckets[i];
				while(node) {
					struct Bucket_Nodes *tmp;
					tmp = node; 
					node = node->next; 
					free(tmp);
				}
			}
			
			free(bucket_pos);
			free(buckets);
			
			/*Calulcation of thread execution time*/
			unsigned long long elapsed_ns;
			elapsed_ns = (end_time.tv_sec-start_time.tv_sec)*1000000000 + (end_time.tv_nsec-start_time.tv_nsec);
			printf("Bucket Sort : Elapsed (ns): %llu\n",elapsed_ns);
			double elapsed_s = ((double)elapsed_ns)/1000000000.0;
			printf("Bucket Sort : Elapsed (s): %lf\n",elapsed_s);
			
		}	
	}
}


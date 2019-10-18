#include <iostream>
#include <iomanip>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <atomic>

#include "mcslock.hpp"



#define MAX_THREADS 200
constexpr size_t CACHELINE_SIZE = 64;
using namespace std;
struct timespec start_time, end_time;
string lock_type;
string barrier_type;
int NUM_THREADS;
int NUM_ITERATIONS;
pthread_barrier_t *mybarrier;
pthread_mutex_t locker;
int cntr=0;
//constexpr size_t CACHELINE_SIZE = 64;
void lock_select(string lock_type);
/*------------------------------
 *------------------------------*/

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
static_assert(sizeof(TasSpinLock) == CACHELINE_SIZE, "");
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
		/*if(tail.exchange(&myNode ,nullptr))
		{}
		else
		{
			while(myNode->next.load() == NULL)
			{
				myNode->wait.store(false,memory_order_relaxed);
			}
		}*/
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
void *thread_main(void *args)
{
	int my_tid=*((int *)args);
	
	if(barrier_type=="--bar=pthread")
	{
		if(lock_type=="--lock=tas")
		{
			TasSpinLock lock;
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&start_time);
			lock.Enter();
			for(int i = 0; i<NUM_ITERATIONS; i++)
			{
				if(i%NUM_THREADS==my_tid)
					cntr++;
			}
			lock.Leave();
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&end_time);
		}
		else if(lock_type=="--lock=ttas")
		{
			TTasSpinLock lock;
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&start_time);
			lock.Enter();
			for(int i = 0; i<NUM_ITERATIONS; i++)
			{
				if(i%NUM_THREADS==my_tid)
					cntr++;
			}
			lock.Leave();
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&end_time);
		}
		else if(lock_type=="--lock=ticket")
		{
			TicketSpinLock lock;
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&start_time);
			lock.Enter();
			for(int i = 0; i<NUM_ITERATIONS; i++)
			{
				if(i%NUM_THREADS==my_tid)
					cntr++;
			}
			lock.Leave();
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&end_time);
		}
		else if(lock_type=="--lock=mcs")
		{
			
			sync::mcs_lock lock;
			sync::Node* myNode;
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&start_time);
			
			lock.Enter(myNode);
			
			for(int i = 0; i<NUM_ITERATIONS; i++)
			{
				if(i%NUM_THREADS==my_tid)
					cntr++;
			}
			lock.Leave(myNode);
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&end_time);
		}
		else if(lock_type=="--lock=pthread")
		{
			MutexLock lock;
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&start_time);
			lock.Enter();
			for(int i = 0; i<NUM_ITERATIONS; i++)
			{
				if(i%NUM_THREADS==my_tid)
					cntr++;
			}
			lock.Leave();
			if(my_tid==0)
				clock_gettime(CLOCK_MONOTONIC,&end_time);
		}
	}
	else if(barrier_type=="--bar=sense")
	{
		 
		Barrier bar;
		printf("Hello");
		clock_gettime(CLOCK_MONOTONIC,&start_time);
		for(int i = 0; i<NUM_ITERATIONS; i++)
		{
			if(i%NUM_THREADS==my_tid)
				cntr++;
		bar.wait();
		clock_gettime(CLOCK_MONOTONIC,&end_time);
		}
	}
	else
		cout << "Undefined lock" <<endl;
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
	
	
	string arg_1=argv[1];
	barrier_type=argv[5];
	string arg_5=argv[5];
	lock_type=argv[6];
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
	if(argc==9)
	{
		
		if(arg_1 == "--name")
		{
			cout << "Vignesh Iyer\n" <<endl;
					
		}
		/*--------------------------------------
		 *This argument i.e. argv[5] determines
		 *the number of threads.
		 *--------------------------------------*/
		
		sscanf(argv[3],"%d",&NUM_THREADS);
		pthread_t threads[NUM_THREADS];
		//pthread_barrier_init(barrier, NULL, NUM_THREADS);
		
		const char *iterate=argv[4];
		int total_n = 0;
		int n;
		while (1 == sscanf(iterate + total_n, "%*[^0123456789]%d%n", &NUM_ITERATIONS, &n))
		{
			total_n += n;
		}
		/*-----------------------------------------
		 *Based on the value of the arg_6 which is
		 *arg{6] the algorithm type is decided.
		 *-----------------------------------------*/
		if(arg_5 == "--bar=pthread" || arg_5 == "--bar=sense")
		{
						
			/*Creation of threads*/
			for ( i = 0; i < NUM_THREADS; i++) {
				pthread_create(&threads[i], NULL, thread_main,(void*)&i );
			}
			
			/*Joining all the threads*/
			for ( i = 0; i < NUM_THREADS; i++)
				pthread_join(threads[i], NULL);
			
			/*Saving the sorted elements in a .txt file*/
			ofstream output_file;
			output_file.open(argv[8]);
			
			if(output_file.is_open())
			{
				output_file << cntr << "\n";
				output_file << endl;
				output_file.close();
			}
			else
				cout << "Unable to open\n" << endl;
			/*Calulcation of thread execution time*/
			unsigned long long elapsed_ns;
			elapsed_ns = (end_time.tv_sec-start_time.tv_sec)*1000000000 + (end_time.tv_nsec-start_time.tv_nsec);
			printf("Counter : Elapsed (ns): %llu\n",elapsed_ns);
			double elapsed_s = ((double)elapsed_ns)/1000000000.0;
			printf("Counter : Elapsed (s): %lf\n",elapsed_s);
			
		}	
	}
	}

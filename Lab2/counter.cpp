#include <iostream>
#include <iomanip>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <atomic>

#include "mcslock.hpp"



#define MAX_THREADS 200
//constexpr size_t CACHELINE_SIZE = 64;
using namespace std;
struct timespec start_time, end_time;
string lock_type;
string barrier_type;
int NUM_THREADS;
int NUM_ITERATIONS;
pthread_barrier_t *mybarrier;
pthread_mutex_t locker;
int cntr=0;
atomic<int> cnt = {0};
//constexpr size_t CACHELINE_SIZE = 64;
void lock_select(string lock_type);
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
        const auto Ticket_value = Next_TicketNo.fetch_add(1, std::memory_order_relaxed);
 
        while (Serving_TicketNo.load(std::memory_order_acquire) != Ticket_value)
            asm("pause");
    }
 
    inline void Leave()
    {
        const auto New_Number = Serving_TicketNo.load(std::memory_order_relaxed)+1;
        Serving_TicketNo.store(New_Number, std::memory_order_release);
    }
 
private:
    std::atomic_size_t Serving_TicketNo = {0};
    std::atomic_size_t Next_TicketNo = {0};
};
static_assert(sizeof(TasSpinLock) == CACHELINE_SIZE, "");

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
	//int cnt = 0;
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
				my_sense=!(my_sense);
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
namespace sync {

    void mcs_lock::Enter() 
	{
        auto previous_node = tail.exchange(&local_node,std::memory_order_acquire);
        if(previous_node != nullptr) 
		{
            local_node.locked = true;
            previous_node->next = &local_node;
            while(local_node.locked)
                asm("pause");
        }
    }

    void mcs_lock::Leave() 
	{
        if(local_node.next == nullptr) 
		{
			mcs_node* p = &local_node;
            if(tail.compare_exchange_strong(p,nullptr,std::memory_order_release,std::memory_order_relaxed)) 
			{
                return;
            }
			while (local_node.next == nullptr) {};
        }
        local_node.next->locked = false;
        local_node.next = nullptr;
    }

    thread_local mcs_lock::mcs_node mcs_lock::local_node = mcs_lock::mcs_node{};

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
				//if(i%NUM_THREADS==my_tid)
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
		clock_gettime(CLOCK_MONOTONIC,&end_time);
		}
		bar.wait();
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

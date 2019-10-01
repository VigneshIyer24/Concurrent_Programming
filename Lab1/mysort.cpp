#include <iostream>
#include <iomanip>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <fstream>
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

/*------------------------------------
 *Structure to have merge sort with
 *values beng thread, the specific
 *high and low values for the same.
 *------------------------------------*/
struct thread_merge {
    int thread_no;
    int merge_low;
    int merge_high;
};

/* Global varaibles for bucket sort*/
int NUM_THREADS;
int INTERVAL;
int bucket_value;
int array_element;

/*Global Variables for merge sort*/
int merge_implement;
int *array_sort;

void BucketSort(int arr[]);
struct Bucket_Nodes *Bucket_InsertSort(struct Bucket_Nodes *list);
int Bucket_Number(int value);
struct Bucket_Nodes **buckets;
pthread_mutex_t lock;

/*--------------------------------
 *
 *Merge Sort Algorithm
 *
 *--------------------------------*/

/*--------------------------------------------------------------
 *@Function : void merge(int,int,int)
 *@Input : This function assigns memory to the input which is
		   both the left and right parts of the middle term.
 *@Return Value : It returns void.
 *--------------------------------------------------------------*/

void merge(int thread_merge_low, int mid, int thread_merge_high)
{

    /*Left_Merge is size of left part and Right_Merge is size of right part*/
    int Left_Merge = mid - thread_merge_low + 1;
    int Right_Merge = thread_merge_high - mid;

    int *left = malloc(Left_Merge * sizeof(int));
    int *right = malloc(Right_Merge * sizeof(int));

    int i;
    int j;

   /*Adding values to the left part of array*/
    for (i = 0; i < Left_Merge; i++)
        left[i] = array_sort[i + thread_merge_low];

    /*Adding values to the right part of array*/
    for (i = 0; i < Right_Merge; i++)
        right[i] = array_sort[i + mid + 1];

    int k = thread_merge_low;

    i = j = 0;

    while (i < Left_Merge && j < Right_Merge) {
        if (left[i] <= right[j])
            array_sort[k++] = left[i++];
        else
            array_sort[k++] = right[j++];
    }

    
    while (i < Left_Merge)
        array_sort[k++] = left[i++];

    
    while (j < Right_Merge)
        array_sort[k++] = right[j++];

    free(left);
    free(right);
}


/*-----------------------------------------------------------
 *@Function : void merge_sort(int, int)
 *@Input : This function uses the merger sort where the 
		   middle element is detected and the merge sort is
		   implemented on the left, right and finally in the
		   overall array.
 *@Return Value : This function returns a void.
 *-----------------------------------------------------------*/
 
void merge_sort(int thread_merge_low, int thread_merge_high)
{

    int mid = thread_merge_low + (thread_merge_high - thread_merge_low) / 2;

    if (thread_merge_low < thread_merge_high) 
	{
        merge_sort(thread_merge_low, mid);
        merge_sort(mid + 1, thread_merge_high);
		merge(thread_merge_low, mid, thread_merge_high);
    }
}


/*--------------------------------------------------------
 *@Function : void *merge_sort_thread(void *args)
 *@Input : This function takes a void pointer as an input
		   and inside the function this pointer is again
		   called to be parameters for a structure pointer
		   which stores the thread number/bucket number.
 *@Return Value : It is a thread so returns 0 on success.
 *--------------------------------------------------------*/

 void *merge_sort_thread(void *arg)
{
    struct thread_merge *thread_merge = arg;
    int thread_merge_low;
    int thread_merge_high;
	clock_gettime(CLOCK_MONOTONIC,&start_time);
    /* Gives the value of the structure paramters to a given variable*/
    thread_merge_low = thread_merge->merge_low;
    thread_merge_high = thread_merge->merge_high;

    int mid = thread_merge_low + (thread_merge_high - thread_merge_low) / 2;

    if (thread_merge_low < thread_merge_high) {
        merge_sort(thread_merge_low, mid);
        merge_sort(mid + 1, thread_merge_high);
        merge(thread_merge_low, mid, thread_merge_high);
    }
	clock_gettime(CLOCK_MONOTONIC,&end_time);
    return 0;
}



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
	clock_gettime(CLOCK_REALTIME,&start_time);
	pthread_mutex_lock(&lock);
	struct bucket_pos *bucket_pos=args;
	position=bucket_pos->position;
	buckets[position] = Bucket_InsertSort(buckets[position]);
	pthread_mutex_unlock(&lock);
	clock_gettime(CLOCK_REALTIME,&end_time);
	return;
}	


/*---------------------------------------------
 *@Function : void print_message()
 *@Description : This prints the message on
 *		 on how to execute on command
 *		 line.
 *---------------------------------------------*/
 
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
	int i,j;
	if(argc ==2 && arg_1 == "--name")
	{
		cout << "Vignesh Iyer\n" <<endl;
		return 1;
	}
	else if(argc < 7 && argc < 2)
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
	else if(argc==7)
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
			 
		given_file.open(argv[1]);
			
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
		given_file.open(argv[1]);
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
		if(arg_6 =="--alg=bk")
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
			output_file.open(argv[3]);
			
			if(output_file.is_open())
			{
				
				for(int i=0;i<=array_element-1;i++)
				{
					output_file << array_bucket[i] << "\n";
				}
				output_file << endl;
				output_file.close();
				
				
			}
			
			else
			{
				cout << "Unable to open output file" << endl;
				return -1;
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
		else if(arg_6=="--alg=ms")
		{
			
			merge_implement=1;
			array_sort = malloc(sizeof(int) * array_element );
			array_element =0;
			/*----------------------------------
			 *This function is of type ifstream
			 *where the input is argv[1]
			 *----------------------------------*/
			 
			given_file.open(argv[1]);
			while(given_file >> number_read)
			{
				
				array_sort[array_element]=number_read;
				array_element++;
			}
			given_file.close();
			struct thread_merge tsklist[NUM_THREADS];

			int len = array_element / NUM_THREADS;

			int thread_merge_low = 0;
			
			/*Giving the structure params some initialization*/
			for (int i = 0; i < NUM_THREADS; i++, thread_merge_low += len) 
			{
				thread_merge = &tsklist[i];
				thread_merge->thread_no = i;

				if (merge_implement) {
					thread_merge->merge_low = thread_merge_low;
					thread_merge->merge_high = thread_merge_low + len - 1;
					if (i == (NUM_THREADS - 1))
						thread_merge->merge_high = array_element - 1;
				}

			}
			
			/*Creating the threads*/
			for (int i = 0; i < NUM_THREADS; i++) 
			{
				thread_merge = &tsklist[i];
				pthread_create(&threads[i], NULL, merge_sort_thread, thread_merge);
			}	

			/*Joining the threads*/
			for (int i = 0; i < NUM_THREADS; i++)
				pthread_join(threads[i], NULL);
			
			/*Calculate the thread execution*/
			unsigned long long elapsed_ns;
			elapsed_ns = (end_time.tv_sec-start_time.tv_sec)*1000000000 + (end_time.tv_nsec-start_time.tv_nsec);
			printf("Merge Sort : Elapsed (ns): %llu\n",elapsed_ns);
			double elapsed_s = ((double)elapsed_ns)/1000000000.0;
			printf("Merge Sort : Elapsed (s): %lf\n",elapsed_s);
			
			/*Using merge sort for the combined thread array*/
			if (merge_implement) 
			{
				struct thread_merge *tskm = &tsklist[0];
				for (int i = 1; i < NUM_THREADS; i++) 
				{
					struct thread_merge *thread_merge = &tsklist[i];
					merge(tskm->merge_low, thread_merge->merge_low - 1, thread_merge->merge_high);
				}
				
			}
			
			/*Saving the sorted list in a .txt file*/
    		ofstream output_file;
			output_file.open(argv[3]);
			if(output_file.is_open())
			{
				for(int i=0;i<=array_element-1;i++)
				{

					output_file << array_sort[i] << "\n";
				}
				output_file << endl;
				output_file.close();
			}
			else
			{
				cout << "Unable to open output file" << endl;
			}
			
		}
		else
		{
			cout << "Wrong input" <<endl;
			exit(-1);
		}
	}
	
	return 0;
}

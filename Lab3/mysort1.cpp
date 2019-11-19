#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;
void merge_sort(int array_merge[],int low,int high);
void merge(int array_merge[],int left_low,int left_high,int right_low,int right_high);

int main(int argc, char *argv[])
{
    int *array_merge, low;
	int array_element=0;
	int number_threads;
	double  start_time,end_time;
	ifstream given_file;
	string arg_1=argv[1];
//	string arg_5=argv[5];
	if(argc <=2)
	{
		if(arg_1 == "--name")
		{
			cout << "Vignesh Iyer" <<endl;
			return 1;
		}
	}
	else if (argc <4 && argc>2)
	{
		if(arg_1 == "--name")
		{
			cout << "Vignesh Iyer\n" <<endl;
			return 1;
		}
	}
	else if(argc==4)
	{
		if(arg_1 == "--name")
		{
			cout << "Vignesh Iyer\n" <<endl;
			return 1;
		}
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
		array_merge = malloc(sizeof(int) * (array_element+1));
		
		array_element =0;
		while(given_file >> number_read)
		{
			//cout<<"Hello"<<endl;
			array_merge[array_element]=number_read;
			array_element++;
		}
		
		given_file.close();	
		//sscanf(argv[5],"%d",&number_threads);
		omp_set_num_threads(12);
		start_time=omp_get_wtime();
		merge_sort(array_merge, 0, array_element-1);
		end_time=omp_get_wtime();
		printf("\nTime Taken :\n");
		printf("\nTime taken=%.9f\n",end_time-start_time);
		
		/*Saving the sorted list in array_merge .txt file*/
    	
		ofstream output_file;
		output_file.open(argv[3]);
		if(output_file.is_open())
		{
			for(int low=0;low<=array_element-1;low++)
			{
				output_file << array_merge[low] << "\n";
			}
				output_file << endl;
				output_file.close();
		}
			else
			{
				cout << "Unable to open output file" << endl;
			}
		
	}	
    return 0;
	
}
/*--------------------------------
 *
 *Merge Sort Algorithm
 *
 *--------------------------------*/

/*--------------------------------------------------------------
 *@Function : void merge_sort(int[],int,int)
 *@Input : This function sorts the whole array and uses openmp
		   library for sorting which sorts the left,right and 
		   the whole array in parralel processing using a 
		   prefined number of threads i.e. 12.
 *@Return Value : It returns void.
 *--------------------------------------------------------------*/
void merge_sort(int array_merge[],int low,int high)
{
    int mid;
        
  if(low<high)
    {
        mid=(low+high)/2;
        
        #pragma omp parallel sections 
        {

            #pragma omp section
            {
                merge_sort(array_merge,low,mid);       /* Left Sort */
            }

            #pragma omp section
            {
                merge_sort(array_merge,mid+1,high);    /* Right Sort */
            }
        }

        merge(array_merge,low,mid,mid+1,high);    /* Combining the sorts */
    }
}


/*--------------------------------------------------------------
 *@Function : void merge(int[],int,int,int,int)
 *@Input : This function assigns memory to the input which is
		   both the left and right parts of the middle term and 
		   sorting function is written in the same.
 *@Return Value : It returns void.
 *--------------------------------------------------------------*/
 
void merge(int array_merge[],int left_low,int left_high,int right_low,int right_high)
{
    int *sort_temp=malloc(sizeof(int)*(right_high+1));    
    int low,high,k;
    low=left_low;    
    high=right_low;    
    k=0;
    
    while(low<=left_high && high<=right_high)    
    {
        if(array_merge[low]<array_merge[high])
            sort_temp[k++]=array_merge[low++];
        else
            sort_temp[k++]=array_merge[high++];
    }
    
    while(low<=left_high)    
        sort_temp[k++]=array_merge[low++];
        
    while(high<=right_high)   
        sort_temp[k++]=array_merge[high++];
        
    /* Transfer elements from sort_temp[] back to array_merge[] */
    for(low=left_low,high=0;low<=right_high;low++,high++)
        array_merge[low]=sort_temp[high];
}

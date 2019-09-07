/*----------------------------------------------------------------
 *@File : Lab0_sort.cpp
 * 
 *@Description : This file uses quicksort algorithm to sort
 *				 the elements in a given file and outputs 
 *				 the sorted elements in a different text file.
 *
 *@Author : Vignesh Iyer
 *
 *@Date : September 4th, 2019
 *----------------------------------------------------------------*/

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;


/*---------------------------------------------
 *@Function : void print_message()
 *@Description : This prints the message on
 *		 on how to execute on command
 *		 line.
 *---------------------------------------------*/

void print_message()
{
	cout << "Second Argument: --name\n" 
	     << "This prints the name of the person whose code this is\n\n"
	     << "Second Argument: inputfile\n"
	     << "Prints the sorted elements on the terminal"
	     <<" " << "if no other arguments are present\n\n"
	     << "Second Argument : inputfile \n"
	     << "Third Argument : -o \n"
	     << "Fourth Argument : outputfile \n"
	     << "This will put the sorted elements in the outputfile\n\n"
	     << endl;
}


/*------------------------------------------------
 *@Function : void swap_elements(float *, float *)	
 *@Input : This function takes float pointers for 
		   swapping elements where a temp variable
		   stores the value of one of the element 
		   so as swap the values.
 *@Return Value : This function is a void function.
 *------------------------------------------------*/

void swap_elements(float *element1, float *element2)
{
	float temp=*element1;
	*element1=*element2;
	*element2=temp;
}


/*---------------------------------------------------------------------
 *@Function : int sort_partition(float [], float , float)
 *@Input : This function takes floating array and the first and last
 * 	   	   index of the given array and selects a reference number here
 *	       the last index number and sorts according to the given code
 *@Return Value : This returns the index of the middle element i.e.
 * 		  		  the final index in the proper place.
 *---------------------------------------------------------------------*/

int sort_partition(float element_array[],int first_index, int last_index)
{
	int i=first_index-1;
	int j;
	float sorting_element=element_array[last_index];
	
	for(j=first_index;j<last_index;j++)
	{
		if(element_array[j]<=sorting_element)
		{
			i++;
			swap_elements(&element_array[i],&element_array[j]);
		}
	}
	
	swap_elements(&element_array[i+1],&element_array[last_index]);
	return (i+1);
}


/*------------------------------------------------
 *@Function : int sort(float [], int , int)
 *@Input : This function takes input as array and 
 * 	       the first and last index of the array 
 * 	       and does the sorting.
 *@Return Value : returns 0 on success. 
 *------------------------------------------------*/

int sort(float required_array[],int first, int last)
{
	if(last>first)
	{
		float middle_element=sort_partition(required_array,first,last);
		
		/*------------------------------------------------
		 *The partition function will only sort elements
		 *as higher or lower to the reference number. The 
		 *sort function will sort elements in order lower
		 *and higher than the reference element.
		 *------------------------------------------------*/
		 
		sort(required_array,first,middle_element-1);
		sort(required_array,middle_element+1,last);
		
	}
	return 0;
}


/*----------------------------------------------
 *@Function : void print_array(float [], int)
 *@Input : This function takes float array and
		   the size and prints the output.
 *@Return Value : No return value.
 *----------------------------------------------*/

void print_array(float arr[], int size)
{
	for(int i=0;i<=size;i++)
		cout<< arr[i] <<"\n";
	cout << endl;
}



int main(int argc, char* argv[])
{
	float given_array[500];
	int array_element=0;
	ifstream given_file;

	if(argc	< 2)
	{
		print_message();
		return 1;
	}

	if(argc <= 2)
	{
		
		string arg_1=argv[1];
		cout << "This code runs quicksort algorithm\n"
             	     << "For more help type --h or --help as an argument while"
             	     << " " << "executing the code\n" << endl;

		if(arg_1 == "--name")
		{
			cout << "Vignesh Iyer\n" <<endl;
			return 1;
		}

		else if(arg_1 == "--h" || arg_1 == "--help")
        	{
                	print_message();
                	return 1;
        	}	

		else
		{
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
		}
	}
	else if (argc < 5 && argc > 2)
	{
		string arg_1=argv[1];
		if(arg_1 == "--name")
		{
			cout << "Vignesh Iyer\n" <<endl;
			return 1;
		}
		else
		{
			/*----------------------------------
			 *This function is of type ifstream
			 *where the input is argv[1]
			 *----------------------------------*/
			 
			given_file.open(argv[1]);
			if (!given_file)
			{
				cerr << "Unable to find input file ByeBye\n";
				exit(-1);   
			}
		}
	}
	else if(argc >=5)
	{
		cerr << "Too many arguments to process\n"
		     << "Have a nice day\n" << endl;
		return -1;
	} 
	float number_read;
	
	/*--------------------------------
	 *The values from the file will be 
	 *put in the array defined which 
	 *will be used for sorting.
	 *--------------------------------*/
	 
	while(given_file >> number_read)
	{
		given_array[array_element]=number_read;
		array_element++;
	}
	given_file.close();
	
	sort(given_array,0,array_element-1);
	
	if(argc < 3)
	{
		print_array(given_array,array_element-1);
	}
	else if (argc < 5 && argc > 2)
	{
			/*-------------------------------
			 *The sorted elements will be an
			 *input to a file which will be 
			 *created if it does not exist 
			 *using ofstream.
			 *-------------------------------*/
			
			ofstream output_file;
			output_file.open(argv[3]);
			if(output_file.is_open())
			{
				for(int i=0;i<=array_element-1;i++)
					output_file << given_array[i] << "\n";
				output_file << endl;
				output_file.close();
			}
			else
				cout << "Unable to open output file" << endl;
	}
	return 0;
}
			
		
			

	
	
	

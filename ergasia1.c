#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char **argv) {
    int rc; // for checking if the MPI_initialization is successful 
    int rank;
    int N; // the size of the Series, that the user will enter (the amount of elements)
    int p;
    int *Tarray = NULL; // pointer declaration for dynamic memory allocation for the Series Array
    int *Tarray_loc = NULL; // pointer declaration dynamic memory allocation for the local Array
    int menu_answer, i, tag1 = 20, tag2 = 30, tag3 = 40, tag4 = 50, tag5 = 60, tag6 = 70; // each tag will be used for the different sets of MPI_Send - MPI_Recv
    int target; // for MPI_Send
    int source; // for MPI_Recv
    int k, num; // will be used for splitting the elements in the Tarray evenly to each process
    int flag, pos, flag_global, pos_global; // for checking if the Series is sorted and for keeping the index in case it is not sorted 
    int last_element, first_element, previous_last; // helping variables for comparing elements on different subarrays 
    MPI_Status status; // declaration of a MPI_Status struct that will be used on MPI_Recv 

    rc = MPI_Init(&argc, &argv);
    
    if (rc != 0) { // in rc a number is stored if that number is not zero the initialization failed and the program should abort 
        fprintf(stderr, "MPI Initialization error!\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &p); // get the amount of processes 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // get the rank of each process

    do {
        if (rank == 0) {
            printf("\nMenu:\n"); // menu prompts
            printf("1. Enter new Series\n");
            printf("2. Exit\n");
            printf("Enter your choice: ");
            fflush(stdout);
            scanf("%d", &menu_answer);

            for (target = 1; target < p; target++) {
                MPI_Send(&menu_answer, 1, MPI_INT, target, tag3, MPI_COMM_WORLD); // sending the answer of the menu that the user gave to all the other processes 
            }

            if (menu_answer == 2) { // if the user entered "2" the program should terminate
                printf("Exiting...\n");
                MPI_Finalize();
                return 0;
            }

            printf("Give the length of the Series: "); // prompt for the length of the Series
            fflush(stdout);
            scanf("%d", &N);

            Tarray = (int *)malloc(N * sizeof(int)); // dynamically allocating memory based on the N the user gave ( N = number of elements)
            
            printf("Enter the %d elements of the array:\n", N); // prompt for the user to enter each element of the series
            fflush(stdout);
            for (i = 0; i < N; i++) {
                scanf("%d", &Tarray[i]); // reading the elements, for loop needed 
            }

            for (target = 1; target < p; target++) {
                MPI_Send(&N, 1, MPI_INT, target, tag1, MPI_COMM_WORLD); // sending the N variable(amount of elements) to every other process
            }

            num = N / p; // amount of elements for each process = ( total amount of elements) / ( total amount of processes)
            k = num;

            Tarray_loc = (int *)malloc(num * sizeof(int)); // dynamically allocating memory for the local array based on num variable that will be used by each process seperately
            
            for (target = 1; target < p; target++) {
                MPI_Send(&Tarray[k], num, MPI_INT, target, tag2, MPI_COMM_WORLD); //sending each subarray to each process based on k variable 
                k += num; // k variable should be incremented each time by num which contains the number of elements for each subarray for every process to get its num elements
            }

            for (k = 0; k < num; k++) {
                Tarray_loc[k] = Tarray[k]; // process 0 will also participate in the checking and it should keep the first num elements of the Series
            }

            free(Tarray); // free Tarray memory for preventing memory leaks
            
        } else {
            MPI_Recv(&menu_answer, 1, MPI_INT, 0, tag3, MPI_COMM_WORLD, &status); // every process except zero receives the menu_answer that the user entered

            if (menu_answer == 2) { // if the user entered 2 every other process should also terminate and the program exit 
                printf("Process %d: Exiting...\n", rank);
                MPI_Finalize();
                return 0;
            }

            MPI_Recv(&N, 1, MPI_INT, 0, tag1, MPI_COMM_WORLD, &status); // every other process receives the length of the series
            
            num = N / p; // recalculating num which is the amount of elements for each process 

            Tarray_loc = (int *)malloc(num * sizeof(int)); // reallocate dynamicaly the memory of the local array
            
            MPI_Recv(&Tarray_loc[0], num, MPI_INT, 0, tag2, MPI_COMM_WORLD, &status); // each process receives its subarray of num elements 

            // printf("Process %d received %d elements: ", rank, num);
            
            /*for (i = 0; i < num; i++) {
                printf("%d ", Tarray_loc[i]);
            }
            */
            
            printf("\n");
        }

        // local sorting check
        flag = 1; // initialize a value for the flag variable
        pos = -1; // starting pos value 
        for (k = 0; k < num - 1; k++) {
            if (Tarray_loc[k] > Tarray_loc[k + 1]) {
                flag = 0; // change the flag value 
                pos = rank * num + k; // finding the first index where the grading stops, for example if it breaks at index 2  global index rank * num should be zero + local index k = 2
                break; 
            }
        }

        
        last_element = Tarray_loc[num - 1]; // each process stores its last subarray element in the variable last_element
        if (rank < p - 1) { // the first p - 1 processes pass the last_element to the next process 
            MPI_Send(&last_element, 1, MPI_INT, rank + 1, tag5, MPI_COMM_WORLD); // sending the last_element to the next process 
        }
        if (rank > 0) {
            MPI_Recv(&previous_last, 1, MPI_INT, rank - 1, tag5, MPI_COMM_WORLD, &status); // the next process receives the last_element from the previous process's subarray
            if (previous_last > Tarray_loc[0]) { //if the last element from the previous subaray is greater than the first element from the current process's subarray
                flag = 0; // make flag 0  
                pos = rank * num; // global index of the first element in the subarray
            }
        }

       
        if (rank == 0) {
            flag_global = flag;
            pos_global = pos;
            for (source = 1; source < p; source++) { // for every process besides zero 
            
                int local_flag, local_pos;
                
                MPI_Recv(&local_flag, 1, MPI_INT, source, tag4, MPI_COMM_WORLD, &status); // process 0 receives the flag variable of each process 
                MPI_Recv(&local_pos, 1, MPI_INT, source, tag6, MPI_COMM_WORLD, &status); // process 0 receives the pos variable of each process 

                if (local_flag == 0) { //if process 0 receives a local_flag with value 0
                    flag_global = 0; // this means that the series is not sorted properly 
                if (pos_global == -1 || (local_pos != -1 && local_pos < pos_global)) { // finds the earliest pos index if it exists by comparing the local_pos with the global if a local_pos is detected
                        pos_global = local_pos;
                    }
                }
            }
            if (flag_global) { // if flag_global = 1
                printf("Yes, the array is sorted.\n");
            } else {
                printf("No, the array is not sorted, it breaks at index: %d\n", pos_global);
            }
        } else {
            MPI_Send(&flag, 1, MPI_INT, 0, tag4, MPI_COMM_WORLD); // every process except zero sends their flag value to process 0
            MPI_Send(&pos, 1, MPI_INT, 0, tag6, MPI_COMM_WORLD); // every process except zero sends their pos value to process 0
        }

        free(Tarray_loc); // free memory space for local array

    } while (1);

    MPI_Finalize(); // Finalize MPI
    
    return 0;
}

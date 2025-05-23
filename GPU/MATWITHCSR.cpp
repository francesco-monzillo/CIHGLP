#include <omp.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
//#include <cstring>
#include "../HYIMPL/SIMPLEMAT.h"
#include "../HYIMPL/utils.h"

#define MAXITERATIONS 100000

//#pragma omp requires unified_shared_memory


void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    printf("Caught segfault at address %p\n", si->si_addr);
    //exit(0);

}


int main(int argc, char* argv[]) {

    /*int num_devices = omp_get_num_devices();
    std::cout << "Number of available target devices: " << num_devices << "\n";
    return 0;*/

    // Initialize the SimpleMat structure
    SimpleMatGPU mat;
    
    /*std::cout << "Enter the number of rows: \n";

    std::cin >> mat.nrow;

    std::cout << "Enter the number of columns: \n";

    std::cin >> mat.ncol;
    */
   
    int categories = 0;
    
    std::cout << "Enter the number of categories: \n";

    std::cin >> categories;

    //One more category for the 0 label (corresponding to the nodes without label)
    //categories = categories + 1;

    std::ifstream hIncidenceFile(argv[1]);
    std::ifstream nLabelsFile(argv[2]);


    mat.incidenceMatrix = extractIncidenceMatrix(&hIncidenceFile, &nLabelsFile, &mat.nrow, &mat.ncol);

    //Declaring labels array
    int* nodelabels = extractNodeLabels(&nLabelsFile, mat.nrow);
    int* edgelabels = (int*) calloc(mat.ncol, sizeof(int));
    /*
    // Initialize the signal handler for segmentation faults
    // This will help us catch segmentation faults and print the address
    // where the fault occurred
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);
    */

    bool stop = false;

    int rounds = 1;

    long long int rows = mat.nrow;
    long long int cols = mat.ncol;

    bool* matrix = mat.incidenceMatrix;

    long long int* row_major_Non_zeroes_for_rows = (long long int*) malloc(sizeof(long long int) * (rows + 1));

    long long int* row_major_Non_zeroes_for_columns = (long long int*) malloc(sizeof(long long int) * 1);

    long long int* column_major_Non_zeroes_for_rows = (long long int*) malloc(sizeof(long long int) * 1);

    long long int* column_major_Non_zeroes_for_columns = (long long int*) malloc(sizeof(long long int) * (cols + 1));

    row_major_Non_zeroes_for_rows[0] = 0;
    column_major_Non_zeroes_for_columns[0] = 0;

    //DEFINING COUNTER FOR ROW_MAJOR_SIZE 
    long long int row_major_size = 0;

    //BUILDING ROW-MAJOR CSR FOR THE MATRIX
    #pragma omp parallel for shared(rows, cols, matrix)
    for(int i = 0; i < rows; ++i) {
        int count = 0;
        for (int j = 0; j < 10; ++j){
            if(matrix[(i * cols) + j] == 1) {
                count++;
        
                //REALLOCATING THE MEMORY FOR POSITIONS DYNAMIC ARRAY
                long long int* pnt = (long long int*) realloc(row_major_Non_zeroes_for_columns, sizeof(long long int) * (row_major_size + 1));
                row_major_Non_zeroes_for_columns = pnt;
                if(pnt == NULL){
                    std::cerr << "Memory allocation failed\n";
                    exit(1);
                }
                row_major_Non_zeroes_for_columns[row_major_size] = j;
                row_major_size++;
            }
        }
        row_major_Non_zeroes_for_rows[i+1] = row_major_Non_zeroes_for_rows[i] + count;
    }

    //DEFINING COUNTER FOR COLUMN_MAJOR_SIZE 
    long long int column_major_size = 0;

    //BUILDING COLUMN-MAJOR CSR FOR THE MATRIX
    #pragma omp parallel for shared(rows, cols, matrix)
    for(int j = 0; j < cols; ++j) {
        int count = 0;
        for (int i = 0; i < rows; ++i){
            if(matrix[(i * cols) + j] == 1) {
                count++;

                long long int* pnt = (long long int*) realloc(column_major_Non_zeroes_for_rows, sizeof(long long int) * (column_major_size + 1));
                column_major_Non_zeroes_for_rows = pnt;
                column_major_Non_zeroes_for_rows[column_major_size] = i;
                column_major_size++;
            }
        }
        std::cout << "j: " << j << "\n";
        column_major_Non_zeroes_for_columns[j+1] = column_major_Non_zeroes_for_columns[j] + count;
    }

    //INDEX USED FOR THE CSR INDEXING FOR VALID POSITIONS
    long long int pos_index = 0;

    double start = omp_get_wtime();


    // Main loop
    // The loop will run until convergence or until the maximum number of iterations is reached
    #pragma omp target enter data map(to: row_major_Non_zeroes_for_rows [0: rows + 1],  column_major_Non_zeroes_for_columns[0: cols + 1] , row_major_Non_zeroes_for_columns[0: row_major_size], column_major_Non_zeroes_for_rows[0: column_major_size], nodelabels[0:rows], edgelabels[0:cols])

    int categoryCount[categories + 1] = {0};
    int change = false;
    int max = -1;
    int maxIndex = -1;

        do{
        stop = true;


        //Sending matrix and label vectors to the device.
        //Retrieving the labels from the device.// map the data to the device
        
            //Dividing iteration space on columns
            //Inverting the matrix indexes because we are using a the transposed matrix (column-major order)
            #pragma omp target
            #pragma omp teams distribute parallel for simd shared(rows, cols, nodelabels, column_major_Non_zeroes_for_rows, column_major_Non_zeroes_for_columns) firstprivate(categoryCount, max, maxIndex, change, pos_index)
            for (int j = 0; j < cols; ++j) {
                long long int iterations = column_major_Non_zeroes_for_columns[j+1] - column_major_Non_zeroes_for_columns[j];
                for (int i = 0; i <= iterations; ++i) {

                    //NEXT VALID POSITION IN THE CSR
                    long long int nextPosition = column_major_Non_zeroes_for_rows[pos_index];

                    //EXTRACTING THE LABEL FROM THE VALID POSITION
                    int label = nodelabels[nextPosition];

                    //INCREMENTING THE INDEX FOR THE NEXT VALID POSITION
                    pos_index++;

                    //printf("mat_value %d\n", matrix[(i * cols) + j]);
                    //If the value is in list 1, we count it
                    //If the value is in list is 0, we do not count it
                    categoryCount[label] = categoryCount[label] + 1;
   
                    if(label != 0 && categoryCount[label] > max) {
                        max = categoryCount[label];
                        maxIndex = label;
                    }
                }
            
                //AVOIDING CONDITIONAL STATEMENTS INSIDE THE DEDVICE PARALLEL REGION
                change = edgelabels[j] != maxIndex;
                edgelabels[j] = maxIndex; 

 

                for (int k = 1; k <= categories; ++k) {
                    categoryCount[k] = 0;
                }
        
                max = -1;
                maxIndex = -1; 

    
            }

            //YOU NEED TO STUDY THE BEHAVIOUR OF CHANGE AND STOP VARIABLES OUTSIDE AND INSIDE THE HOST
            //Dividing iteration space on rows
            #pragma omp target
            #pragma omp teams distribute parallel for simd shared(rows, cols, edgelabels, row_major_Non_zeroes_for_rows, row_major_Non_zeroes_for_columns) firstprivate(categoryCount, max, maxIndex, change, pos_index)
            for (int i = 0; i < rows; ++i) {
                long long int iterations = row_major_Non_zeroes_for_rows[i+1] - row_major_Non_zeroes_for_columns[i];
                    //printf("rounds: %d\nstop %d\n", rounds, stop);
                    for (int j = 0; j < cols; ++j) {
                
                        //NEXT VALID POSITION IN THE CSR
                        long long int nextPosition = row_major_Non_zeroes_for_columns[pos_index];
                        
                        //EXTRACTING THE LABEL FROM THE VALID POSITION
                        int label = edgelabels[nextPosition];

                        //INCREMENTING THE INDEX FOR THE NEXT VALID POSITION
                        pos_index++;

                        //If the value is in list 1, we count it
                        //If the value is in list is 0, we do not count it
                        categoryCount[label] = categoryCount[label] + 1;

                        if(label != 0 && categoryCount[label] > max) {
                            max = categoryCount[label];
                            maxIndex = label;
                        }
                    }

            
                    //AVOIDING CONDITIONAL STATEMENTS INSIDE THE DEDVICE PARALLEL REGION
                    change = nodelabels[i] != maxIndex;
                    nodelabels[i] = maxIndex;

                    for (int k = 1; k <= categories; ++k) {
                        categoryCount[i] = 0;
                    }
        
                    max = -1;
                    maxIndex = -1; 

                }

                if(change){
                    stop = !change;
                }

        //printf("stop: %d\nround: %d\n", change,rounds);
        change = false;
        rounds++;
    }while(!stop && rounds < MAXITERATIONS);
    
    // Exit the target region and copy the data back to the host
    #pragma omp target exit data map(from: nodelabels[0:rows], edgelabels[0:cols])


    double end = omp_get_wtime();

    // Print the final labels
    std::cout << "Final node labels:\n";
    for (int i = 0; i < mat.nrow; ++i) {
        std::cout << "Node " << i << ": " << nodelabels[i] << "\n";
    }
    std::cout << "Final edge labels:\n";
    for (int j = 0; j < mat.ncol; ++j) {
        std::cout << "Edge " << j << ": " << edgelabels[j] << "\n";
    }

    std::cout << "Number of rounds: " << rounds << "\n";

    std::cout << "Execution time: " << end - start << " seconds\n";


    // Free the allocated memory
    free(mat.incidenceMatrix);
    free(nodelabels);
    free(edgelabels);
    return 0;
}
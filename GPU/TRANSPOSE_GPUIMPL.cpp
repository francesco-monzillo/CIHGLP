#include <omp.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
//#include <cstring>
#include "../HYIMPL/SIMPLEMAT.h"
#include "../HYIMPL/utils.h"
#define NCATEGORIES 20
#define MAXITERATIONS 100

//#pragma omp requires unified_shared_memory


void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    printf("Caught segfault at address %p\n", si->si_addr);
    //exit(0);

}


int main(int argc, char* argv[]) {

    // Initialize the SimpleMat structure
    SimpleMatGPU mat;
    
    std::cout << "Enter the number of rows: \n";

    std::cin >> mat.nrow;

    std::cout << "Enter the number of columns: \n";

    std::cin >> mat.ncol;
    

    //One more category for the 0 label (corresponding to the nodes without label)
    //categories = categories + 1;

    std::ifstream hIncidenceFile(argv[1]);
    std::ifstream nLabelsFile(argv[2]);


    /*
    mat.incidenceMatrix = extractIncidenceMatrix(&hIncidenceFile, &nLabelsFile, &mat.nrow, &mat.ncol);

    //Declaring labels array
    int* nodelabels = extractNodeLabels(&nLabelsFile, mat.nrow);
    */

    int* edgelabels = (int*) malloc(mat.ncol * sizeof(int));
    int* nodelabels = (int*) malloc(mat.nrow * sizeof(int));
    mat.incidenceMatrix = (bool*) malloc(mat.nrow * mat.ncol * sizeof(bool));
    
    // Seed the random number generator
    srand(static_cast<unsigned>(time(0)));


    printf("ciao\n");
    // Initialize the incidence matrix

    long long int rows = mat.nrow;
    long long int cols = mat.ncol;

    bool* matrix = mat.incidenceMatrix;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            // Randomly assign 0 or 1 to the incidence matrix
            // Here we just use a random number generator for demonstration purposes
            if(i==0){
                edgelabels[j] = 0;
            }
            int randNum = rand() % 10;
            if(randNum < 3)
                mat.incidenceMatrix[(i * cols) + j] = 1;
            else
                mat.incidenceMatrix[(i * cols) + j] = 0;
            //std::cout << mat.incidenceMatrix[(i * mat.ncol) + j] << " ";
        }
        //std::cout << "\n";
        nodelabels[i] = rand() % NCATEGORIES + 1;
    }


    bool stop = false;

    int rounds = 1;

    //MAX NUMBER OF TEAMS
    long long int workGNumForEdges = mat.ncol / omp_get_teams_thread_limit();
    long long int workGNumForRows = mat.nrow / omp_get_teams_thread_limit();

    //MAX NUMBER OF THREADS INSIDE A TEAM
    long long int maxThreads = omp_get_max_threads();

    //CALL FUNCTION TO CALCULATE THE SCHEDULING ARRAYS CONTAINING INDICES FOR EACH WORK-ITEM (WORK IN PROGRESS))

    long long int matsize = mat.nrow * mat.ncol;


    bool* matrix2 = (bool*) malloc(sizeof(bool) * matsize);
    printf("Matrix size: %lld\n", matsize);

    #pragma omp target enter data map(alloc: matrix2[0:matsize])

    bool change = false;


    int stride = 16;
    

    #pragma omp target enter data map(to: matrix[0:matsize])
    // Transpose the matrix
    
    #pragma omp target
    #pragma omp teams distribute parallel for simd collapse(2) firstprivate(rows, cols, stride)
    for(int li = 0; li < rows; li+=stride) {
        for(int lj = 0; lj < cols; lj+=stride) {
            for(int i = 0; i < stride; ++i) {
                for (int j = 0; j < stride; ++j){
                    matrix2[(lj + j) * rows + (li + i)] = matrix[(li + i) * cols + (lj + j)];
                    //printf("(%d, %d)", matrix2[j * rows + i], matrix[i * cols + j]);
                }
            }
        }
    }
    double start2 = omp_get_wtime();

    /*#pragma omp target
    #pragma omp teams distribute parallel for simd firstprivate(rows, cols)
    for(int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j){
            matrix2[(j) * rows + (i)] = matrix[(i) * cols + (j)];
                //printf("(%d, %d)", matrix2[j * rows + i], matrix[i * cols + j]);
        }
    }*/
    double end2 = omp_get_wtime();
    
    std::cout << "Transpose execution time: " << end2 - start2 << " seconds\n";

    double start = omp_get_wtime();

    // Main loop
    // The loop will run until convergence or until the maximum number of iterations is reached
    #pragma omp target enter data map(to: edgelabels[0:cols], nodelabels[0:rows]) // map the data to the device

        do{
        stop = true;


        //Sending matrix and label vectors to the device.
        //Retrieving the labels from the device.// map the data to the device
        
            //Dividing iteration space on columns
            //Inverting the matrix indexes because we are using a the transposed matrix (column-major order)
            #pragma omp target map(tofrom: change)
            #pragma omp teams distribute parallel for firstprivate(rows, cols, nodelabels) reduction(|:change)
            for (int j = 0; j < cols; ++j) {

                int categoryCount[NCATEGORIES + 1] = {0};
                int max = -1;
                int maxIndex = -1;

                for (int i = 0; i < rows; ++i) {
        
                
                    int label = nodelabels[i];
                    //printf("label %d\n", label);

                    //printf("mat_value %d\n", matrix[(i * cols) + j]);
                    //If the value is in list 1, we count it
                    //If the value is in list is 0, we do not count it
                    categoryCount[label] = categoryCount[label] + matrix2[(j * rows) + i];
                }
            
                //AVOIDING CONDITIONAL STATEMENTS INSIDE THE DEVICE PARALLEL REGION
                for(int k = 1; k <= NCATEGORIES; ++k) {
                    if(categoryCount[k] > max) {
                        max = categoryCount[k];
                        maxIndex = k;
                    }
                }

                int edgelabel = edgelabels[j];

                if(edgelabel != maxIndex)
                    change = true;

                edgelabels[j] = maxIndex;

            }

 
            //YOU NEED TO STUDY THE BEHAVIOUR OF CHANGE AND STOP VARIABLES OUTSIDE AND INSIDE THE HOST
            //Dividing iteration space on rows
            #pragma omp target map(tofrom: change)
            #pragma omp teams distribute parallel for firstprivate(rows, cols, edgelabels) reduction(|:change)
            for (int i = 0; i < rows; ++i) {
                    //printf("rounds: %d\nstop %d\n", rounds, stop);
                int categoryCount[NCATEGORIES + 1] = {0};
                int max = -1;
                int maxIndex = -1;

                    for (int j = 0; j < cols; ++j) {
                
                        int label = edgelabels[j];

                        //If the value is in list 1, we count it
                        //If the value is in list is 0, we do not count it
                        categoryCount[label] = categoryCount[label] + matrix[(i * cols) + j];
                    }

            
                    //AVOIDING CONDITIONAL STATEMENTS INSIDE THE DEDVICE PARALLEL REGION

                    for(int k = 1; k <= NCATEGORIES; ++k) {
                        if(categoryCount[k] > max) {
                            max = categoryCount[k];
                            maxIndex = k;
                        }
                    }

                    int nodeLabel = nodelabels[i];
                    if(nodeLabel != maxIndex)
                        change = true;

                    nodelabels[i] = maxIndex;

            }

            if(change){
                stop = false;
            }

        //printf("stop: %d\nround: %d\n", change,rounds);
        change = false;
        rounds++;
    }while(!stop &&  rounds < MAXITERATIONS);
    
    // Exit the target region and copy the data back to the host
    #pragma omp target exit data map(from: nodelabels[0:rows], edgelabels[0:cols])

    double end = omp_get_wtime();



    // Print the final labels
    //std::cout << "Final node labels:\n";
    //for (int i = 0; i < mat.nrow; ++i) {
    //    std::cout << "Node " << i << ": " << nodelabels[i] << "\n";
    //}
    //std::cout << "Final edge labels:\n";
    //for (int j = 0; j < mat.ncol; ++j) {
       // std::cout << "Edge " << j << ": " << edgelabels[j] << "\n";
    //}

    std::cout << "Number of rounds: " << rounds << "\n";

    std::cout << "Execution time: " << end - start << " seconds\n";


    // Free the allocated memory
    free(mat.incidenceMatrix);
    free(nodelabels);
    free(edgelabels);
    return 0;
}
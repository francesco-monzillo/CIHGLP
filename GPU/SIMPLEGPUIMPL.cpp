#include <omp.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
//#include <cstring>
#include "../HYIMPL/SIMPLEMAT.h"

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

    std::string lineString;

    long long int index = 0;
    

    //counting number of columns FROM DATASET
    while(std::getline (hIncidenceFile, lineString)) {
        index++;
    }

    mat.ncol = index;
    index = 0;
    

    //counting number of rows
    while(std::getline (nLabelsFile, lineString)) {
        index++;
    }

    mat.nrow = index;


    //Avoiding overflow
    long long int matsize = mat.nrow * mat.ncol;

    //Declaring labels array
    int* nodelabels = (int*) calloc(mat.nrow, sizeof(int));
    int* edgelabels = (int*) calloc(mat.ncol, sizeof(int));

    mat.incidenceMatrix = (bool*) calloc(matsize, sizeof(bool));

    // Reset the file stream to the beginning
    hIncidenceFile.clear();
    hIncidenceFile.seekg(0, hIncidenceFile.beg);

    // Reset the file stream to the beginning
    nLabelsFile.clear();
    nLabelsFile.seekg(0, nLabelsFile.beg);


    index = 0;
    std::string s = "";

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

    // Use a while loop together with the getline() function to read the file line by line
    //READING THE INCIDENCE MATRIX
    while (std::getline (hIncidenceFile, lineString) && index < mat.ncol) {
    
        std::cout << "index "<< index << ": ";

        for(char c : lineString) {
            if(isdigit(c)) {
                s = s + c;
            }else{
                if(!s.empty()) {
                    //std::cout << s << " ";
                    int row = std::stoi(s);
                    
                    std::cout << "ROW Value: " << row << "\n";
                    std::cout << "Value: " << (row-1) * mat.ncol << "\n";
                    long long int offset = (((row - 1) * mat.ncol) + index);
                    mat.incidenceMatrix[offset] = true;
                    s = "";
                }
            }
        }

        //NEEDED FOR THE LAST LINE
        if(hIncidenceFile.peek()==EOF && !s.empty()) {
            std::cout << s << " ";
            long long int row = std::stoi(s);
            long long int offset = (((row - 1) * mat.ncol) + index);
            mat.incidenceMatrix[offset] = true;
        }

        s = "";

        index++;
    }

    index = 0;

    //READING THE NODE LABELS
    while (std::getline (nLabelsFile, lineString)) {
        nodelabels[index] = std::stoi(lineString);

        index++;
    }

    /*
    for(int i = 0; i < mat.nrow; ++i) {
        std::cout << "Node " << i << ": " << nodelabels[i] << "\n";
    }*/ 


    // Seed the random number generator
    srand(static_cast<unsigned>(time(0)));

    
    /*COMMENTING RANDOM MATRIX
    // Initialize the incidence matrix
    for (int i = 0; i < mat.nrow; ++i) {
        for (int j = 0; j < mat.ncol; ++j) {
            // Randomly assign 0 or 1 to the incidence matrix
            // Here we just use a random number generator for demonstration purposes
            if(i==0){
                edgelabels[j] = 0;
            }
            mat.incidenceMatrix[(i * mat.ncol) + j] = rand() % 2;
            std::cout << mat.incidenceMatrix[(i * mat.ncol) + j] << " ";
        }
        std::cout << "\n";
        nodelabels[i] = rand() % categories + 1;
    }

    */



    bool stop = false;

    int rounds = 1;

    long long int rows = mat.nrow;
    long long int cols = mat.ncol;

    bool* matrix = mat.incidenceMatrix;


    //BUILDING A RANDOM LOAD DISTRIBUTION ALTERNATIVE (IN PROGRESS)
    /*for(int i = 0; i < mat.nrow; ++i) {
        for (int j = 0; j < mat.ncol; ++j){
            bool switchvar = (rand() % 2) == 1;
            if(switchvar){
                bool switchrows = (rand() % 2) == 1;
                if(switchrows){
                    
                    bool switchIndex = rand() % rows;

                    for(int k=0; k < cols; ++k){
                        long long int offset = (i * mat.ncol) + k;
                        bool temp = matrix[offset];
                        matrix[offset] = matrix[j * mat.ncol + k];
                        matrix[j * mat.ncol + k] = temp;
                    }
                }
                long long int offset = (i * mat.ncol) + j;
                bool temp = matrix[offset];
                matrix[offset] = matrix[j * mat.ncol + i];
                matrix[j * mat.ncol + i] = temp;
            }
        }
    }*/


    //TESTING THE TRANSPOSITION CORRECTNESS
    /*#pragma omp parallel for collapse(2) shared(rows, cols, matrix, matrix2)
    for(int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j){
            bool isEqual = (matrix2[(i * rows) + j] == matrix[j * cols + i]) ? 1 : 0;
            printf("%d: (%d, %d)\n",isEqual, i, j);
        }
    }

    return 0;*/

    double start = omp_get_wtime();


    // Main loop
    // The loop will run until convergence or until the maximum number of iterations is reached
    #pragma omp target enter data map(to: matrix[0:matsize], nodelabels[0:mat.nrow], edgelabels[0:mat.ncol])

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
            #pragma omp teams distribute parallel for simd shared(rows, cols, nodelabels, matrix) firstprivate(categoryCount, max, maxIndex, change)
            for (int j = 0; j < cols; ++j) {
                for (int i = 0; i < rows; ++i) {
                
                    int label = nodelabels[i];

                    //printf("mat_value %d\n", matrix[(i * cols) + j]);
                    //If the value is in list 1, we count it
                    //If the value is in list is 0, we do not count it
                    categoryCount[label] = categoryCount[label] + matrix[(i * cols) + j];
   
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
            #pragma omp teams distribute parallel for simd shared(rows, cols, edgelabels, matrix) firstprivate(categoryCount, max, maxIndex, change)
            for (int i = 0; i < rows; ++i) {
                    //printf("rounds: %d\nstop %d\n", rounds, stop);
                    for (int j = 0; j < cols; ++j) {
                
                        int label = edgelabels[j];

                        //If the value is in list 1, we count it
                        //If the value is in list is 0, we do not count it
                        categoryCount[label] = categoryCount[label] + matrix[(i * cols) + j];

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
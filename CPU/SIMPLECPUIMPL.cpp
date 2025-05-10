#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include "../HYIMPL/SIMPLEMAT.h"

#define MAXITERATIONS 100000

int main(int argc, char* argv[]) {

    // Initialize the SimpleMat structure
    SimpleMat mat;
    std::cout << "Enter the number of rows: \n";

    std::cin >> mat.nrow;

    std::cout << "Enter the number of columns: \n";

    std::cin >> mat.ncol;
    
    int categories = 0;

    std::cout << "Enter the number of categories: \n";

    std::cin >> categories;

    //One more category for the 0 label (corresponding to the nodes without label)
    //categories = categories + 1;
    
    int processors = 0;

    std::cout << "Enter the number of processors: \n";
    std::cin >> processors;

    /* COMMENTING OUT THE FILE READING PART FOR TESTING PURPOSES
    

    std::ifstream hIncidenceFile(argv[1]);
    std::ifstream nLabelsFile(argv[2]);

    std::string lineString;

    int index = 0;
    
    //counting number of columns
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
    */

    //Declaring labels array
    int* nodelabels = (int*) calloc(mat.nrow, sizeof(int));
    int* edgelabels = (int*) calloc(mat.ncol, sizeof(int));

    mat.incidenceMatrix = (bool**) malloc(sizeof(bool*) * mat.nrow);

    for(int i = 0; i < mat.nrow; ++i) {
        
        bool* row = (bool*) calloc(mat.ncol, sizeof(bool));
        mat.incidenceMatrix[i] = row;

        //Automatically assigned to false and 0 :)
        /*for (int j = 0; j < mat.ncol; j++){
           mat.incidenceMatrix[i][j] = false;
           edgelabels[j] = 0;
        }*/

    }


    /*
    // Reset the file stream to the beginning
    hIncidenceFile.clear();
    hIncidenceFile.seekg(0, hIncidenceFile.beg);

    // Reset the file stream to the beginning
    nLabelsFile.clear();
    nLabelsFile.seekg(0, nLabelsFile.beg);


    index = 0;
    std::string s = "";
    */
    
    
    /* COMMENTING OUT THE FILE READING PART FOR TESTING PURPOSES
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
                    int row =std::stoi(s);
                    mat.incidenceMatrix[row - 1][index] = true;
                    s = "";
                }
            }
        }

        //NEEDED FOR THE LAST LINE
        if(hIncidenceFile.peek()==EOF && !s.empty()) {
            std::cout << s << " ";
            int row =std::stoi(s);
            mat.incidenceMatrix[row - 1][index] = true;
        }

        s = "";

        index++;
    }

    index = 0;

    
    //READING THE NODE LABELS
    while (std::getline (nLabelsFile, lineString)) {
        //Adding 1 to the label to avoid 0 labels (will be used for the nodes without label)
        nodelabels[index] = std::stoi(lineString) + 1;
        index++;
    }
    */


    /*for(int i = 0; i < mat.nrow; ++i) {
        std::cout << "Node " << i << ": " << nodelabels[i] << "\n";
    }*/   

    // Seed the random number generator
    srand(static_cast<unsigned>(time(0)));


    // Initialize the incidence matrix
    for (int i = 0; i < mat.nrow; ++i) {
        for (int j = 0; j < mat.ncol; ++j) {
            // Randomly assign 0 or 1 to the incidence matrix
            // Here we just use a random number generator for demonstration purposes
            if(i==0)
                edgelabels[j] = 0;
            mat.incidenceMatrix[i][j] = rand() % 2;
            std::cout << mat.incidenceMatrix[i][j] << " ";
        }
        std::cout << "\n";

        nodelabels[i] = rand() % categories + 1;

    }


    bool stop = false;

    int rounds = 1;



    double start = omp_get_wtime();

    // Main loop
    // The loop will run until convergence or until the maximum number of iterations is reached
    
   
    //MAYBE TRY TO IMPROVE FALSE SHARING?


    #pragma omp parallel shared(rounds, stop, mat) num_threads(processors)
    {

        int categoryCount[categories + 1] = {0};
        int max = -1;
        int maxIndex = -1; 
        int change = false;

        while(!stop && rounds < MAXITERATIONS) {
            stop = true;

            //NEED TO STUDY WHY THE OMP PARALLEL REGION IS NOT WORKING OUTSIDE THE WHILE LOOP (IT BLOCKS AT A CERTAIN POINT)
            //Dividing iteration space on columns
            #pragma omp for
            for (int j = 0; j < mat.ncol; ++j) {

                    for (int i = 0; i < mat.nrow; ++i) {
                
                        int label = nodelabels[i];

                        //If the value is in list 1, we count it
                        //If the value is in list is 0, we do not count it
                        categoryCount[label] = categoryCount[label] + mat.incidenceMatrix[i][j];
                        
                        if(label != 0 && categoryCount[label] > max) {
                            max = categoryCount[label];
                            maxIndex = label;
                        }
                    }

                    // Check if the label has changed
                    if(edgelabels[j] != maxIndex) {
                        change = true;
                        edgelabels[j] = maxIndex;
                    }

                    for (int k = 0; k <= categories; ++k) {
                        categoryCount[k] = 0;
                    }
        
                    max = -1;
                    maxIndex = -1; 

                    std::cout << "EDGE " << j << " label:" << edgelabels[j] << "\n";

            }

            //Dividing iteration space on rows
            #pragma omp for
            for (int i = 0; i < mat.nrow; ++i) {

                    for (int j = 0; j < mat.ncol; ++j) {
                
                        int label = edgelabels[j];

                        //If the value is in list 1, we count it
                        //If the value is in list is 0, we do not count it
                        categoryCount[label] = categoryCount[label] + mat.incidenceMatrix[i][j];

                        if(label != 0 && categoryCount[label] > max) {
                            max = categoryCount[label];
                            maxIndex = label;
                        }
                    }

                    // Check if the label has changed
                    if(nodelabels[i] != maxIndex) {
                        change = true;
                        nodelabels[i] = maxIndex;      
                    }

                    for (int k = 0; k <= categories; ++k) {
                        categoryCount[i] = 0;
                    }
        
                    max = -1;
                    maxIndex = -1; 

                    //std::cout << "NODE " << i << " label:" << nodelabels[i] << "\n";
            }
        

            if(change) {
            stop = !change;
            }


        #pragma omp single
        {
            std::cout << "rounds: " << rounds << "\n";
            std::cout << "stop: " << stop << "\n";
    
            rounds++;
        }
    }
        
    }


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
    for (int i = 0; i < mat.nrow; ++i) {
        free(mat.incidenceMatrix[i]);
    }

    free(mat.incidenceMatrix);
    free(nodelabels);
    free(edgelabels);
    return 0;
}
#include "SIMPLEMAT.h"
#include <iostream>
#include <fstream>
#include <omp.h>


bool* extractIncidenceMatrix(std::ifstream* hIncidenceFilePoint, std::ifstream* nLabelsFilePoint, long long int* rows, long long int* cols){
    bool* incidenceMatrix = nullptr;

    std::string lineString = "";

    std::ifstream& hIncidenceFile = *hIncidenceFilePoint;
    std::ifstream& nLabelsFile = *nLabelsFilePoint;

    hIncidenceFile.clear();
    hIncidenceFile.seekg(0, hIncidenceFile.beg);

    nLabelsFile.clear();
    nLabelsFile.seekg(0, nLabelsFile.beg);

    long long int index = 0;  

    //counting number of columns FROM DATASET
    while(std::getline (hIncidenceFile, lineString)) {
        index++;
    }

    *cols = index;

    index = 0;

    //counting number of rows FROM DATASET
    while(std::getline (nLabelsFile, lineString)) {
        index++;
    }

    *rows = index;

    incidenceMatrix = (bool*) malloc((*rows) * (*cols) * sizeof(bool));

    // Reset the file stream to the beginning
    hIncidenceFile.clear();
    hIncidenceFile.seekg(0, hIncidenceFile.beg);

    nLabelsFile.clear();
    nLabelsFile.seekg(0, nLabelsFile.beg);

    index = 0;

    std::string s = "";

    // Use a while loop together with the getline() function to read the file line by line
    //READING THE INCIDENCE MATRIX
    while (std::getline (hIncidenceFile, lineString) && index < *cols) {
    
        //std::cout << "index "<< index << ": ";

        for(char c : lineString) {
            if(isdigit(c)) {
                s = s + c;
            }else{
                if(!s.empty()) {
                    //std::cout << s << " ";
                    int row = std::stoi(s);
                    
                    //std::cout << "ROW Value: " << row << "\n";
                    //std::cout << "Value: " << (row-1) * (*cols) << "\n";
                    long long int offset = (((row - 1) * (*cols)) + index);
                    incidenceMatrix[offset] = true;
                    s = "";
                }
            }
        }

        //NEEDED FOR THE LAST LINE
        if(hIncidenceFile.peek()==EOF && !s.empty()) {
            //std::cout << s << " ";
            long long int row = std::stoi(s);
            long long int offset = (((row - 1) * (*cols)) + index);
            incidenceMatrix[offset] = true;
        }

        s = "";

        index++;
    }

    return incidenceMatrix;
}


int* extractNodeLabels(std::ifstream* nLabelsFilePoint, long long int rows){
   // printf("nodelabels size: %d\n", sizeof(int) * rows);
    int* nodelabels = (int*) malloc(sizeof(int) * rows);
    std::string lineString;
    std::ifstream& nLabelsFile = *nLabelsFilePoint;

    nLabelsFile.clear();
    nLabelsFile.seekg(0, nLabelsFile.beg);

    long long int index = 0;  

    //READING THE NODE LABELS
    while (std::getline (nLabelsFile, lineString)) {
        std::string s = "";
        for(char c : lineString) {
            if(isdigit(c)) {
                s = s + c;
            }else{
                if(!s.empty()) {
                    //std::cout << s << " ";
                    nodelabels[index] = std::stoi(s);

                    break;
                }
            }
        }


        if(nLabelsFile.peek()==EOF && !s.empty()) {
            //std::cout << s << " ";
            long long int row = std::stoi(s);
            nodelabels[index] = row;
        }else{
            try {
            long long int val = std::stoi(s);
            nodelabels[index] = val;
        } catch (const std::invalid_argument& e) {

        } catch (const std::out_of_range& e) {

        }
        }

        index++;
    }
    nLabelsFile.close();

    return nodelabels;
}





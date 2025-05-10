/*#include "SIMPLEMAT.h"
#include <iostream>


//(if nrow == 0) This function counts the number of edges in the incidence matrix for a given node.
//(if ncol == 0) This function counts the number of vertices in the incidence matrix for a given hyperedge.
int countingBelongings(int* list, int ncol, int nrow) {

    //Will be assigned to the number of iterations we need to do for counting
    int iterations = 0;

    if(nrow == 0 && ncol == 0) {
        return -1;
    }

    if(ncol == 0) {
        iterations = nrow;
    }else {
        iterations = ncol;
    }

    int count = 0;

    for (int i = 0; i < iterations; ++i) {
        //If the value is 1, we count it
        //If the value is 0, we do not count it
        count = count + list[i];
    }

    return count;
}

//Establishes next category for a given node or hyperedge
/*int aggregatedCategory(int* list, int* labels, int ncol, int nrow, int categories) {
    
    //Will be assigned to the number of iterations we need to do for counting
    int iterations = 0;

    int max = 0;

    if(nrow == 0 && ncol == 0) {
        return -1;
    }

    if(ncol == 0) {
        iterations = nrow;
    }else {
        iterations = ncol;
    }

    int categoryCount[categories] = {0};
    int max = -1;
    int maxIndex = -1;

    for (int i = 0; i < iterations; ++i) {

        int label = labels[i];

        //If the value is in list 1, we count it
        //If the value is in list is 0, we do not count it
        categoryCount[label] = categoryCount[label] + list[i];

        if(categoryCount[label] > max) {
            max = categoryCount[label];
            maxIndex = i;
        }
    }

    return categoryCount[maxIndex];
}*/
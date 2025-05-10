struct SimpleMat{
    long long int nrow;
    
    long long int ncol;
    
    bool** incidenceMatrix;

};

struct SimpleMatGPU{
    long long int nrow;
    
    long long int ncol;
    
    bool* incidenceMatrix;

};

// SimpleMat is a structure that represents a simple matrix with integer values (SimpleMat[i][j] = 0 if node i is contained in hyperedge j, 1 otherwhise).


//int countingBelongings(int* list, int ncol, int nrow);
//int aggregatedCategory(int* list, int* labels, int ncol, int nrow, int categories);
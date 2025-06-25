#include <stdint.h>

struct SimpleMat{
    long long int nrow;
    
    long long int ncol;
    
    bool** incidenceMatrix;

};

struct SimpleMatGPU{
    long long int nrow;
    
    long long int ncol;
    
    uint32_t* incidenceMatrix;

};
// SimpleMat is a structure that represents a simple matrix with integer values (SimpleMat[(i * cols) + j] = 1 if node i is contained in hyperedge j, 0 otherwhise).

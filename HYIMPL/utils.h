#include <fstream>

bool* extractIncidenceMatrix(std::ifstream* hIncidenceFile, std::ifstream* nLabelsFile, long long int* rows, long long int* cols);
int* extractNodeLabels(std::ifstream* nLabelsFile, long long int rows);
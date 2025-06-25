#include <iostream>
#include <omp.h>

using namespace std;
//#pragma omp requires unified_shared_memory

int main() {
    int sum = 0;
    #pragma omp target map(tofrom: sum) device(0)
    #pragma omp teams distribute parallel for reduction(+: sum)
    for (int i = 0; i < 96; ++i) {
        printf("%d\n", omp_get_team_num());
        //printf("%d\n", omp_get_device_num());
    }
    cout << "Sum: " << sum << endl;
    return 0;
}
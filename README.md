# CIHGLP

The purpose of this project is to try to exploit the potentials offered by GPU-based computations, through the OpenMP programming model, in order to create more efficient and scalable solutions for a specific Hypergraph Label Propagation algorithm
This algorithm requires an hypergraph with labeled nodes as input, and returns a fully labeled hypergraph as output (hyperedge also will have a label)

The methodology is very simple. The final solution can take multiple rounds. In each round there are two phases. First, nodes will propagate their label to their belonging hyperedges. Second, hyperdeges will do the same with the nodes contained in each of them.
The label update rule states that each node/hyperedge will update its label to the most frequent one among the received labels. This rule is true for each of the two phases.

In order to execute this algorithm on your input hypergraph you will require two things:
- An appropriate compiler that supports OpenMP directives
- A GPU vendor driver that allows the work offloading from the CPU

Once you have all the needed components, you can execute two versions of this algorithm (both presents in this repo for comparison purposes):
- Simple one
- And a more efficient but also more memory expensive one that uses a transposed version of the matrix that represents the original hypergraph

In the second one there are three optimizations:
- O2 compiler flag
- Usage of a transpose matrix
- Tiling solution for the transposition operation

The first optimization gives a great performance boost with low to no effort.
The purpose of the last two optimization instead, is to try to increase the coalescence or the cache locality, based on which phase you want to allocate the usage of the transpose matrix


In order to compile each of the two solutions you need to navigate toward the *GPU* folder and, then, assuming you are using clang++ as OpenMP compiler and an NVIDIA GPU, you can use one of these two commands for the first and the second version:

clang++ -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target=nvptx64-nvidia-cuda -march=<your_CUDA_capability_version> ../HYIMPL/utils.cpp ./SIMPLEGPUIMPL.cpp -o <binary_name>

clang++ -O2 -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target=nvptx64-nvidia-cuda -march=sm_86 ../HYIMPL/utils.cpp ./TRANSPOSE_GPUIMPL.cpp -o <binary_name>

In order to execute one of the two binaries you must follow this schema:

./<binary_name> <file_containing_the_incidence_matrix> <file_containing_node_labels>

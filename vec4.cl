__kernel void vector_add(__global const int4 *A, 
                         __global const int4 *B,  
                         __global int4 *C) {
    // Get the index of the current element to be processed
    int i = get_global_id(0);

    // Do the operation
    C[i] = A[i] + B[i];
}

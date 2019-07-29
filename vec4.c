#include <stdio.h>
#include <stdlib.h>

// Suppress deprecation warning for clCreateCommandQueue.
// Successor is clCreateCommandQueueWithProperties, but
// we don't know how to use that.
#define CL_TARGET_OPENCL_VERSION 120


#include <CL/cl.h>

#include <CL/cl_platform.h>

#define MAX_SOURCE_SIZE (0x100000)
#define LIST_SIZE 16777216


// This might be an example of vectorized code in OpenCL -- the design
// in intent is to force both parallelization and vectorization on the
// CPU, without mucking about with intrinsics, or otherwise
// encounntering the kind of fragility that mainstream compilers seem
// to run into all the time that breaks vectorization.  The bulk of
// this is taken from this site:
//
// https://community.khronos.org/t/how-to-avoid-double-allocation-on-cpu/3566/5
//
// The tricks are:
// - Align the buffers!  OpenCL will copy misaligned buffers.
// - When allocating the buffers, use the CL_MEM_USE_HOST_PTR flag,
//   and be sure to supply the penultimate argument.
// - There's no need to write the buffers.
// - To "retrieve" data, you apparently have to do this
//      clEnqueueMapBuffer step.  It's unclear why this is
//      the case, but answers are wrong if you don't.
//      Example code also unmaps it, which is polite.

// As of July 26, 2019:
// I haven't actually verified that this successfully uses CPU vector
// instructions, or tested what fraction of the CPU memory bandwidth
// we're using, or verified that it spreads the task over all the coes. 


int main() {

    int i, j;
    char* value;
    size_t valueSize;
    cl_uint platformCount;
    cl_platform_id* platforms;
    cl_uint deviceCount;
    cl_device_id* devices;
    cl_device_id saved_device;
    cl_uint maxComputeUnits;
    cl_ulong maxBufferSize;
    cl_int ret;

    // get all platforms
    clGetPlatformIDs(0, NULL, &platformCount);
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
    clGetPlatformIDs(platformCount, platforms, NULL);

    for (i = 0; i < platformCount; i++) {

        // get all devices
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
        devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);
	clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL, &valueSize);
	value = (char *)malloc(valueSize);
	clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, valueSize, value, NULL);
	printf("Platform %d: %s\n",i+1,value);
	free(value);
	
        // for each device print critical attributes
        for (j = 0; j < deviceCount; j++) {

            // print device name
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
            value = (char*) malloc(valueSize);
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
            printf("%d. Device: %s\n", j+1, value);
            free(value);

            // print hardware device version
            clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, 0, NULL, &valueSize);
            value = (char*) malloc(valueSize);
            clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, valueSize, value, NULL);
            printf(" %d.%d Hardware version: %s\n", j+1, 1, value);
            free(value);

            // print software driver version
            clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, 0, NULL, &valueSize);
            value = (char*) malloc(valueSize);
            clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, valueSize, value, NULL);
            printf(" %d.%d Software version: %s\n", j+1, 2, value);
            free(value);

            // print c version supported by compiler for device
            clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &valueSize);
            value = (char*) malloc(valueSize);
            clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
            printf(" %d.%d OpenCL C version: %s\n", j+1, 3, value);
            free(value);

            // print parallel compute units
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS,
                    sizeof(maxComputeUnits), &maxComputeUnits, NULL);
            printf(" %d.%d Parallel compute units: %d\n", j+1, 4, maxComputeUnits);

	    // print max buffer size.
	    clGetDeviceInfo(devices[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE,
			    sizeof(maxBufferSize), &maxBufferSize,NULL);
	    printf(" %d %d Maximum buffer size (bytes):  %ld\n", j+1, 5, maxBufferSize);
	    
	    if ((i==1)&&(j==0)) {
	      saved_device = devices[j];
	      printf("Using this device.\n");
	    }
        }

        free(devices);

    }

    free(platforms);

    // We've picked out the CPU device.  Do the OpenCL basics.
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("vec4.cl","r");
    if(!fp) {
      fprintf(stderr,"Failed to load the kernel.\n");
      exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str,1,MAX_SOURCE_SIZE,fp);
    fclose(fp);
    
    cl_context context = clCreateContext(NULL,1,&saved_device,NULL,NULL,&ret);
    cl_command_queue cq = clCreateCommandQueue(context, saved_device, 0, &ret);
    
    cl_program pgm = clCreateProgramWithSource(context, 1,
					       (const char **)&source_str,
					       (const size_t *)&source_size,
					       &ret);

    printf("Building the program.\n");
    ret = clBuildProgram(pgm,1,&saved_device,NULL,NULL,NULL);
    printf("Ret after building the program: %d.\n",ret);
    
    cl_kernel krnl = clCreateKernel(pgm,"vector_add",&ret);
    printf("Ret after kernel creation: %d.\n",ret);

    /* if (ret == CL_BUILD_PROGRAM_FAILURE) { */
    /*   // Determine the size of the log */
    /*   size_t log_size; */
    /*   clGetProgramBuildInfo(pgm, saved_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size); */

    /*   // Allocate memory for the log */
    /*   char *log = (char *) malloc(log_size); */
      
    /*   // Get the log */
    /*   clGetProgramBuildInfo(pgm, saved_device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL); */

    /*   // Print the log */
    /*   printf("%s\n", log); */
    /* } */

    // Initialize local data. "aligned_alloc" is in stdlib.h.
    cl_int4 *A = (cl_int4 *)aligned_alloc(128,sizeof(cl_int4)*LIST_SIZE);
    cl_int4 *B = (cl_int4 *)aligned_alloc(128,sizeof(cl_int4)*LIST_SIZE);
    cl_int4 *C = (cl_int4 *)aligned_alloc(128,sizeof(cl_int4)*LIST_SIZE);
    for(int i=0;i<LIST_SIZE;++i) {
      A[i].s0 = 4*i; A[i].s1 = 4*i+1; A[i].s2 = 4*i+2; A[i].s3 = 4*i + 3;
      B[i].s0 = (LIST_SIZE-i)*4; B[i].s1 = (LIST_SIZE-i)*4-1;
      B[i].s2 = (LIST_SIZE-i)*4-2; B[i].s3 = (LIST_SIZE-i)*4-3;;
      C[i].s0 = 0; C[i].s1 = 0; C[i].s2 = 0; C[i].s3 = 0;
    }

    // Allocate device data.  
    cl_mem device_a = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
				     LIST_SIZE*sizeof(cl_int4),A,&ret);
    cl_mem device_b = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
				     LIST_SIZE*sizeof(cl_int4),B,&ret);
    cl_mem device_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
				     LIST_SIZE*sizeof(cl_int4),C,&ret);
    printf("Ret after creating C buffer: %d.\n",ret);
    
    // In zero-copy scheme, skip this.
    // ret = clEnqueueWriteBuffer(cq, device_a, CL_TRUE,
    //			       LIST_SIZE*sizeof(cl_int4),A,0,NULL,NULL);
    // ret = clEnqueueWriteBuffer(cq, device_b, CL_TRUE, 0,
    //			       LIST_SIZE*sizeof(cl_int4),B,0,NULL,NULL);
    //
    // printf("Ret after allocations: %d.\n", ret);

    // Set the kernel args.
    ret = clSetKernelArg(krnl, 0, sizeof(cl_mem), (void *)&device_a);
    ret = clSetKernelArg(krnl, 1, sizeof(cl_mem), (void *)&device_b);
    ret = clSetKernelArg(krnl, 2, sizeof(cl_mem), (void *)&device_c);

    printf("Ret after kernel args: %d.\n", ret);
    
    // These are like array size and block size in CUDA.
    size_t global_item_size = LIST_SIZE;
    size_t local_item_size = 64; 
    ret = clEnqueueNDRangeKernel(cq, krnl, 1, NULL,
				 &global_item_size, &local_item_size,
				 0, NULL, NULL);

    printf("Ret after kernel call: %d.\n",ret);
    
    // Retrieve C. Not in zero-copy.
    // ret = clEnqueueReadBuffer(cq, device_c, CL_TRUE, 0,
    //				  LIST_SIZE*sizeof(cl_int4), C, 0, NULL, NULL);
    // printf("Ret after reading the buffer: %d.\n", ret);

    void *ptr = clEnqueueMapBuffer(cq, device_c, CL_TRUE, CL_MAP_READ,
       				   LIST_SIZE*sizeof(cl_int4), 0, 0, NULL, NULL, &ret);
    printf("Ret after MapBuffer: %d.\n",ret);

    // Apparently one has to examine the contents of the C buffer before unmapping it.
    // It's not entirely clear to me why one needs to unmap it at all.
    
    for(int i=0;i<LIST_SIZE;i++) {
      if ((i<3) || (i>(LIST_SIZE-4))) {
	printf("%d + %d = %d\n", A[i].s0,B[i].s0,C[i].s0);
	printf("%d + %d = %d\n", A[i].s1,B[i].s1,C[i].s1);
	printf("%d + %d = %d\n", A[i].s2,B[i].s2,C[i].s2);
	printf("%d + %d = %d\n", A[i].s3,B[i].s3,C[i].s3);
      }
    }
    
    // ret = clEnqueueUnmapMemObject(cq, device_c, ptr, 0, NULL, NULL);
    // printf("Ret after unmapping: %d.\n", ret);
    
    return 0;
}

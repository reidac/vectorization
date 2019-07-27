#include <stdlib.h>
#include <stdio.h>

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)
#define LIST_SIZE 1024

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

	    if ((i==1)&&(j==0)) {
	      saved_device = devices[j];
	    }
        }

        free(devices);

    }

    free(platforms);

    // We've picked out the CPU device.  Do the OpenCL basics.
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("vec2.cl","r");
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
    ret = clBuildProgram(pgm,1,&saved_device,NULL,NULL,NULL);
    cl_kernel krnl = clCreateKernel(pgm,"vector_add",&ret);
    
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

    // Initialize local data.
    int *A = (int *)malloc(sizeof(int)*LIST_SIZE);
    int *B = (int *)malloc(sizeof(int)*LIST_SIZE);
    int *C = (int *)malloc(sizeof(int)*LIST_SIZE);
    for(int i=0;i<LIST_SIZE;++i) {
      A[i]=i;
      B[i]=LIST_SIZE-i;
      C[i]=0;
    }

    // Allocate device data.  
    cl_mem device_a = clCreateBuffer(context, CL_MEM_READ_ONLY,
				     LIST_SIZE*sizeof(int),NULL,&ret);
    cl_mem device_b = clCreateBuffer(context, CL_MEM_READ_ONLY,
				     LIST_SIZE*sizeof(int),NULL,&ret);
    cl_mem device_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
				     LIST_SIZE*sizeof(int),NULL,&ret);

    ret = clEnqueueWriteBuffer(cq, device_a, CL_TRUE, 0,
			       LIST_SIZE*sizeof(int),A,0,NULL,NULL);
    ret = clEnqueueWriteBuffer(cq, device_b, CL_TRUE, 0,
			       LIST_SIZE*sizeof(int),B,0,NULL,NULL);

    printf("Ret after allocations: %d.\n", ret);

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
    
    // Retrieve C.
    ret = clEnqueueReadBuffer(cq, device_c, CL_TRUE, 0,
			      LIST_SIZE*sizeof(int), C, 0, NULL, NULL);

    printf("Ret after reading the buffer: %d.\n", ret);
    
    for(int i=0;i<LIST_SIZE;i++)
      printf("%d + %d = %d\n", A[i],B[i],C[i]);

    return 0;
}

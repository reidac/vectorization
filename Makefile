#
#
opencl_kernel_targets = vec2 vec4
opencl_targets = devices

$(opencl_kernel_targets): %: %.c %.cl
	gcc $@.c -o $@ -lOpenCL

$(opencl_targets): %: %.c
	gcc $@.c -o $@ -lOpenCL

clean:
	rm -vf $(opencl_kernel_targets) $(opencl_targets)

#
#
opencl_kernel_targets = vec2 vec4
opencl_targets = devices
intrinsic_targets = intrinsic

$(opencl_kernel_targets): %: %.c %.cl
	gcc $@.c -o $@ -lOpenCL

$(opencl_targets): %: %.c
	gcc $@.c -o $@ -lOpenCL

$(intrinsic_targets): %: %.c
	gcc $@.c -o $@

clean:
	rm -vf $(opencl_kernel_targets) $(opencl_targets) $(intrinsic_targets)


## Goals

The basic goal is to get better performance out of CPU code
by learning how to reliably use vectorization. The initial choice
of OpenCL is motived by its use in the loo.py project, which 
was the initial motivation for some of this, although the 
approach here is to understand it in lower-level C code.

## OpenCL

Started with an OpenCL vector-addition [example][1].
There's no corresponding code in this repo, but the
"vec2.c" and "vec4.c" codes are derived pretty closely
from this example.

Then looked into how to query the [device][2] capabilites in OpenCL.
This is the source of the "devices.c" code in this repo, lightly
edited to retrieve a few extra pieces of information.

The "vec2.c" code is specialized to select the CPU device on 
my desktop, and includes code to diagnose kernel build
failures, taken from [here][3]

The "vec4.c" code uses explicit OpenCL vector types,
described [here][4].

Since OpenCL is intended for both CPU and GPU operations, it 
includes a step where data is copied to the device where the
kernel is executed. In the CPU case, this is stupid, the 
"host" and the "device" have the same memory, and if you're
interested in performance, an unnecessary copy is an obvious
problem. It turns out there is a [solution][5], involving
creating the "device" buffers with the `CL_MEM_USE_HOST_POINTER`
flag, and then using `clEnqueueMapBuffer` to read the output
from the destination array.  This isn't entirely trivial to 
use -- the buffer mapping process might still make a copy of
the array if the initially-allocated array is not correctly
aligned in memory for the vector SIMD instructions.  This
can be prevented by using `aligned_alloc` to allocate the
initial "host" arrays.

General OpenCL docs are [here][6].


## Intrinsics

The OpenCL scheme seems to work, but it's very complcaited.
In the [double-allocation][5] discussion, many of the responders 
expressed the opinion that OpenCL is the wrong tool for CPU
vectorization, it's far simpler to just do it directly.
My limited experience with this is that it's very fussy, and
the compiler will very often just refuse to vectorize some
code, for a variety of reasons, maybe because it can't be sure
that elements are being accessed sequentially, or it's 
confounded by some other context that shouldn't matter.

But, possibly part of this problem is also the alignment

So the next step in _this_ scheme is to give that a go.

There's an [older][7] reference to work from, and a reminder
to specify the [architecture][8] to the compiler, otherwise
it won't even attempt to access the SIMD instructions.

Also, [some rando][9] says you can measure the memory bandwidth
used with cachegrind.  Seems useful.

[1]: https://www.eriksmistad.no/getting-started-with-opencl-and-gpu-computing/
[2]: https://gist.github.com/courtneyfaulkner/7919509  
[3]: https://stackoverflow.com/questions/9464190/error-code-11-what-are-all-possible-reasons-of-getting-error-cl-build-prog
[4]: https://www.fz-juelich.de/SharedDocs/Downloads/IAS/JSC/EN/slides/opencl/opencl-04-vector.pdf
[5]: https://community.khronos.org/t/how-to-avoid-double-allocation-on-cpu/3566/3
[6]: https://github.com/KhronosGroup/OpenCL-Docs
[7]: https://ds9a.nl/gcc-simd/index.html
[8]: https://stackoverflow.com/questions/43613577/compile-c-code-with-avx2-avx512-intrinsics-on-avx
[9]: https://superuser.com/questions/458133/how-to-measure-memory-bandwidth-usage

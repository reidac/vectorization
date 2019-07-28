#include <stdio.h>
#include <stdlib.h>

#include <x86intrin.h>

#define LIST_SIZE 16777216

typedef __m128i elem;
  
int main() {
  elem *a = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);
  elem *b = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);
  elem *c = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);

  for(int i=0;i<LIST_SIZE;++i) {
    a[i] = _mm_set_epi32(i*4,i*4+1,i*4+2,i*4+3);
    b[i] = _mm_set_epi32((LIST_SIZE-i)*4,(LIST_SIZE-i)*4-1,
			 (LIST_SIZE-i)*4-2,(LIST_SIZE-i)*4-3);
  }

  // Vector loop, maybe.
#pragma omp parallel
#pragma omp for schedule(static)
  for(int i=0;i<LIST_SIZE;i++) {
    c[i] = a[i] + b[i];
  }

  for(int i=0;i<LIST_SIZE;i++) {
    if ((i<3) || (i>(LIST_SIZE-4))) {
	printf("%d + %d = %d\n",
	       _mm_extract_epi32(a[i],3),
	       _mm_extract_epi32(b[i],3),
	       _mm_extract_epi32(c[i],3));
	printf("%d + %d = %d\n",
	       _mm_extract_epi32(a[i],2),
	       _mm_extract_epi32(b[i],2),
	       _mm_extract_epi32(c[i],2));
	printf("%d + %d = %d\n",
	       _mm_extract_epi32(a[i],1),
	       _mm_extract_epi32(b[i],1),
	       _mm_extract_epi32(c[i],1));
	printf("%d + %d = %d\n",
	       _mm_extract_epi32(a[i],0),
	       _mm_extract_epi32(b[i],0),
	       _mm_extract_epi32(c[i],0));
    }
  }
  return 0;
}

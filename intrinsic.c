#include <stdio.h>
#include <stdlib.h>

#include <x86intrin.h>

#define LIST_SIZE 16777216

typedef union {
  __m128i v;
  int vvec[4];
} elem;
  
int main() {
  elem *a = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);
  elem *b = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);
  elem *c = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);

  for(int i=0;i<LIST_SIZE;++i) {
    a[i].vvec[0] = i*4; a[i].vvec[1]=i*4+1;
    a[i].vvec[2]=i*4+2; a[i].vvec[3]=i*4+3;
    b[i].vvec[0]=(LIST_SIZE-i)*4; b[i].vvec[1]=(LIST_SIZE-i)*4-1;
    b[i].vvec[2]=(LIST_SIZE-i)*4-2; b[i].vvec[3]=(LIST_SIZE-i)*4-3;
  }

  // Vector loop, maybe.
#pragma omp parallel
#pragma omp for schedule(static)
  for(int i=0;i<LIST_SIZE;i++) {
    c[i].v = a[i].v + b[i].v;
  }

  for(int i=0;i<LIST_SIZE;i++) {
    if ((i<3) || (i>(LIST_SIZE-4))) {
      for(int j=0;j<4;j++) {
	printf("%d + %d = %d\n",a[i].vvec[j],b[i].vvec[j],c[i].vvec[j]);
      }
    }
  }
  return 0;
}

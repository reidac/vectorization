#include <stdio.h>
#include <stdlib.h>

#include <x86intrin.h>

#define LIST_SIZE 16777216

typedef union {
  __m128i v;
  int v1,v2,v3,v4;
} elem;
  
int main() {
  elem *a = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);
  elem *b = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);
  elem *c = (elem *)aligned_alloc(128,sizeof(elem)*LIST_SIZE);

  for(int i=0;i<LIST_SIZE;++i) {
    a[i].v1 = i*4; a[i].v2 = i*4+1; a[i].v3 = i*4+2; a[i].v4 = i*4+3;
    b[i].v1 = (LIST_SIZE-i)*4; b[i].v2 = (LIST_SIZE-i)*4-1;
    b[i].v3 = (LIST_SIZE-i)*4-2; b[i].v4 = (LIST_SIZE-i)*4-3;
  }

  // Vector loop, maybe...
  for(int i=0;i<LIST_SIZE;i++) {
    c[i].v = a[i].v + b[i].v;
  }

  for(int i=0;i<LIST_SIZE;i++) {
    if ((i<3) || (i>(LIST_SIZE-4))) {
      printf("%d + %d = %d\n", a[i].v1,b[i].v1,c[i].v1);
      printf("%d + %d = %d\n", a[i].v2,b[i].v2,c[i].v2);
      printf("%d + %d = %d\n", a[i].v3,b[i].v3,c[i].v3);
      printf("%d + %d = %d\n", a[i].v4,b[i].v4,c[i].v4);
    }
  }
  return 0;
}

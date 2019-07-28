#include <stdio.h>
#include <stdlib.h>

#include <x86intrin.h>

#define LIST_SIZE 16777216

// TODO: Actually use some intrinsics.
int main() {
  int *a = (int *)aligned_alloc(128,sizeof(int)*LIST_SIZE);
  int *b = (int *)aligned_alloc(128,sizeof(int)*LIST_SIZE);
  int *c = (int *)aligned_alloc(128,sizeof(int)*LIST_SIZE);

  for(int i=0;i<LIST_SIZE;++i) {
    a[i] = i;
    b[i] = LIST_SIZE-i;
  }
      
  return 0;
}

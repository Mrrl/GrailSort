GrailSort
=========

![Lang](https://img.shields.io/badge/language-c++-green.svg)
![C++ Standard](https://img.shields.io/badge/c++-14-yellow.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)


Stable In-place sorting in O(n*log(n)) worst time.

This algorithm is based on ideas from the article

   B-C. Huang and M. A. Langston, Fast Stable Merging and Sorting in Constant Extra Space (1989-1992)
   (http://comjnl.oxfordjournals.org/content/35/6/643.full.pdf)

To use sorting define type SORT_TYPE and comparison function SORT_CMP and include "GrailSort.h" file.
Code in C-compatible.

Or include "grailsort.hpp" file that provides four sorting functions that are consistent with the std::sort interface (for c++14 or later).

There is a similar project at https://github.com/BonzaiThePenguin/WikiSort . A different algorithm based on the same idea is implemented there.

Example
=======

C Language:

```c
#include<stdio.h>

// define SORT_TYPE and SORT_CMP.
#define SORT_TYPE int

int SORT_CMP(int *a, int *b){
    return (*a > *b) - (*a < *b);
}

#include "GrailSort.h"

int arr[]={8,5,42,5,5,11,6,-9,36,326,346};
int main(){
    int sz=sizeof(arr)/sizeof(int);

    GrailSort(arr, sz); // Sort In-Place.
    // GrailSortWithBuffer(arr, sz); // Sort With Constant Buffer (512 items).
    // GrailSortWithDynBuffer(arr, sz); // Sort With Dynamic Buffer.
    // RecStableSort(arr, sz); // Classic Rotate In-Place Mergesort.

    for(int i=0;i<sz;i++) printf("%d ", a[i]);
    return 0;
}
```


C++ Language:

```cpp
#include<iostream>
#include<vector>
#include "grailsort.hpp"

using namespace std;
vector<int> a{8,5,42,5,5,11,6,-9,36,326,346};
int main(){
    grailsort::grail_sort(a.begin(), a.end()); // Sort In-Place.
    // grailsort::grail_sort_buffer(a.begin(), a.end()); // Sort With Constant Buffer (512 items).
    // grailsort::grail_sort_dyn_buffer(a.begin(), a.end()); // Sort With Dynamic Buffer.
    // grailsort::rec_stable_sort(a.begin(), a.end()); // Classic Rotate In-Place Mergesort.
    for(auto p:a) cout<<p<<" ";
    return 0;
}
```


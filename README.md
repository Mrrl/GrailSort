GrailSort
=========

Stable In-place sorting in O(n*log(n)) worst time.

This algorithm is based on ideas from the article

   B-C. Huang and M. A. Langston, Fast Stable Merging and Sorting in Constant Extra Space (1989-1992)
   (http://comjnl.oxfordjournals.org/content/35/6/643.full.pdf)

To use sorting define type SORT_TYPE and comparison function SORT_CMP and include "GrailSort.h" file.
Code in C-compatible.

Algorithms with some external memory are also implemented.

GrailSortWithBuffer uses fixed buffer of 512 elements (allocated in the stack).

GrailSortWithDynBuffer uses dynamiclly allocated buffer less than 2*sqrt(N) elements (allocated in heap).

Below there is a table of times for softing of arrays of 8-byte structures. Three versions of algorithm are compared with std::stable_sort().
Array of 1, 10 and 100 million structures were generated with two different numbers of different keys. First (the less) is the worst case for the algorithm - when number of keys is not enough for two internal buffers.
Second number is enough for fast version of algorithms.

Time is in milliseconds (compiled in MSVS 2013). Second number is time relative to result of std::stable_sort on the same data.

    size             stable_sort  no buffer  static buffer   dynamic buffer 
    1M, 1023 keys        200     218 (1.09)     224 (1.12)      203 (1.02)
    1M, 2047 keys        195     164 (0.84)     162 (0.83)      157 (0.81)
    10M, 4095 keys      2220    2765 (1.24)    2790 (1.25)     2784 (1.25)
    10M, 8191 keys      2233    1964 (0.88)    1863 (0.83)     1718 (0.77)
    100M, 16383 keys   24750   33652 (1.36)   33708 (1.36)    33338 (1.35)           
    100M, 32767 keys   25230   22852 (0.90)   21331 (0.84)    19442 (0.77)

There is a similar project at https://github.com/BonzaiThePenguin/WikiSort . A different algorithm based on the same idea is implemented there.

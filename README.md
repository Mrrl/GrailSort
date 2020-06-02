GrailSort
=========
Abstract by MusicTheorist:

Grailsort is an *in-place, stable, worst-case O(n log n) time* variant of Mergesort and implementation of 'Block Merge Sort'.

The algorithm starts by attempting to collect (2 * sqrt(n)) unique elements at the beginning of the array. One-half of this collection serves as an internal buffer that traverses through the list, merging subarrays of increasing length each iteration. When the length of these subarrays no longer fits inside the buffer, the subarrays are then broken down into blocks of size sqrt(n) and swapped to approximate places, using the other half of said unique elements as 'keys' to tag potential swaps of equal elements. Afterwards, the internal buffer is left to finish a 'local merge' of said blocks together. This entire process continues as a bottom-up merge until all subarrays are combined, and finally the collection of unique elements are merged back into the rest of the array without a buffer.

Choosing the length O(sqrt(n)) for blocks and key-buffers allows for assistance from simple O(n^2) algorithms, including Insertion and Selection Sort, whose time complexities equate to O(n^2 * sqrt(n)), simplifying down to an optimal O(n) time. 'Situationally optimal' complexities are of particular importance when an array to be sorted does not have (2 * sqrt(n)) unique elements. Without enough 'keys', Grailsort may not have either a large enough buffer for 'local merges' or enough 'block swapping' tags to guarantee stability, or both. In these cases, normally suboptimal in-place merge sorts based on rotations are instead used to still achieve a stable and O(n log n) worst-case sort, without using any extra space.

=========

Original README by Andrey Astrelin:

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

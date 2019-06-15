#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <algorithm>

typedef struct{
	int key;
	int val;
} key_val_pair;

long long comps;

inline int compare(const key_val_pair* a, const key_val_pair* b) {
	comps++;

	if(a->key < b->key) return (-1);
	else if(a->key > b->key) return (1);
	else return (0);
}

#define GRAIL_SORT_TYPE key_val_pair
#define GRAIL_SORT_COMPARE compare

#include "GrailSort.h"

/******** Tests *********/

int seed = 100000001;
int rand(int k) {
	seed = seed * 1234565 + 1;
	return ((int) (((long long) (seed & 0x7fffffff) * k) >> 31));
}


void generate_array(GRAIL_SORT_TYPE* arr, int* key_center, int length, int key_count) {
	for(int i = 0; i < key_count; i++) key_center[i] = 0;

	for(int i = 0; i < length; i++) {
		if(key_count) {
			int key = rand(key_count);
			arr[i].key = key;
			arr[i].val = key_center[key]++;
		} 
		else {
			arr[i].key = rand(1000000000);
			arr[i].val = 0;
		}
	}
}

bool test_array(GRAIL_SORT_TYPE* arr, int length) {
	for(int i = 1; i < length; i++) {
		int dk = GRAIL_SORT_COMPARE(arr + (i - 1), arr + i);

		if(dk > 0) return (false);
		else if(dk == 0 && arr[i - 1].val > arr[i].val) return (false);
	}
	return (true);
}

void print_array(char* s, GRAIL_SORT_TYPE* arr, int length) {
	printf("%s:", s);
	for(int i = 0; i < length; i++) printf(" %d:%d", arr[i].key, arr[i].val);
	printf("\n");
}

extern "C" int extern_comp(const void* a, const void* b) {
	return (GRAIL_SORT_COMPARE((const GRAIL_SORT_TYPE*) a, (const GRAIL_SORT_TYPE*) b));
}

void quick_sort_test(GRAIL_SORT_TYPE* arr, int length) {
	qsort(arr, length, sizeof(GRAIL_SORT_TYPE), extern_comp);
}

bool stable_comp(GRAIL_SORT_TYPE a, GRAIL_SORT_TYPE b) {
	return (GRAIL_SORT_COMPARE(&a, &b) < 0);
}

void stable_sort_test(GRAIL_SORT_TYPE* arr, int length) {
	std::stable_sort(arr, arr + length, stable_comp);
}

void rec_stable_sort_test(GRAIL_SORT_TYPE* arr, int length) {
	rec_stable_sort(arr, length);
}

void sort_test(GRAIL_SORT_TYPE* arr, int* key_center, int length, int key_count, bool run_grail) {
	uint64_t nanos;
	struct timespec start, end;

	generate_array(arr, key_center, length, key_count);

	printf("%s N: %d, NK: %d ", run_grail ? "GrailSort:   " : "StableSort:", length, key_count);

	comps = 0;

	clock_gettime(CLOCK_MONOTONIC, &start);

	if(run_grail) {
		grail_sort(arr, length);
		//grail_sort_with_static_buffer(arr, length);
	    //grail_sort_with_dynamic_buffer(arr, length;
	}
	else {
		stable_sort_test(arr, length);
		//rec_stable_sort_test(arr, length);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	nanos = 1e+9 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
	float time_taken_in_ms = nanos / 1e+6;

	printf("Comparisons: %I64d, Real time: %f ms ", comps, time_taken_in_ms);

	bool array_sorted = test_array(arr, length);

	if(array_sorted) {
		printf("Sort was successful\n");
	} else{
		printf("Sort was NOT successful\n");
	}
}

void test_two_sorts(GRAIL_SORT_TYPE* arr, int* key_arr, int length, int key_count) {
	int h = seed;
	sort_test(arr, key_arr, length, key_count, false);

	seed = h;
	sort_test(arr, key_arr, length, key_count, true);
}

int main() {
	int max_length = 100000000;
	int max_key_count = 200000;

	GRAIL_SORT_TYPE* arr = new GRAIL_SORT_TYPE[max_length];
	int* key_arr = new int[max_key_count];

	sort_test(arr, key_arr, max_length, 0, false);
	sort_test(arr, key_arr, max_length, 0, true);

#if 0
	for(int u = 100; u <= max_length; u *= 10) {
		for(int v = 2; v <= u && v <= max_key_count; v *= 2) {
			test_two_sorts(arr, key_arr, u, v - 1);
		}
	}
#else
	test_two_sorts(arr, key_arr, 1000000, 1023);
	test_two_sorts(arr, key_arr, 1000000, 2047);
	test_two_sorts(arr, key_arr, 10000000, 4095);
	test_two_sorts(arr, key_arr, 10000000, 8191);
	test_two_sorts(arr, key_arr, 100000000, 16383);
	test_two_sorts(arr, key_arr, 100000000, 32767);
	test_two_sorts(arr, key_arr, 100000000, 32767);
	test_two_sorts(arr, key_arr, 100000000, 16383);
	test_two_sorts(arr, key_arr, 10000000, 8191);
	test_two_sorts(arr, key_arr, 10000000, 4095);
	test_two_sorts(arr, key_arr, 1000000, 2047);
	test_two_sorts(arr, key_arr, 1000000, 1023);
#endif

	return (0);
}

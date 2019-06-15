/********* Grail sorting *********************************/
/*                                                       */
/* (c) 2013 by Andrey Astrelin                           */
/*                                                       */
/*                                                       */
/* Stable sorting that works in O(N*log(N)) worst time   */
/* and uses O(1) extra memory                            */
/*                                                       */
/* Define SORT_TYPE and SORT_CMP                         */
/* and then call GrailSort() function                    */
/*                                                       */
/* For sorting with fixed external buffer (512 items)    */
/* use GrailSortWithBuffer()                             */
/*                                                       */
/* For sorting w/ dynamic external buffer (sqrt(length)) */
/* use GrailSortWithDynBuffer()                          */
/*                                                       */
/* Also classic in-place merge sort is implemented       */
/* under the name of RecStableSort()                     */
/*                                                       */
/*********************************************************/

#include<memory.h>
#include<malloc.h>
#define GRAIL_STATIC_EXT_BUFFER_LENGTH 512

inline void grail_swap(GRAIL_SORT_TYPE* a, GRAIL_SORT_TYPE* b) {
    GRAIL_SORT_TYPE temp = *a;
    *a = *b;
    *b = temp;
}

inline void grail_multi_swap(GRAIL_SORT_TYPE* a, GRAIL_SORT_TYPE* b, int swaps_left) {
    while((swaps_left--) != 0) grail_swap(a++, b++);
}

// grailShift is called when indices b and a are right next to each other.
// Rather than continuously swapping elements, we can save time by keeping arr[b] in memory,
// shifting everything from b down to a by 1, and then writing arr[b] into index a. This way,
// we save extra write operations and make Grail Sort slightly faster in processes like finding
// keys. Effectively, grailShift is a specialized Insertion Sort.

// Most of the time, grailRotate is doing multiple gapped swaps instead, which is similar
// to Comb Sort. Before, using grailRotate to sort keys would still be similar to
// Insertion Sort, but it would instead be a variant known as Optimized Gnome Sort, a.k.a
// Dr. Hamid Sarbazi-Azad's Stupid Sort remembering the index it came from.

// I was able to generalize this in grailRotate, at the slight cost of a conditional. Nevertheless,
// GrailSort did prove to be slightly faster over the average of a thousand tests.
inline void grail_shift(GRAIL_SORT_TYPE* arr, int writes_left) {
    GRAIL_SORT_TYPE temp = *(arr + 1);

    while(writes_left-- != 0) {
        *(arr + 1) = *arr;
        arr--;
    }
    *(arr + 1) = temp;
}

inline void grail_insert_sort(GRAIL_SORT_TYPE *arr, int len){
    for(int i = 1; i < len; i++) {
        int pos = i - 1;
        GRAIL_SORT_TYPE temp = *(arr + i);

        while(pos >= 0 && GRAIL_SORT_COMPARE(&temp, arr + pos) < 0) {
            *(arr + pos + 1) = *(arr + pos);
            pos--;
        }
        *(arr + pos + 1) = temp;
    }
}

static void grail_rotate(GRAIL_SORT_TYPE* arr, int len_a, int len_b) {
    while(len_a != 0 && len_b != 0) {
        if(len_a <= len_b) {
            if((arr + len_a) - arr == 1) grail_shift(arr, len_a);
            else grail_multi_swap(arr, arr + len_a, len_a);

            arr += len_a;
            len_b -= len_a;
        }
        else {
            if((arr + len_a) - (arr + (len_a - len_b)) == 1) grail_shift(arr + (len_a - len_b), len_b);
            else grail_multi_swap(arr + (len_a - len_b), arr + len_a, len_b);

            len_a -= len_b;
        }
    }
}

//dir bool: false is left, true is right
static int grail_binary_search(GRAIL_SORT_TYPE* arr, int len, GRAIL_SORT_TYPE* key, bool dir) {
    int left = -1, right = len;

    while(left < right - 1) {
        int mid = left + ((right - left) >> 1);

        if(!dir) {
            if(GRAIL_SORT_COMPARE(arr + mid, key) >= 0) right = mid;
            else left = mid;
        }
        else {
            if(GRAIL_SORT_COMPARE(arr + mid, key) > 0) right = mid;
            else left = mid;
        }
    }
    return (right);
}

// cost: 2 * len + nk^2 / 2
static int grail_find_keys(GRAIL_SORT_TYPE* arr, int len, int key_count) {
    int dist = 1;
    int keys_found = 1, first_key = 0;  // first key is always here

    while(dist < len && keys_found < key_count) {
        int loc = grail_binary_search(arr + first_key, keys_found, arr + dist, false);

        if(loc == keys_found || GRAIL_SORT_COMPARE(arr + dist, arr + (first_key + loc)) != 0) {
            grail_rotate(arr + first_key, keys_found, dist - (first_key + keys_found));

            first_key = dist - keys_found;

            grail_rotate(arr + (first_key + loc), keys_found - loc, 1);
            keys_found++;
        }
        dist++;
    }
    grail_rotate(arr, first_key, keys_found);

    return (keys_found);
}

// cost: min(L1, L2)^2 + max(L1, L2)
static void grail_merge_without_buffer(GRAIL_SORT_TYPE* arr, int len1, int len2) {
    if(len1 < len2) {
        while(len1 != 0) {
            //Binary search left
            int loc = grail_binary_search(arr + len1, len2, arr, false);

            if(loc != 0) {
                grail_rotate(arr, len1, loc);

                arr += loc;
                len2 -= loc;
            }

            if(len2 == 0) break;

            do {
                arr++;
                len1--;
            } while(len1 != 0 && GRAIL_SORT_COMPARE(arr, arr + len1) <= 0);
        }
    }
    else {
        while(len2 != 0) {
            //Binary search right
            int loc = grail_binary_search(arr, len1, arr + (len1 + len2 - 1), true);

            if(loc != len1) {
                grail_rotate(arr + loc, len1 - loc, len2);
                len1 = loc;
            }

            if(len1 == 0) break;

            do {
                len2--;
            } while(len2 != 0 && GRAIL_SORT_COMPARE(arr + len1 - 1, arr + len1 + len2 - 1) <= 0);
        }
    }
}

// arr[dist..-1] - buffer, arr[0, left_len - 1] ++ arr[left_len, left_len + right_len - 1]
// -> arr[dist, dist + left_len + right_len - 1]
static void grail_merge_left(GRAIL_SORT_TYPE* arr, int left_len, int right_len, int dist) {
    int left = 0, right = left_len;

    right_len += left_len;

    while(right < right_len) {
        if(left == left_len || GRAIL_SORT_COMPARE(arr + left, arr + right) > 0) {
            grail_swap(arr + (dist++), arr + (right++));
        }
        else grail_swap(arr + (dist++), arr + (left++));
    }
    if(dist != left) grail_multi_swap(arr + dist, arr + left, left_len - left);
}
static void grail_merge_right(GRAIL_SORT_TYPE* arr, int left_len, int right_len, int dist) {
    int merged_pos = left_len + right_len + dist - 1;
    int right = left_len + right_len - 1, left = left_len - 1;

    while(left >= 0) {
        if(right < left_len || GRAIL_SORT_COMPARE(arr + left, arr + right) > 0) {
            grail_swap(arr + (merged_pos--), arr + (left--));
        }
        else grail_swap(arr + (merged_pos--), arr + (right--));
    }
    if(right != merged_pos) {
        while(right >= left_len) grail_swap(arr + (merged_pos--), arr + (right--));
    }
}

static void grail_smart_merge_with_buffer(GRAIL_SORT_TYPE* arr, int* left_over_len, int* left_over_frag, int block_len) {
    int dist = 0 - block_len;
    int left = 0, right = *left_over_len;
    int left_end = right, right_end = right + block_len;
    int frag_type = 1 - *left_over_frag;  // 1 if inverted

    while(left < left_end && right < right_end) {
        if(GRAIL_SORT_COMPARE(arr + left, arr + right) - frag_type < 0) {
            grail_swap(arr + (dist++), arr + (left++));
        }
        else grail_swap(arr + (dist++), arr + (right++));
    }
    if(left < left_end) {
        *left_over_len = left_end - left;

        while(left < left_end) grail_swap(arr + (--left_end), arr + (--right_end));
    }
    else {
        *left_over_len = right_end - right;
        *left_over_frag = frag_type;
    }
}
static void grail_smart_merge_without_buffer(GRAIL_SORT_TYPE* arr, int* left_over_len, int* left_over_frag, int reg_block_len) {
    if(!reg_block_len) return;

    int len1 = *left_over_len, len2 = reg_block_len;
    int frag_type = 1 - *left_over_frag; // 1 if inverted

    if(len1 && GRAIL_SORT_COMPARE(arr + (len1 - 1), arr + len1) - frag_type >= 0) {
        while(len1 != 0) {
            int found_len;

            //Binary search left, else search right
            if(frag_type) found_len = grail_binary_search(arr + len1, len2, arr, false);
            else found_len = grail_binary_search(arr + len1, len2, arr, true);

            if(found_len != 0) {
                grail_rotate(arr, len1, found_len);

                arr += found_len;
                len2 -= found_len;
            }

            if(len2 == 0) {
                *left_over_len = len1;
                return;
            }
            else {
                do {
                    arr++;
                    len1--;
                } while(len1 != 0 && GRAIL_SORT_COMPARE(arr, arr + len1) - frag_type < 0);
            }
        }
    }
    *left_over_len = len2;
    *left_over_frag = frag_type;
}

/***** Sort With Extra Buffer *****/

// arr[dist..-1] - free, arr[0, left_end - 1] ++ arr[left_end, left_end + right_end - 1]
// -> arr[dist, dist + left_end + right_end - 1]
static void grail_merge_left_with_extra_buffer(GRAIL_SORT_TYPE* arr, int left_end, int right_end, int dist) {
    int left = 0, right = left_end;
    right_end += left_end;

    while(right < right_end) {
        if(left == left_end || GRAIL_SORT_COMPARE(arr + left, arr + right) > 0) {
            arr[dist++] = arr[right++];
        }
        else arr[dist++] = arr[left++];
    }
    if(dist != left) {
        while(left < left_end) arr[dist++] = arr[left++];
    }
}

static void grail_smart_merge_with_extra_buffer(GRAIL_SORT_TYPE* arr, int* left_over_len, int* left_over_frag, int block_len) {
    int dist = 0 - block_len;
    int left = 0, right = *left_over_len;
    int left_end = right, right_end = right + block_len;
    int frag_type = 1 - *left_over_frag; // 1 if inverted

    while(left < left_end && right < right_end) {
        if(GRAIL_SORT_COMPARE(arr + left, arr + right) - frag_type < 0) {
            arr[dist++] = arr[left++];
        }
        else arr[dist++] = arr[right++];
    }

    if(left < left_end) {
        *left_over_len = left_end - left;

        while(left < left_end) arr[--right_end] = arr[--left_end];
    }
    else {
        *left_over_len = right_end - right;
        *left_over_frag = frag_type;
    }
}

// arr - starting array. arr[0 - reg_block_len..-1] - buffer (if have_buffer).
// reg_block_len - length of regular blocks. First block_count are stable sorted by 1st elements and key-coded
// keys_pos - where keys are in array, in same order as blocks. keys_pos < midkey means stream A
// a_block_count are regular blocks from stream A.
// last_len is length of last (irregular) block from stream B, that should go before a_block_count blocks.
// last_len = 0 requires a_block_count = 0 (no irregular blocks). last_len > 0, a_block_count = 0 is possible.
static void grail_merge_buffers_left_with_extra_buffer(GRAIL_SORT_TYPE* keys_pos, GRAIL_SORT_TYPE* midkey, GRAIL_SORT_TYPE* arr,
        int block_count, int reg_block_len, int a_block_count, int last_len) {
    if(block_count == 0) {
        int a_block_len = a_block_count * reg_block_len;

        grail_merge_left_with_extra_buffer(arr, a_block_len, last_len, 0 - reg_block_len);
        return;
    }

    int left_over_len, process_index;
    left_over_len = process_index = reg_block_len;

    int left_over_frag = GRAIL_SORT_COMPARE(keys_pos, midkey) < 0 ? 0 : 1;
    int rest_to_process;

    for(int key_index = 1; key_index < block_count; key_index++, process_index += reg_block_len) {
        rest_to_process = process_index - left_over_len;
        int next_frag = GRAIL_SORT_COMPARE(keys_pos + key_index, midkey) < 0 ? 0 : 1;

        if(next_frag == left_over_frag) {
            memcpy(arr + rest_to_process - reg_block_len, arr + rest_to_process, left_over_len * sizeof(GRAIL_SORT_TYPE));

            rest_to_process = process_index;
            left_over_len = reg_block_len;
        }
        else {
            grail_smart_merge_with_extra_buffer(arr + rest_to_process, &left_over_len, &left_over_frag, reg_block_len);
        }
    }
    rest_to_process = process_index - left_over_len;

    if(last_len) {
        if(left_over_frag) {
            memcpy(arr + rest_to_process - reg_block_len, arr + rest_to_process, left_over_len * sizeof(GRAIL_SORT_TYPE));

            rest_to_process = process_index;
            left_over_len = reg_block_len * a_block_count;
            left_over_frag = 0;
        }
        else left_over_len += reg_block_len * a_block_count;

        grail_merge_left_with_extra_buffer(arr + rest_to_process, left_over_len, last_len, 0 - reg_block_len);
    }
    else {
        memcpy(arr + rest_to_process - reg_block_len, arr + rest_to_process, left_over_len * sizeof(GRAIL_SORT_TYPE));
    }
}

/***** End Sort With Extra Buffer *****/

// arr - starting array. arr[0 - block_len..-1] - buffer (if have_buffer).
// reg_block_len - length of regular blocks. First block_count blocks are stable sorted by 1st elements and key-coded
// keys_pos - where keys are located in arr, in same order as blocks. keys_pos < midkey means stream A
// a_block_count are regular blocks from stream A.
// last_len is length of last (irregular) block from stream B, that should go before a_block_count blocks.
// last_len = 0 requires a_block_count = 0 (no irregular blocks). last_len > 0, a_block_count = 0 is possible.
static void grail_merge_buffers_left(GRAIL_SORT_TYPE* keys_pos, GRAIL_SORT_TYPE* midkey, GRAIL_SORT_TYPE* arr, int block_count,
        int reg_block_len, bool have_buffer, int a_block_count, int last_len) {

    if(block_count == 0) {
        int total_a_len = a_block_count * reg_block_len;

        if(have_buffer) grail_merge_left(arr, total_a_len, last_len, 0 - reg_block_len);
        else grail_merge_without_buffer(arr, total_a_len, last_len);

        return;
    }

    int left_over_len, process_index;
    left_over_len = process_index = reg_block_len;
    int left_over_frag = GRAIL_SORT_COMPARE(keys_pos, midkey) < 0 ? 0 : 1;
    int rest_to_process;

    for(int key_index = 1; key_index < block_count; key_index++, process_index += reg_block_len){
        rest_to_process = process_index - left_over_len;
        int next_frag = GRAIL_SORT_COMPARE(keys_pos + key_index, midkey) < 0 ? 0 : 1;

        if(next_frag == left_over_frag) {
            if(have_buffer) grail_multi_swap(arr + rest_to_process - reg_block_len, arr + rest_to_process, left_over_len);

            rest_to_process = process_index;
            left_over_len = reg_block_len;
        }
        else {
            if(have_buffer) {
                grail_smart_merge_with_buffer(arr + rest_to_process, &left_over_len, &left_over_frag, reg_block_len);
            }
            else {
                grail_smart_merge_without_buffer(arr + rest_to_process, &left_over_len, &left_over_frag, reg_block_len);
            }
        }
    }

    rest_to_process = process_index - left_over_len;

    if(last_len) {
        if(left_over_frag) {
            if(have_buffer) grail_multi_swap(arr + rest_to_process - reg_block_len, arr + rest_to_process, left_over_len);

            rest_to_process = process_index;
            left_over_len = reg_block_len * a_block_count;
            left_over_frag = 0;
        }
        else {
            left_over_len += reg_block_len * a_block_count;
        }
        if(have_buffer) grail_merge_left(arr + rest_to_process, left_over_len, last_len, 0 - reg_block_len);
        else grail_merge_without_buffer(arr + rest_to_process, left_over_len, last_len);
    }
    else {
        if(have_buffer) grail_multi_swap(arr + rest_to_process, arr + (rest_to_process - reg_block_len), left_over_len);
    }
}

// build blocks of length build_len
// input: [0 - build_len, 0 - 1] elements are buffer
// output: first build_len elements are buffer, blocks 2 * build_len and last subblock sorted
static void grail_build_blocks(GRAIL_SORT_TYPE* arr, int len, int build_len, GRAIL_SORT_TYPE* ext_buf, int ext_buf_len) {
    int build_buf = build_len < ext_buf_len ? build_len : ext_buf_len;

    while((build_buf & (build_buf - 1)) != 0) {
        build_buf &= build_buf - 1;  // max power or 2 - just in case
    }

    int extra_dist, part;

    if(build_buf) {
        memcpy(ext_buf, arr - build_buf, build_buf * sizeof(GRAIL_SORT_TYPE));

        for(int dist = 1; dist < len; dist += 2) {
            extra_dist = 0;
            if(GRAIL_SORT_COMPARE(arr + (dist - 1), arr + dist) > 0) extra_dist = 1;

            arr[dist - 3] = arr[dist - 1 + extra_dist];
            arr[dist - 2] = arr[dist - extra_dist];
        }
        if(len % 2) arr[len - 3] = arr[len - 1];

        arr -= 2;

        for(part = 2; part < build_buf; part *= 2) {
            int left = 0, right = len - 2 * part;

            while(left <= right) {
                grail_merge_left_with_extra_buffer(arr + left, part, part, 0 - part);
                left += 2 * part;
            }

            int rest = len - left;

            if(rest > part) grail_merge_left_with_extra_buffer(arr + left, part, rest - part, 0 - part);
            else {
                while(left < len) {
                    arr[left - part] = arr[left];
                    left++;
                }
            }
            arr -= part;
        }
        memcpy(arr + len, ext_buf, build_buf * sizeof(GRAIL_SORT_TYPE));
    }
    else {
        for(int dist = 1; dist < len; dist += 2) {
            extra_dist = 0;
            if(GRAIL_SORT_COMPARE(arr + (dist - 1), arr + dist) > 0) extra_dist = 1;

            grail_swap(arr + (dist - 3), arr + (dist - 1 + extra_dist));
            grail_swap(arr + (dist - 2), arr + (dist - extra_dist));
        }

        if(len % 2) grail_swap(arr + (len - 1), arr + (len - 3));

        arr -= 2;
        part = 2;
    }

    while(part < build_len) {
        int left = 0, right = len - 2 * part;

        while(left <= right) {
            grail_merge_left(arr + left, part, part, 0 - part);
            left += 2 * part;
        }

        int rest = len - left;

        if(rest > part) {
            grail_merge_left(arr + left, part, rest - part, 0 - part);
        }
        else grail_rotate(arr + left - part, part, rest);

        arr -= part;
        part *= 2;
    }

    int rest_to_build = len % (2 * build_len);
    int left_over_pos = len - rest_to_build;

    if(rest_to_build <= build_len) grail_rotate(arr + left_over_pos, rest_to_build, build_len);
    else grail_merge_right(arr + left_over_pos, build_len, rest_to_build - build_len, build_len);

    while(left_over_pos > 0) {
        left_over_pos -= 2 * build_len;
        grail_merge_right(arr + left_over_pos, build_len, build_len, build_len);
    }
}

// keys are on the left of arr. Blocks of length build_len combined. We'll combine them into pairs
// build_len and nkeys are powers of 2. (2 * build_len / reg_block_len) keys are guaranteed
static void grail_combine_blocks(GRAIL_SORT_TYPE* keys_pos, GRAIL_SORT_TYPE *arr, int len, int build_len, int reg_block_len,
        bool have_buffer, GRAIL_SORT_TYPE* ext_buf) {

    int combined_len = len / (2 * build_len);
    int left_over = len % (2 * build_len);

    if(left_over <= build_len) {
        len -= left_over;
        left_over = 0;
    }

    if(ext_buf) memcpy(ext_buf, arr - reg_block_len, reg_block_len * sizeof(GRAIL_SORT_TYPE));

    for(int i = 0; i <= combined_len; i++) {
        if(i == combined_len && left_over == 0) break;

        GRAIL_SORT_TYPE* block_pos = arr + i * 2 * build_len;
        int block_count = (i == combined_len ? left_over : 2 * build_len) / reg_block_len;

        grail_insert_sort(keys_pos, block_count + (i == combined_len ? 1 : 0));

        int midkey = build_len / reg_block_len;

        for(int index = 1; index < block_count; index++) {
            int left_index = index - 1;

            for(int right_index = index; right_index < block_count; right_index++) {
                int right_comp = GRAIL_SORT_COMPARE(block_pos + left_index * reg_block_len, block_pos + right_index * reg_block_len);

                if(right_comp > 0 || (right_comp == 0 && GRAIL_SORT_COMPARE(keys_pos + left_index, keys_pos + right_index) > 0)) {
                    left_index = right_index;
                }
            }

            if(left_index != index - 1) {
                grail_multi_swap(block_pos + (index - 1) * reg_block_len, block_pos + left_index * reg_block_len, reg_block_len);
                grail_swap(keys_pos + (index - 1), keys_pos + left_index);

                if(midkey == index - 1 || midkey == left_index) {
                    midkey ^= (index - 1) ^ left_index;
                }
            }
        }

        int a_block_count, last_len;
        a_block_count = last_len = 0;
        if(i == combined_len) last_len = left_over % reg_block_len;

        if(last_len != 0) {
            while(a_block_count < block_count &&
                    GRAIL_SORT_COMPARE( (block_pos + block_count * reg_block_len),
                            (block_pos + (block_count - a_block_count - 1) * reg_block_len) ) < 0) {

                a_block_count++;
            }
        }

        if(ext_buf) {
            grail_merge_buffers_left_with_extra_buffer(keys_pos, keys_pos + midkey, block_pos, block_count - a_block_count,
                    reg_block_len, a_block_count, last_len);
        }
        else {
            grail_merge_buffers_left(keys_pos, keys_pos + midkey, block_pos, block_count - a_block_count,
                    reg_block_len, have_buffer, a_block_count, last_len);
        }
    }
    if(ext_buf) {
        for(int i = len - 1; i >= 0; i--) arr[i] = arr[i - reg_block_len];

        memcpy(arr - reg_block_len, ext_buf, reg_block_len * sizeof(GRAIL_SORT_TYPE));
    }
    else if(have_buffer) {
        while(--len >= 0) grail_swap(arr + len, arr + len - reg_block_len);
    }
}

static void grail_lazy_stable_sort(GRAIL_SORT_TYPE* arr, int len) {
    for(int dist = 1; dist < len; dist += 2) {
        if(GRAIL_SORT_COMPARE(arr + dist - 1, arr + dist) > 0) grail_swap(arr + (dist - 1), arr + dist);
    }

    for(int part = 2; part < len; part *= 2) {
        int left = 0, right = len - 2 * part;

        while(left <= right) {
            grail_merge_without_buffer(arr + left, part, part);
            left += 2 * part;
        }

        int rest = len - left;

        if(rest > part) grail_merge_without_buffer(arr + left, part, rest - part);
    }
}

void grail_common_sort(GRAIL_SORT_TYPE* arr, int len, GRAIL_SORT_TYPE* ext_buf, int ext_buf_len) {
    if(len <= 16) {
        grail_insert_sort(arr, len);
        return;
    }

    int block_len = 1;
    while(block_len * block_len < len) block_len *= 2;

    int key_count = (len - 1) / block_len + 1;
    int keys_found = grail_find_keys(arr, len, key_count + block_len);

    bool buffer_enabled = true;

    if(keys_found < key_count + block_len) {
        if(keys_found < 4) {
            grail_lazy_stable_sort(arr, len);
            return;
        }

        key_count = block_len;
        while(key_count > keys_found) key_count /= 2;

        buffer_enabled = false;
        block_len = 0;
    }

    int dist = block_len + key_count;
    int build_len = buffer_enabled ? block_len : key_count;

    if(buffer_enabled) grail_build_blocks(arr + dist, len - dist, build_len, ext_buf, ext_buf_len);
    else grail_build_blocks(arr + dist, len - dist, build_len, NULL, 0);

    // 2 * build_len are built
    while(len - dist > (build_len *= 2)) {
        int reg_block_len = block_len;
        bool build_buf_enabled = buffer_enabled;

        if(!buffer_enabled) {
            if(key_count > 4 && key_count / 8 * key_count >= build_len) {
                reg_block_len = key_count / 2;
                build_buf_enabled = true;
            }
            else {
                int calc_keys = 1;
                long long i = (long long) build_len * keys_found / 2;

                while(calc_keys < key_count && i != 0) {
                    calc_keys *= 2;
                    i /= 8;
                }
                reg_block_len = (2 * build_len) / calc_keys;
            }
        }
        grail_combine_blocks(arr, arr + dist, len - dist, build_len, reg_block_len, build_buf_enabled,
                build_buf_enabled && reg_block_len <= ext_buf_len ? ext_buf : NULL);
    }
    grail_insert_sort(arr, dist);
    grail_merge_without_buffer(arr, dist, len - dist);
}

void grail_sort(GRAIL_SORT_TYPE* arr, int len) {
    grail_common_sort(arr, len, NULL, 0);
}

void grail_sort_with_static_buffer(GRAIL_SORT_TYPE* arr, int len) {
    GRAIL_SORT_TYPE ext_buffer[GRAIL_STATIC_EXT_BUFFER_LENGTH];
    grail_common_sort(arr, len, ext_buffer, GRAIL_STATIC_EXT_BUFFER_LENGTH);
}

void grail_sort_with_dynamic_buffer(GRAIL_SORT_TYPE* arr, int len) {
    int temp_len = 1;
    while(temp_len * temp_len < len) temp_len *= 2;

    GRAIL_SORT_TYPE* ext_buffer = (GRAIL_SORT_TYPE*) malloc(temp_len * sizeof(GRAIL_SORT_TYPE));

    if(ext_buffer == NULL) grail_sort_with_static_buffer(arr, len);
    else {
        grail_common_sort(arr, len, ext_buffer, temp_len);
        free(ext_buffer);
    }
}

/****** classic MergeInPlace *************/

static void grail_rec_merge(GRAIL_SORT_TYPE* arr, int len1, int len2) {
    if(len1 < 3 || len2 < 3) {
        grail_merge_without_buffer(arr, len1, len2);
        return;
    }

    int midpoint;
    if(len1 < len2) midpoint = len1 + len2 / 2;
    else midpoint = len1 / 2;

    //Left binary search
    int len1_left, len1_right;
    len1_left = len1_right = grail_binary_search(arr, len1, arr + midpoint, false);

    //Right binary search
    if(len1_right < len1 && GRAIL_SORT_COMPARE(arr + len1_right, arr + midpoint) == 0) {
        len1_right = grail_binary_search(arr + len1_left, len1 - len1_left, arr + midpoint, true) + len1_left;
    }

    int len2_left, len2_right;
    len2_left = len2_right = grail_binary_search(arr + len1, len2, arr + midpoint, false);

    if(len2_right < len2 && GRAIL_SORT_COMPARE(arr + len1 + len2_right, arr + midpoint) == 0) {
        len2_right = grail_binary_search(arr + len1 + len2_left, len2 - len2_left, arr + midpoint, true) + len2_left;
    }

    if(len1_left == len1_right) grail_rotate(arr + len1_right, len1 - len1_right, len2_right);
    else {
        grail_rotate(arr + len1_left, len1 - len1_left, len2_left);

        if(len2_right != len2_left) {
            grail_rotate(arr + (len1_right + len2_left), len1 - len1_right, len2_right - len2_left);
        }
    }

    grail_rec_merge(arr + (len1_right + len2_right), len1 - len1_right, len2 - len2_right);
    grail_rec_merge(arr, len1_left, len2_left);
}
void rec_stable_sort(GRAIL_SORT_TYPE* arr, int len) {
    for(int dist = 1; dist < len; dist += 2) {
        if(GRAIL_SORT_COMPARE(arr + dist - 1, arr + dist) > 0) grail_swap(arr + (dist - 1), arr + dist);
    }
    for(int part = 2; part < len; part *= 2) {
        int left = 0, right = len - 2 * part;

        while(left <= right) {
            grail_rec_merge(arr + left, part, part);
            left += 2 * part;
        }

        int rest = len - left;
        if(rest > part) grail_rec_merge(arr + left, part, rest-part);
    }
}

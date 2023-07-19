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
/* For sorting with dynamic external buffer (O(sqrt(N)) items) */
/* use GrailSortWithDynBuffer()                          */
/*                                                       */
/* Also classic in-place merge sort is implemented       */
/* under the name of RecStableSort()                     */
/*                                                       */
/*********************************************************/

#include<iterator>
#include<utility>

namespace grailsort {
	namespace _internal {
		const int GRAIL_EXT_BUFFER_LENGTH = 512;

		template<typename Iter>
		using iter_value = typename std::iterator_traits<Iter>::value_type;

		template<typename Iter>
		inline void grail_swap1(Iter a, Iter b) {
			iter_value<Iter> c = *a;
			*a = *b;
			*b = c;
		}

		template<typename Iter>
		inline void grail_swapN(Iter a, Iter b, int n) {
			while (n--) grail_swap1(a++, b++);
		}

		template<typename Iter>
		static void grail_rotate(Iter a, int l1, int l2) {
			while (l1 && l2) {
				if (l1 <= l2) {
					grail_swapN(a, a + l1, l1);
					a += l1; l2 -= l1;
				}
				else {
					grail_swapN(a + (l1 - l2), a + l1, l2);
					l1 -= l2;
				}
			}
		}

		template<typename Iter1, typename Iter2, typename Comp>
		static int grail_BinSearchLeft(Iter1 arr, int len, Iter2 key, Comp comp) {
			int a = -1, b = len, c;
			while (a < b - 1) {
				c = a + ((b - a) >> 1);
				if (comp(arr + c, key) >= 0) b = c;
				else a = c;
			}
			return b;
		}

		template<typename Iter1, typename Iter2, typename Comp>
		static int grail_BinSearchRight(Iter1 arr, int len, Iter2 key, Comp comp) {
			int a = -1, b = len, c;
			while (a < b - 1) {
				c = a + ((b - a) >> 1);
				if (comp(arr + c, key) > 0) b = c;
				else a = c;
			}
			return b;
		}

		// cost: 2*len+nk^2/2
		template<typename Iter, typename Comp>
		static int grail_FindKeys(Iter arr, int len, int nkeys, Comp comp) {
			int h = 1, h0 = 0;  // first key is always here
			int u = 1, r;
			while (u < len && h < nkeys) {
				r = grail_BinSearchLeft(arr + h0, h, arr + u, comp);
				if (r == h || comp(arr + u, arr + (h0 + r)) != 0) {
					grail_rotate(arr + h0, h, u - (h0 + h));
					h0 = u - h;
					grail_rotate(arr + (h0 + r), h - r, 1);
					h++;
				}
				u++;
			}
			grail_rotate(arr, h0, h);
			return h;
		}

		template<typename Iter, typename Comp>
		static void grail_MergeWithoutBuffer(Iter arr, int len1, int len2, Comp comp) {
			int h;
			if (len1 < len2) {
				while (len1) {
					h = grail_BinSearchLeft(arr + len1, len2, arr, comp);
					if (h != 0) {
						grail_rotate(arr, len1, h);
						arr += h;
						len2 -= h;
					}
					if (len2 == 0) break;
					do {
						arr++; len1--;
					} while (len1 && comp(arr, arr + len1) <= 0);
				}
			}
			else {
				while (len2) {
					h = grail_BinSearchRight(arr, len1, arr + (len1 + len2 - 1), comp);
					if (h != len1) {
						grail_rotate(arr + h, len1 - h, len2);
						len1 = h;
					}
					if (len1 == 0) break;
					do {
						len2--;
					} while (len2 && comp(arr + len1 - 1, arr + len1 + len2 - 1) <= 0);
				}
			}
		}

		// arr[M..-1] - buffer, arr[0,L1-1]++arr[L1,L1+L2-1] -> arr[M,M+L1+L2-1]
		template<typename Iter, typename Comp>
		static void grail_MergeLeft(Iter arr, int L1, int L2, int M, Comp comp) {
			int p0 = 0, p1 = L1; L2 += L1;
			while (p1 < L2) {
				if (p0 == L1 || comp(arr + p0, arr + p1) > 0) {
					grail_swap1(arr + (M++), arr + (p1++));
				}
				else {
					grail_swap1(arr + (M++), arr + (p0++));
				}
			}
			if (M != p0) grail_swapN(arr + M, arr + p0, L1 - p0);
		}

		template<typename Iter, typename Comp>
		static void grail_MergeRight(Iter arr, int L1, int L2, int M, Comp comp) {
			int p0 = L1 + L2 + M - 1, p2 = L1 + L2 - 1, p1 = L1 - 1;

			while (p1 >= 0) {
				if (p2 < L1 || comp(arr + p1, arr + p2)>0) {
					grail_swap1(arr + (p0--), arr + (p1--));
				}
				else {
					grail_swap1(arr + (p0--), arr + (p2--));
				}
			}
			if (p2 != p0) while (p2 >= L1) grail_swap1(arr + (p0--), arr + (p2--));
		}

		template<typename Iter, typename Comp>
		static void grail_SmartMergeWithBuffer(Iter arr, int* alen1, int* atype, int len2, int lkeys, Comp comp) {
			int p0 = -lkeys, p1 = 0, p2 = *alen1, q1 = p2, q2 = p2 + len2;
			int ftype = 1 - *atype;  // 1 if inverted
			while (p1 < q1 && p2 < q2) {
				if (comp(arr + p1, arr + p2) - ftype < 0) grail_swap1(arr + (p0++), arr + (p1++));
				else grail_swap1(arr + (p0++), arr + (p2++));
			}
			if (p1 < q1) {
				*alen1 = q1 - p1;
				while (p1 < q1) grail_swap1(arr + (--q1), arr + (--q2));
			}
			else {
				*alen1 = q2 - p2;
				*atype = ftype;
			}
		}

		template<typename Iter, typename Comp>
		static void grail_SmartMergeWithoutBuffer(Iter arr, int* alen1, int* atype, int _len2, Comp comp) {
			int len1, len2, ftype, h;

			if (!_len2) return;
			len1 = *alen1;
			len2 = _len2;
			ftype = 1 - *atype;
			if (len1 && comp(arr + (len1 - 1), arr + len1) - ftype >= 0) {
				while (len1) {
					h = ftype ? grail_BinSearchLeft(arr + len1, len2, arr, comp) : grail_BinSearchRight(arr + len1, len2, arr, comp);
					if (h != 0) {
						grail_rotate(arr, len1, h);
						arr += h;
						len2 -= h;
					}
					if (len2 == 0) {
						*alen1 = len1;
						return;
					}
					do {
						arr++; len1--;
					} while (len1 && comp(arr, arr + len1) - ftype < 0);
				}
			}
			*alen1 = len2; *atype = ftype;
		}

		/***** Sort With Extra Buffer *****/

		// arr[M..-1] - free, arr[0,L1-1]++arr[L1,L1+L2-1] -> arr[M,M+L1+L2-1]
		template<typename Iter, typename Comp>
		static void grail_MergeLeftWithXBuf(Iter arr, int L1, int L2, int M, Comp comp) {
			int p0 = 0, p1 = L1; L2 += L1;
			while (p1 < L2) {
				if (p0 == L1 || comp(arr + p0, arr + p1) > 0) arr[M++] = arr[p1++];
				else arr[M++] = arr[p0++];
			}
			if (M != p0) while (p0 < L1) arr[M++] = arr[p0++];
		}

		template<typename Iter, typename Comp>
		static void grail_SmartMergeWithXBuf(Iter arr, int* alen1, int* atype, int len2, int lkeys, Comp comp) {
			int p0 = -lkeys, p1 = 0, p2 = *alen1, q1 = p2, q2 = p2 + len2;
			int ftype = 1 - *atype;  // 1 if inverted
			while (p1 < q1 && p2 < q2) {
				if (comp(arr + p1, arr + p2) - ftype < 0) arr[p0++] = arr[p1++];
				else arr[p0++] = arr[p2++];
			}
			if (p1 < q1) {
				*alen1 = q1 - p1;
				while (p1 < q1) arr[--q2] = arr[--q1];
			}
			else {
				*alen1 = q2 - p2;
				*atype = ftype;
			}
		}

		// arr - starting array. arr[-lblock..-1] - buffer (if havebuf).
		// lblock - length of regular blocks. First nblocks are stable sorted by 1st elements and key-coded
		// keys - arrays of keys, in same order as blocks. key<midkey means stream A
		// nblock2 are regular blocks from stream A. llast is length of last (irregular) block from stream B, that should go before nblock2 blocks.
		// llast=0 requires nblock2=0 (no irregular blocks). llast>0, nblock2=0 is possible.
		template<typename Iter1, typename Iter2, typename Iter3, typename Comp>
		static void grail_MergeBuffersLeftWithXBuf(Iter1 keys, Iter2 midkey, Iter3 arr, int nblock, int lblock, int nblock2, int llast, Comp comp) {
			int l, prest, lrest, frest, pidx, cidx, fnext, plast;

			if (nblock == 0) {
				l = nblock2 * lblock;
				grail_MergeLeftWithXBuf(arr, l, llast, -lblock, comp);
				return;
			}

			lrest = lblock;
			frest = comp(keys, midkey) < 0 ? 0 : 1;
			pidx = lblock;
			for (cidx = 1; cidx < nblock; cidx++, pidx += lblock) {
				prest = pidx - lrest;
				fnext = comp(keys + cidx, midkey) < 0 ? 0 : 1;
				if (fnext == frest) {
					std::copy_n(arr + prest, lrest, arr + prest - lblock);
					prest = pidx;
					lrest = lblock;
				}
				else {
					grail_SmartMergeWithXBuf(arr + prest, &lrest, &frest, lblock, lblock, comp);
				}
			}
			prest = pidx - lrest;
			if (llast) {
				plast = pidx + lblock * nblock2;
				if (frest) {
					std::copy_n(arr + prest, lrest, arr + prest - lblock);
					prest = pidx;
					lrest = lblock * nblock2;
					frest = 0;
				}
				else {
					lrest += lblock * nblock2;
				}
				grail_MergeLeftWithXBuf(arr + prest, lrest, llast, -lblock, comp);
			}
			else {
				std::copy_n(arr + prest, lrest, arr + prest - lblock);
			}
		}

		/***** End Sort With Extra Buffer *****/

		// build blocks of length K
		// input: [-K,-1] elements are buffer
		// output: first K elements are buffer, blocks 2*K and last subblock sorted
		template<typename Iter1, typename Iter2, typename Comp>
		static void grail_BuildBlocks(Iter1 arr, int L, int K, Iter2 extbuf, int LExtBuf, Comp comp) {
			int m, u, h, p0, p1, rest, restk, p, kbuf;
			kbuf = K < LExtBuf ? K : LExtBuf;
			while (kbuf & (kbuf - 1)) kbuf &= kbuf - 1;  // max power or 2 - just in case

			if (kbuf) {
				std::copy_n(arr - kbuf, kbuf, extbuf);
				for (m = 1; m < L; m += 2) {
					u = 0;
					if (comp(arr + (m - 1), arr + m) > 0) u = 1;
					arr[m - 3] = arr[m - 1 + u];
					arr[m - 2] = arr[m - u];
				}
				if (L % 2) arr[L - 3] = arr[L - 1];
				arr -= 2;
				for (h = 2; h < kbuf; h *= 2) {
					p0 = 0;
					p1 = L - 2 * h;
					while (p0 <= p1) {
						grail_MergeLeftWithXBuf(arr + p0, h, h, -h, comp);
						p0 += 2 * h;
					}
					rest = L - p0;
					if (rest > h) {
						grail_MergeLeftWithXBuf(arr + p0, h, rest - h, -h, comp);
					}
					else {
						for (; p0 < L; p0++)	arr[p0 - h] = arr[p0];
					}
					arr -= h;
				}
				std::copy_n(extbuf, kbuf, arr + L);
			}
			else {
				for (m = 1; m < L; m += 2) {
					u = 0;
					if (comp(arr + (m - 1), arr + m) > 0) u = 1;
					grail_swap1(arr + (m - 3), arr + (m - 1 + u));
					grail_swap1(arr + (m - 2), arr + (m - u));
				}
				if (L % 2) grail_swap1(arr + (L - 1), arr + (L - 3));
				arr -= 2;
				h = 2;
			}
			for (; h < K; h *= 2) {
				p0 = 0;
				p1 = L - 2 * h;
				while (p0 <= p1) {
					grail_MergeLeft(arr + p0, h, h, -h, comp);
					p0 += 2 * h;
				}
				rest = L - p0;
				if (rest > h) {
					grail_MergeLeft(arr + p0, h, rest - h, -h, comp);
				}
				else grail_rotate(arr + p0 - h, h, rest);
				arr -= h;
			}
			restk = L % (2 * K);
			p = L - restk;
			if (restk <= K) grail_rotate(arr + p, restk, K);
			else grail_MergeRight(arr + p, K, restk - K, K, comp);
			while (p > 0) {
				p -= 2 * K;
				grail_MergeRight(arr + p, K, K, K, comp);
			}
		}

		// arr - starting array. arr[-lblock..-1] - buffer (if havebuf).
		// lblock - length of regular blocks. First nblocks are stable sorted by 1st elements and key-coded
		// keys - arrays of keys, in same order as blocks. key<midkey means stream A
		// nblock2 are regular blocks from stream A. llast is length of last (irregular) block from stream B, that should go before nblock2 blocks.
		// llast=0 requires nblock2=0 (no irregular blocks). llast>0, nblock2=0 is possible.
		template<typename Iter1, typename Iter2, typename Iter3, typename Comp>
		static void grail_MergeBuffersLeft(Iter1 keys, Iter2 midkey, Iter3 arr, int nblock, int lblock, bool havebuf, int nblock2, int llast, Comp comp) {
			int l, prest, lrest, frest, pidx, cidx, fnext, plast;

			if (nblock == 0) {
				l = nblock2 * lblock;
				if (havebuf) grail_MergeLeft(arr, l, llast, -lblock, comp);
				else grail_MergeWithoutBuffer(arr, l, llast, comp);
				return;
			}

			lrest = lblock;
			frest = comp(keys, midkey) < 0 ? 0 : 1;
			pidx = lblock;
			for (cidx = 1; cidx < nblock; cidx++, pidx += lblock) {
				prest = pidx - lrest;
				fnext = comp(keys + cidx, midkey) < 0 ? 0 : 1;
				if (fnext == frest) {
					if (havebuf) grail_swapN(arr + prest - lblock, arr + prest, lrest);
					prest = pidx;
					lrest = lblock;
				}
				else {
					if (havebuf) {
						grail_SmartMergeWithBuffer(arr + prest, &lrest, &frest, lblock, lblock, comp);
					}
					else {
						grail_SmartMergeWithoutBuffer(arr + prest, &lrest, &frest, lblock, comp);
					}

				}
			}
			prest = pidx - lrest;
			if (llast) {
				plast = pidx + lblock * nblock2;
				if (frest) {
					if (havebuf) grail_swapN(arr + prest - lblock, arr + prest, lrest);
					prest = pidx;
					lrest = lblock * nblock2;
					frest = 0;
				}
				else {
					lrest += lblock * nblock2;
				}
				if (havebuf) grail_MergeLeft(arr + prest, lrest, llast, -lblock, comp);
				else grail_MergeWithoutBuffer(arr + prest, lrest, llast, comp);
			}
			else {
				if (havebuf) grail_swapN(arr + prest, arr + (prest - lblock), lrest);
			}
		}

		template<typename Iter, typename Comp>
		static void grail_SortIns(Iter arr, int len, Comp comp) {
			int i, j;
			for (i = 1; i < len; i++) {
				for (j = i - 1; j >= 0 && comp(arr + (j + 1), arr + j) < 0; j--) grail_swap1(arr + j, arr + (j + 1));
			}
		}

		template<typename Iter, typename Comp>
		static void grail_LazyStableSort(Iter arr, int L, Comp comp) {
			int m, u, h, p0, p1, rest;
			for (m = 1; m < L; m += 2) {
				u = 0;
				if (comp(arr + m - 1, arr + m) > 0) grail_swap1(arr + (m - 1), arr + m);
			}
			for (h = 2; h < L; h *= 2) {
				p0 = 0;
				p1 = L - 2 * h;
				while (p0 <= p1) {
					grail_MergeWithoutBuffer(arr + p0, h, h, comp);
					p0 += 2 * h;
				}
				rest = L - p0;
				if (rest > h) grail_MergeWithoutBuffer(arr + p0, h, rest - h, comp);
			}
		}

		// keys are on the left of arr. Blocks of length LL combined. We'll combine them in pairs
		// LL and nkeys are powers of 2. (2*LL/lblock) keys are guarantied

		template<typename Iter1, typename Iter2, typename Iter3, typename Comp>
		static void grail_CombineBlocks(Iter1 keys, Iter2 arr, int len, int LL, int lblock, bool havebuf, Iter3 xbuf, Comp comp) {
			int M, nkeys, b, NBlk, midkey, lrest, u, p, v, kc, nbl2, llast;
			Iter2 arr1;

			M = len / (2 * LL);
			lrest = len % (2 * LL);
			nkeys = (2 * LL) / lblock;
			if (lrest <= LL) {
				len -= lrest;
				lrest = 0;
			}
			if (xbuf) std::copy_n(arr - lblock, lblock, xbuf);
			for (b = 0; b <= M; b++) {
				if (b == M && lrest == 0) break;
				arr1 = arr + b * 2 * LL;
				NBlk = (b == M ? lrest : 2 * LL) / lblock;
				grail_SortIns(keys, NBlk + (b == M ? 1 : 0), comp);
				midkey = LL / lblock;
				for (u = 1; u < NBlk; u++) {
					p = u - 1;
					for (v = u; v < NBlk; v++) {
						kc = comp(arr1 + p * lblock, arr1 + v * lblock);
						if (kc > 0 || (kc == 0 && comp(keys + p, keys + v) > 0)) p = v;
					}
					if (p != u - 1) {
						grail_swapN(arr1 + (u - 1) * lblock, arr1 + p * lblock, lblock);
						grail_swap1(keys + (u - 1), keys + p);
						if (midkey == u - 1 || midkey == p) midkey ^= (u - 1) ^ p;
					}
				}
				nbl2 = llast = 0;
				if (b == M) llast = lrest % lblock;
				if (llast != 0) {
					while (nbl2 < NBlk && comp(arr1 + NBlk * lblock, arr1 + (NBlk - nbl2 - 1) * lblock) < 0) nbl2++;
				}
				if (xbuf) grail_MergeBuffersLeftWithXBuf(keys, keys + midkey, arr1, NBlk - nbl2, lblock, nbl2, llast, comp);
				else grail_MergeBuffersLeft(keys, keys + midkey, arr1, NBlk - nbl2, lblock, havebuf, nbl2, llast, comp);
			}
			if (xbuf) {
				for (p = len; --p >= 0;) arr[p] = arr[p - lblock];
				std::copy_n(xbuf, lblock, arr - lblock);
			}
			else if (havebuf) while (--len >= 0) grail_swap1(arr + len, arr + len - lblock);
		}

		template<typename Iter1, typename Iter2, typename Comp>
		void grail_commonSort(Iter1 arr, int Len, Iter2 extbuf, int LExtBuf, Comp comp) {
			int lblock, nkeys, findkeys, ptr, cbuf, lb, nk;
			bool havebuf, chavebuf;
			long long s;

			if (Len < 16) {
				grail_SortIns(arr, Len, comp);
				return;
			}

			lblock = 1;
			while (lblock * lblock < Len) lblock *= 2;
			nkeys = (Len - 1) / lblock + 1;
			findkeys = grail_FindKeys(arr, Len, nkeys + lblock, comp);
			havebuf = true;
			if (findkeys < nkeys + lblock) {
				if (findkeys < 4) {
					grail_LazyStableSort(arr, Len, comp);
					return;
				}
				nkeys = lblock;
				while (nkeys > findkeys) nkeys /= 2;
				havebuf = false;
				lblock = 0;
			}
			ptr = lblock + nkeys;
			cbuf = havebuf ? lblock : nkeys;
			if (havebuf) grail_BuildBlocks(arr + ptr, Len - ptr, cbuf, extbuf, LExtBuf, comp);
			else grail_BuildBlocks(arr + ptr, Len - ptr, cbuf, (Iter2)0, 0, comp);

			// 2*cbuf are built
			while (Len - ptr > (cbuf *= 2)) {
				lb = lblock;
				chavebuf = havebuf;
				if (!havebuf) {
					if (nkeys > 4 && nkeys / 8 * nkeys >= cbuf) {
						lb = nkeys / 2;
						chavebuf = true;
					}
					else {
						nk = 1;
						s = (long long)cbuf * findkeys / 2;
						while (nk < nkeys && s != 0) {
							nk *= 2; s /= 8;
						}
						lb = (2 * cbuf) / nk;
					}
				}
				else {
#if 0
					if (LExtBuf != 0) {
						while (lb > LExtBuf && lb * lb > 2 * cbuf) lb /= 2;  // set size of block close to sqrt(new_block_length)
					}
#endif
				}
				grail_CombineBlocks(arr, arr + ptr, Len - ptr, cbuf, lb, chavebuf, chavebuf && lb <= LExtBuf ? extbuf : NULL, comp);
			}
			grail_SortIns(arr, ptr, comp);
			grail_MergeWithoutBuffer(arr, ptr, Len - ptr, comp);
		}

		template<typename Iter, typename Comp>
		void GrailSort(Iter arr, int Len, Comp comp) {
			iter_value<Iter>* buffer = NULL;
			grail_commonSort(arr, Len, buffer, 0, comp);
		}

		template<typename Iter, typename Comp>
		void GrailSortWithBuffer(Iter arr, int Len, Comp comp) {
			iter_value<Iter> ExtBuf[GRAIL_EXT_BUFFER_LENGTH];
			grail_commonSort(arr, Len, ExtBuf, GRAIL_EXT_BUFFER_LENGTH, comp);
		}

		template<typename Iter, typename Comp>
		void GrailSortWithDynBuffer(Iter arr, int Len, Comp comp) {
			int L = 1;
			iter_value<Iter>* ExtBuf;
			while (L * L < Len) L *= 2;
			ExtBuf = new iter_value<Iter>[L];
			if (ExtBuf == NULL) GrailSortWithBuffer(arr, Len, comp);
			else {
				grail_commonSort(arr, Len, ExtBuf, L, comp);
				delete[] ExtBuf;
			}
		}

		/****** classic MergeInPlace *************/

		template<typename Iter, typename Comp>
		static void grail_RecMerge(Iter A, int L1, int L2, Comp comp) {
			int K, k1, k2, m1, m2;
			if (L1 < 3 || L2 < 3) {
				grail_MergeWithoutBuffer(A, L1, L2, comp); return;
			}
			if (L1 < L2) K = L1 + L2 / 2;
			else K = L1 / 2;
			k1 = k2 = grail_BinSearchLeft(A, L1, A + K, comp);
			if (k2 < L1 && comp(A + k2, A + K) == 0) k2 = grail_BinSearchRight(A + k1, L1 - k1, A + K, comp) + k1;
			m1 = grail_BinSearchLeft(A + L1, L2, A + K, comp);
			m2 = m1;
			if (m2 < L2 && comp(A + L1 + m2, A + K) == 0) m2 = grail_BinSearchRight(A + L1 + m1, L2 - m1, A + K, comp) + m1;
			if (k1 == k2) grail_rotate(A + k2, L1 - k2, m2);
			else {
				grail_rotate(A + k1, L1 - k1, m1);
				if (m2 != m1) grail_rotate(A + (k2 + m1), L1 - k2, m2 - m1);
			}
			grail_RecMerge(A + (k2 + m2), L1 - k2, L2 - m2, comp);
			grail_RecMerge(A, k1, m1, comp);
		}

		template<typename Iter, typename Comp>
		void RecStableSort(Iter arr, int L, Comp comp) {
			int u, m, h, p0, p1, rest;

			for (m = 1; m < L; m += 2) {
				u = 0;
				if (comp(arr + m - 1, arr + m) > 0) grail_swap1(arr + (m - 1), arr + m);
			}
			for (h = 2; h < L; h *= 2) {
				p0 = 0;
				p1 = L - 2 * h;
				while (p0 <= p1) {
					grail_RecMerge(arr + p0, h, h, comp);
					p0 += 2 * h;
				}
				rest = L - p0;
				if (rest > h) grail_RecMerge(arr + p0, h, rest - h, comp);
			}
		}
	}

	template<typename Compare>
	struct comperator {
		Compare compare;

		explicit comperator(Compare&& comp) :
			compare(std::forward<Compare>(comp))
		{}

		template<typename T, typename U>
		int operator()(T* lhs, U* rhs)
		{
			if (compare(*lhs, *rhs)) {
				return -1;
			}
			if (compare(*rhs, *lhs)) {
				return 1;
			}
			return 0;
		}
	};

	// Custom Comperator
	template<typename RandomAccessIterator, typename Comp>
	void grail_sort(RandomAccessIterator first, RandomAccessIterator last, Comp comp) {
		_internal::GrailSort(
			first, (int)std::distance(first, last), comperator<Comp>(std::move(comp)));
	}

	template<typename RandomAccessIterator, typename Comp>
	void grail_sort_buffer(RandomAccessIterator first, RandomAccessIterator last, Comp comp) {
		_internal::GrailSortWithBuffer(
			first, (int)std::distance(first, last), comperator<Comp>(std::move(comp)));
	}

	template<typename RandomAccessIterator, typename Comp>
	void grail_sort_dyn_buffer(RandomAccessIterator first, RandomAccessIterator last, Comp comp) {
		_internal::GrailSortWithDynBuffer(
			first, (int)std::distance(first, last), comperator<Comp>(std::move(comp)));
	}

	template<typename RandomAccessIterator, typename Comp>
	void rec_stable_sort(RandomAccessIterator first, RandomAccessIterator last, Comp comp) {
		_internal::RecStableSort(
			first, (int)std::distance(first, last), comperator<Comp>(std::move(comp)));
	}


	// Normal Comperator
	template<typename RandomAccessIterator>
	void grail_sort(RandomAccessIterator first, RandomAccessIterator last) {
		grail_sort(first, last, std::less<_internal::iter_value<RandomAccessIterator>>());
	}

	template<typename RandomAccessIterator>
	void grail_sort_buffer(RandomAccessIterator first, RandomAccessIterator last) {
		grail_sort_buffer(first, last, std::less<_internal::iter_value<RandomAccessIterator>>());
	}

	template<typename RandomAccessIterator>
	void grail_sort_dyn_buffer(RandomAccessIterator first, RandomAccessIterator last) {
		grail_sort_dyn_buffer(first, last, std::less<_internal::iter_value<RandomAccessIterator>>());
	}

	template<typename RandomAccessIterator>
	void rec_stable_sort(RandomAccessIterator first, RandomAccessIterator last) {
		rec_stable_sort(first, last, std::less<_internal::iter_value<RandomAccessIterator>>());
	}
}
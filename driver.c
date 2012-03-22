/*
 * Copyright (c) 2010 OKUMURA Yoshio <yoshio.okumura@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>  /* for printf */
#include <stdlib.h> /* calloc, free */
#include <string.h> /* for strcmp, sprintf */
#include <time.h>   /* for clock */
#include "thtbl.h"

/* prototypes */
static void *mycalloc(size_t nmemb, size_t size);
static void myfree(void *p);

static size_t hash_str(const void *value);
static size_t hash_alyways_1(const void *value);
static size_t hash_fnv1a(const void *value);
static size_t hash_zackw(const void *value);

static int cmp(const void *v1, const void *v2);
static void rem(void *value);

static time_t bench(thtbl_hash_f f, size_t len);

int main() {
	struct {
		thtbl_hash_f hash;
		const char *name;
	} benches[] = {
			{ hash_str,       "string" },
			{ hash_zackw,     "zackw" },
			{ hash_fnv1a,     "FNV-1a" },
			{ NULL, NULL }
	};

	size_t lengths[] = { 128, 1024, 8192, 16384, 32768, 65536, 1024 * 1024 };

	size_t i;
	for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
		time_t elapsed;
		size_t j;

		printf("%zd\n", lengths[i]);
		for (j = 0; benches[j].hash; j++) {
			printf("%s\n", benches[j].name);

			elapsed = bench(benches[j].hash, lengths[i]);

			printf("\t%f [sec]\n", (double) elapsed / (double) CLOCKS_PER_SEC);
		}
		printf("\n");
	}

	return 0;
}

/* implementations */
static void *mycalloc(size_t nmemb, size_t size) {
	return calloc(nmemb, size);
}

static void myfree(void *p) {
	free(p);
}

static size_t hash_str(const void *value) {
	const unsigned char *s = (const unsigned char *) value;
	size_t hash = 0;
	unsigned char c;

	while ((c = *s++)) {
        hash = (hash << 5) - hash + c;
	}
	return hash;
}

//#ifndef THTBL_LP64
#if 1
#define FNV1_32_INIT ((size_t) 0x811c9dc5)
#define FNV_32_PRIME ((size_t) 0x01000193)
/*
 * http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */
static size_t hash_fnv1a(const void *value) {
	const unsigned char *s = (const unsigned char *) value;
	size_t h = FNV1_32_INIT;
	unsigned char c;

	while ((c = *s++)) {
		h ^= c;
        h *= FNV_32_PRIME;
	}
	return h;
}
#else
#define FNV1_64_INIT ((size_t) 0xcbf29ce484222325ULL)
#define FNV_64_PRIME ((size_t) 0x100000001b3ULL)

static size_t hash_fnv1a(const void *value) {
	const unsigned char *s = (const unsigned char *) value;
	size_t h = FNV1_64_INIT;
	unsigned char c;

	while ((c = *s++)) {
		h ^= (size_t) c;
        h *= FNV_64_PRIME;
	}
	return h;
}
#endif

/*
 * http://gcc.gnu.org/ml/gcc-patches/2001-08/msg01021.html
 */
static size_t hash_zackw(const void *value) {
	const unsigned char *s = (const unsigned char *) value;
	size_t hash = 0;
	unsigned char c;

	while ((c = *s++)) {
        hash = hash * 67 + c - 113;
	}
	return hash;
}

static int cmp(const void *v1, const void *v2) {
	return strcmp(v1, v2);
}

static void rem(void *value) {
	myfree(value);
}

static time_t bench(thtbl_hash_f f, size_t len) {
	thtbl_e e;
	thtbl_t *table;
	time_t t1, t2;
	size_t i;
	char *buf;
	size_t collisions, searches;

	if ((e = thtbl_create(&table, len, f, cmp, rem, mycalloc, myfree))) {
		printf("thtbl_create() failed(%d).\n", e);
		return 0;
	} else {
		t1 = clock();
		for (i = 0; i < len * 2; i++) {
			buf = mycalloc(1, 256);
			sprintf(buf, "abcdefghijklmnopqrstuvwxyz_value_%010zd", i);
			e = thtbl_insert(table, buf);
		}

		e = thtbl_size(table, &i);

		thtbl_stat(table, &searches, &collisions);
		printf("\tsearches: %zd, collisions: %zd (%f%%)\n",
				searches, collisions,
				searches ? ((double) collisions / (double) searches) : 0.0);

		t2 = clock();

		thtbl_destroy(table);

		return t2 - t1;
	}
}

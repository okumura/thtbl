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

/*
 * thtbl is a simple hash table.
 */

#ifndef THTBL_H
#define THTBL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum thtbl_e {
	THTBL_EOK = 0,
	THTBL_EINVAL = 1,
	THTBL_ENOMEM = 2,
	THTBL_ENOENT = 3,
	THTBL_EEXIST = 4,
	THTBL_EFULL = 5
} thtbl_e;

/* TODO: consider works on kernel by THTBL_WORKS_ON_KERNEL */

#if defined(__LP64__) || defined(_WIN64)
	#define THTBL_LP64
#endif

/* define size_t for gcc, ICC. */
#if (defined(__GNUC__) || defined(__ICC)) && defined(__SIZE_TYPE__)
#ifndef _SIZE_T
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;
#endif
#endif

typedef size_t (*thtbl_hash_f)(const void *value);
typedef int (*thtbl_cmp_f)(const void *v1, const void *v2);
typedef void (*thtbl_rem_f)(void *value);
typedef void *(*thtbl_calloc_f)(size_t nmemb, size_t size);
typedef void (*thtbl_free_f)(void *p);
typedef int (*thtbl_enum_f)(void *value, void *aux);

typedef struct thtbl_t thtbl_t;

thtbl_e thtbl_create(thtbl_t **self, size_t limit, thtbl_hash_f hash,
		thtbl_cmp_f cmp, thtbl_rem_f rem, thtbl_calloc_f calloc,
		thtbl_free_f free);
thtbl_e thtbl_destroy(thtbl_t *self);
thtbl_e thtbl_size(thtbl_t *self, size_t *result);
thtbl_e thtbl_stat(thtbl_t *self, size_t *searches, size_t *collisions);
thtbl_e thtbl_clear(thtbl_t *self);
thtbl_e thtbl_insert(thtbl_t *self, const void *value);
thtbl_e thtbl_remove(thtbl_t *self, const void *value);
thtbl_e thtbl_find(thtbl_t *self, const void *value, const void **found);
thtbl_e thtbl_enum(thtbl_t *self, thtbl_enum_f cb, void *aux);

#ifdef __cplusplus
}
#endif

#endif /* THTBL_H */

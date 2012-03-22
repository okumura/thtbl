#include "thtbl.h"

#define EMPTY ((void *) (0))

#if defined(THTBL_LP64)
	#define DELETED ((void *) (0xffffffffffffffffLLU))
#else
	#define DELETED ((void *) (0xffffffff))
#endif

#define IS_USED(x) (x != EMPTY && x != DELETED)
#define IS_NOT_USED(x) (x == EMPTY || x == DELETED)
#define IS_EMPTY(x) (x == EMPTY)
#define IS_DELETED(x) (x == DELETED)

typedef void *slot_t;

struct thtbl_t {
	thtbl_hash_f hash;
	thtbl_cmp_f cmp;
	thtbl_rem_f rem;

	thtbl_calloc_f calloc;
	thtbl_free_f free;

	size_t limit_overhead;
	size_t limit;
	size_t used;
	size_t cols;
	size_t searches;

	slot_t *slots;
};

/* private def. */
static size_t calc_limit(size_t limit);
static void remove_values(thtbl_t *self);

/* public impl. */
thtbl_e thtbl_create(thtbl_t **self, size_t limit, thtbl_hash_f hash,
		thtbl_cmp_f cmp, thtbl_rem_f rem, thtbl_calloc_f calloc,
		thtbl_free_f free) {
	size_t limit_overhead;
	thtbl_t *tmp;
	size_t i;

	if (!self) {
		return THTBL_EINVAL;
	} else if (!limit) {
		return THTBL_EINVAL;
	} else if (!(limit_overhead = calc_limit(limit))) {
		return THTBL_EINVAL;
	} else if (!hash) {
		return THTBL_EINVAL;
	} else if (!cmp) {
		return THTBL_EINVAL;
	} else if (!calloc) {
		return THTBL_EINVAL;
	} else if (!free) {
		return THTBL_EINVAL;
	} else if (!(tmp = calloc(1, sizeof(thtbl_t)))) {
		return THTBL_ENOMEM;
	} else if (!(tmp->slots = calloc(limit_overhead, sizeof(slot_t)))) {
		free(tmp);
		return THTBL_ENOMEM;
	} else {
		tmp->hash = hash;
		tmp->cmp = cmp;
		tmp->rem = rem;
		tmp->calloc = calloc;
		tmp->free = free;
		tmp->limit_overhead = limit_overhead;
		tmp->limit = limit;
		tmp->used = 0;
		tmp->cols = 0;
		tmp->searches = 0;
		*self = tmp;

		return THTBL_EOK;
	}
}

thtbl_e thtbl_destroy(thtbl_t *self) {
	if (!self) {
		return THTBL_EINVAL;
	} else {
		remove_values(self);
		self->free(self->slots);
		self->free(self);

		return THTBL_EOK;
	}
}

thtbl_e thtbl_size(thtbl_t *self, size_t *size) {
	if (!self) {
		return THTBL_EINVAL;
	} else {
		*size = self->used;
		return THTBL_EOK;
	}
}

thtbl_e thtbl_stat(thtbl_t *self, size_t *searches, size_t *collisions) {
	if (!self) {
		return THTBL_EINVAL;
	} else {
		*searches = self->searches;
		*collisions = self->cols;
		return THTBL_EOK;
	}
}

thtbl_e thtbl_clear(thtbl_t *self) {
	size_t i;

	if (!self) {
		return THTBL_EINVAL;
	} else {
		remove_values(self);
		/* TODO: FIXME: free and calloc is faster? */
		for (i = 0; i < self->limit_overhead; i++) {
			self->slots[i] = EMPTY;
		}
		self->used = 0;

		return THTBL_EOK;
	}
}

thtbl_e thtbl_insert(thtbl_t *self, const void *value) {
	size_t hash;
	size_t index;
	slot_t *slot;
	size_t k;

	if (!self) {
		return THTBL_EINVAL;
	} else if (!value) {
		return THTBL_EINVAL;
	} else if (self->used >= self->limit) {
		return THTBL_ENOMEM;
	} else {
		/* open addressing by linear probing */
		self->searches++;
		hash = self->hash(value);
		for (k = 0; k < self->limit_overhead; k++, self->cols++) {
			index = (hash + k) & (self->limit_overhead - 1);
			slot = &self->slots[index];
			if (IS_NOT_USED(*slot)) {
				*slot = (void *) value;
				self->used++;
				return THTBL_EOK;
			} else  if (self->cmp(value, *slot) != 0) {
				/* hash collided but not same value. try next. */
			} else {
				return THTBL_EEXIST;
			}
		}
		/* hash table is filled. time to grow. */
		return THTBL_EFULL;
	}
}

thtbl_e thtbl_remove(thtbl_t *self, const void *value) {
	size_t hash;
	size_t index;
	slot_t *slot;
	size_t k;

	if (!self) {
		return THTBL_EINVAL;
	} else if (!value) {
		return THTBL_EINVAL;
	} else if (!self->used) {
		return THTBL_ENOENT;
	} else {
		/* open addressing by linear probing */
		self->searches++;
		hash = self->hash(value);
		for (k = 0; k < self->limit_overhead; k++, self->cols++) {
			index = (hash + k) & (self->limit_overhead - 1);
			slot = &self->slots[index];
			if (IS_NOT_USED(*slot)) {
				return THTBL_ENOENT;
			} else  if (self->cmp(value, *slot) != 0) {
				/* hash collided but not same value. try next. */
			} else {
				if (self->rem) {
					self->rem(*slot);
				}
				*slot = DELETED;
				self->used--;
				return THTBL_EOK;
			}
		}
		/* hash table is filled. time to grow. */
		return THTBL_EFULL;
	}
}

thtbl_e thtbl_find(thtbl_t *self, const void *value, const void **found){
	size_t hash;
	size_t index;
	slot_t *slot;
	size_t k;

	if (!self) {
		return THTBL_EINVAL;
	} else if (!value) {
		return THTBL_EINVAL;
	} else if (!self->used) {
		return THTBL_ENOENT;
	} else {
		/* open addressing by linear probing */
		self->searches++;
		hash = self->hash(value);
		for (k = 0; k < self->limit_overhead; k++, self->cols++) {
			index = (hash + k) & (self->limit_overhead - 1);
			slot = &self->slots[index];
			if (IS_EMPTY(*slot)) {
				return THTBL_ENOENT;
			} else if (IS_DELETED(*slot)) {
				/* skip the deleted slot. nothing to do. */
			} else if (self->cmp(value, *slot) != 0) {
				/* hash collided but not same value. try next. */
			} else {
				*found = *slot;
				return THTBL_EOK;
			}
		}
		/* hash table is filled. time to grow. */
		return THTBL_EFULL;
	}
}

thtbl_e thtbl_enum(thtbl_t *self, thtbl_enum_f cb, void *aux) {
	size_t i;
	slot_t *slot;

	if (!self) {
		return THTBL_EINVAL;
	} else if (!cb) {
		return THTBL_EINVAL;
	} else {
		for (i = 0, slot = self->slots; i < self->limit_overhead; i++, slot++) {
			if (IS_USED(*slot)) {
				if (cb(*slot, aux)) {
					break;
				}
			}
		}
		return THTBL_EOK;
	}
}

/* private impl. */
static size_t calc_limit(size_t limit) {
	size_t i, r;

#ifdef THTBL_LP64
	for (i = 4; i < 63; i++) {
		if ((r = 1 << i) > limit) {
			return r;
		}
	}
#else
	for (i = 4; i < 31; i++) {
		if ((r = 1 << i) > limit) {
			return r;
		}
	}
#endif

	return 0;
}

static void remove_values(thtbl_t *self) {
	size_t i;
	slot_t *s;

	if (self->rem) {
		/* TODO: FIXME: more faster! */
		for (i = 0, s = self->slots; i < self->limit_overhead; i++, s++) {
			if (IS_USED(*s)) {
				self->rem(*s);
			}
		}
	}
}

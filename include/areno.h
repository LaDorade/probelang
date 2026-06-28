/**
 *
 * Maty Ju, github.com/LaDorade
 * github.com/LaDorade/my_c_stdlib
 *
 * file raw: https://raw.githubusercontent.com/LaDorade/my_c_stdlib/refs/heads/main/areno.h
 * 
 */

#ifndef ARENO_H_
#define ARENO_H_

#ifndef  ARENO_ASSERT
#include <assert.h>
#define  ARENO_ASSERT assert
#endif //ARENO_ASSERT

#ifndef  ARENO_MALLOC
#define  ARENO_MALLOC malloc
#endif //ARENO_MALLOC

#ifndef  ARENO_FREE
#define  ARENO_FREE free
#endif //ARENO_FREE

#ifndef  ARENO_MEMSET
#include <string.h>
#define  ARENO_MEMSET memset
#endif //ARENO_MEMSET

#ifndef  ARENO_MEMCPY
#include <string.h>
#define  ARENO_MEMCPY memcpy
#endif //ARENO_MEMCPY

#ifndef  ARENO_CAPACITY
#define  ARENO_CAPACITY 1024*1024 // 1MB
#endif //ARENO_CAPACITY

typedef struct Areno Areno;
struct Areno {
	void*  start;
	Areno* next;
	size_t count;
};

void *areno_alloc  (Areno *areno, size_t size_in_byte);
void *areno_calloc (Areno *areno, size_t size_in_byte);
void *areno_realloc(Areno *areno, void *ptr, size_t old_size, size_t new_size);
void  areno_reset  (Areno *areno);
void  areno_free   (Areno *areno);
char *areno_printf (Areno *areno, const char *fmt, ...);

#define ARRAY_MIN_CAPACITY 128
#define areno_arr_push(areno, arr, item) do {             \
    if ((arr)->count >= (arr)->capacity) {                \
        size_t old_cap = (arr)->capacity;                 \
        if ((arr)->count <= 0)                            \
            (arr)->capacity = ARRAY_MIN_CAPACITY;         \
        else                                              \
            (arr)->capacity *= 2;                         \
        (arr)->items = areno_realloc((areno),             \
                (arr)->items,                             \
                old_cap,                                  \
                sizeof(*(arr)->items) * (arr)->capacity); \
    }                                                     \
    (arr)->items[(arr)->count++] = (item);                \
} while (0);

#endif // ARENO_H_

#ifdef ARENO_IMPLEMENTATION

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef ARENO_DEBUG_INFO
#include <stdio.h>
void ARENO_DEBUG(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	// print message on format "[ARENO_DEBUG] blablabla"
	printf("[ARENO-DEBUG] ");
	vprintf(fmt, args);
	fflush(stdout);
	va_end(args);
}
#else
void ARENO_DEBUG(const char *fmt, ...)
{
	(void)fmt; // let the compile optimize the code
}
#endif //ARENO_DEBUG_INFO

void *areno_alloc(Areno* areno, size_t size_in_byte)
{
	ARENO_ASSERT(size_in_byte < ARENO_CAPACITY && "Requested more than one Areno capcity");

	if (areno->start == NULL) { // initial alloc
		areno->start = ARENO_MALLOC(ARENO_CAPACITY);
		ARENO_ASSERT(areno->start != NULL);
	}
	
	size_t alignment = ((areno->count + 15) & ~15) - areno->count;

	if (areno->count + alignment + size_in_byte >= ARENO_CAPACITY) { // not enough place in this areno
		if (areno->next == NULL) {
			ARENO_DEBUG("Region %p full, creating new one\n", areno);
			areno->next = (Areno *)ARENO_MALLOC(sizeof(Areno));
			ARENO_ASSERT(areno->next != NULL);

			*areno->next = (Areno) {0};
		}
		return areno_alloc(areno->next, size_in_byte);
	}

	areno->count += alignment;
	void *alloc = (char*)areno->start + areno->count;
	areno->count += size_in_byte;

	ARENO_DEBUG("Allocating %zu bytes on region %p. Count is now %zu\n", size_in_byte, areno, areno->count);

	return alloc;
}

void *areno_calloc(Areno* areno, size_t size_in_byte)
{
    void *alloc = areno_alloc(areno, size_in_byte);
    return ARENO_MEMSET(alloc, 0, size_in_byte);
}

void *areno_realloc(Areno *areno, void *ptr, size_t old_size, size_t new_size)
{
    if (new_size <= old_size) return ptr;
    // TODO: maybe check in the future if this is the last alloc, to optimize space
    void *alloc = areno_alloc(areno, new_size);
    return ARENO_MEMCPY(alloc, ptr, old_size);
}

void areno_free(Areno* areno)
{
	if (areno->start != NULL) {
		ARENO_FREE(areno->start);
		areno->start = NULL;
	}

	Areno *current = areno->next;
	while (current != NULL) {
		Areno *to_free = current;
		current = current->next;

		if (to_free->start != NULL) {
			ARENO_FREE(to_free->start);
		}
		ARENO_FREE(to_free);
	}

	areno->next  = NULL;
	areno->count = 0;
}

void areno_reset(Areno* areno)
{
	areno->count = 0;
	for (Areno* ar = areno->next; ar != NULL; ar = ar->next)
	{
		ar->count = 0;
	}
}

char *areno_printf(Areno* areno, const char *fmt, ...)
{
    va_list args;
    va_list args2;

    va_start(args, fmt);
    va_copy(args2, args);
    va_start(args2, fmt);

    int size  = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (size < 0) {
        va_end(args2);
        return NULL;
    }

    char *str = areno_alloc(areno, size);
    vsnprintf(str, size + 1, fmt, args2);

    va_end(args2);

    return str;
}
#endif // ARENO_IMPLEMENTATION


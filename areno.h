/**
 *
 * Maty Ju, github.com/LaDorade
 * github.com/LaDorade/my_c_stdlib
 *
 * file raw: https://raw.githubusercontent.com/LaDorade/my_c_stdlib/refs/heads/main/areno.h
 * 
 */

#ifndef __ARENO_H_
#define __ARENO_H_

#include <stdlib.h>

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

#ifndef  ARENO_CAPACITY
#define  ARENO_CAPACITY 1024*1024 // 1M
#endif //ARENO_CAPACITY

typedef struct Areno Areno;
typedef struct Areno {
	void*  start;
	Areno* next;
	size_t count;
} Areno;

void *areno_alloc(Areno* areno, size_t size_in_byte);
void  areno_reset(Areno* areno);
void  areno_free (Areno* areno);

#endif // __ARENO_H_

#ifdef ARENO_IMPLEMENTATION

#ifdef   ARENO_DEBUG_INFO
#include <stdio.h>
#include <stdarg.h>
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
	for (Areno* ar = ar->next; ar != NULL; ar = ar->next)
	{
		ar->count = 0;
	}
}

#endif // ARENO_IMPLEMENTATION

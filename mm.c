/*
* mm-naive.c - The fastest, least memory-efficient malloc package.
*
* In this naive approach, a block is allocated by simply incrementing
* the brk pointer.  A block is pure payload. There are no headers or
* footers.  Blocks are never coalesced or reused. Realloc is
* implemented directly using mm_malloc and mm_free.
*
* NOTE TO STUDENTS: Replace this header comment with your own header
* comment that gives a high level description of your solution.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
* NOTE TO STUDENTS: Before you do anything else, please
* provide your team information in the following struct.
********************************************************/
team_t team = {
	/* Team name */
	"ateam",
	/* First member's full name */
	"Harry Bovik",
	/* First member's email address */
	"bovik@cs.cmu.edu",
	/* Second member's full name (leave blank if none) */
	"",
	/* Second member's email address (leave blank if none) */
	""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WORDSIZE 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define INFO_SIZE 32
#define PUT(pointer,value) (*(long*)pointer = value) //Put value at address that pointer holds
#define GETPNEXT(pointer) (*((char*)pointer+WORDSIZE))
#define GETPPREV(pointer) (*((char*)pointer+2*WORDSIZE))
#define GETSIZE(pointer) (*((long*)pointer))
#define GETPAYLOADPTR(pointer) ((char*)pointer+WORDSIZE)
/*void pointers are not allowed to perform arithmetic operation, this MACRO will transform*/
#define CAST_TO_BYTE_PTR(pointer) ((char*)pointer)
/*Use best fit policy when free list has little of free node*/
void* find_best_fit(void* free_pHead, int rounded_size);
/*Use first fit policy when free list has plenty of free node*/
void* find_first_fit(void* free_pHead, int rounded_size);
/*Allocate more heap when there are no more space in the free list*/
void* allocateMoreHeap(size_t rounded_size);
/*Join prev blocks and next blocks with current blocks when free memory and fix link.
The argument is the pointer to the HEADER, and return the pointer to the HEADER of new coalescing block
*/
void* join(void*pointer);
void* free_pHead;
/*
* mm_init - initialize the malloc package.
*/
int mm_init(void)
{
	/*Initialize free list pointer with SPECIAL BLOCK indicates the signal of the end of the list
	With HEADER = FOOTER = PNEXT = PREV = 0
	*/
	free_pHead = mem_sbrk(ALIGN(INFO_SIZE));
	PUT(CAST_TO_BYTE_PTR(free_pHead), 0x0);
	PUT(CAST_TO_BYTE_PTR(free_pHead) + WORDSIZE, 0x0); //pnext = null
	PUT(CAST_TO_BYTE_PTR(free_pHead) + 2 * WORDSIZE, 0x0); //pprev = null
	PUT(CAST_TO_BYTE_PTR(free_pHead) + 3 * WORDSIZE, 0x0);
	return 0;
}

/*
* mm_malloc - Allocate a block by incrementing the brk pointer.
*     Always allocate a block whose size is a multiple of the alignment.
*/
void *mm_malloc(size_t size)
{
	int newsize = ALIGN(size);
	void* p = find_first_fit(free_pHead, newsize);
	if (p != NULL)
		return p;
	return allocateMoreHeap(newsize);
}

/*
* mm_free - Freeing a block does nothing.
*/
void mm_free(void *ptr)
{
	void* ptr = CAST_TO_BYTE_POINTER(ptr)-WORDSIZE;
	ptr = join(ptr);
	push_front(ptr);
}

/*
* mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/
void *mm_realloc(void *ptr, size_t size)
{
	void *oldptr = ptr;
	void *newptr;
	size_t copySize;

	newptr = mm_malloc(size);
	if (newptr == NULL)
		return NULL;
	copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
	if (size < copySize)
		copySize = size;
	memcpy(newptr, oldptr, copySize);
	mm_free(oldptr);
	return newptr;
}
/*This is first fit placement policy returning the pointer to the payload*/
void* find_first_fit(void*free_pHead, int rounded_size)
	{
	int complete_size = rounded_size + INFO_SIZE;
	void* traverse_ptr = free_pHead;
	while (*traverse_ptr != 0x0)
	{
		if (GETSIZE(traverse_ptr) >= complete_size)
		{
			PUT(traverse_ptr,COMBINE(complete_size,1));
			PUT(CAST_TO_BYTE_PTR(traverse_ptr)+WORDSIZE+rounded_size,COMBINE(complete_size,1));
			return GETPAYLOADPTR(traverse_ptr);
		}
			
		else
		{
			traverse_ptr = GETPNEXT(traverse_ptr);
		}
	}
	return NULL;// return null pointer when there is no appropriate block
}
/*This is best fit policy placement policy returning the pointer to the payload*/
void* find_best_fit(void *free_pHead, int rounded_size)
{
	int complete_size = rounded_size + INFO_SIZE;
	int fit_size = -1;
	void* result_ptr = NULL;
	void* traverse_ptr = free_pHead;
	while (*traverse_ptr != 0x0)
	{
		if (GETSIZE(traverse_ptr) >= complete_size)
		{
			if (fit_size == -1)
			{
				fit_size = GETSIZE(traverse_ptr);
				result_ptr = traverse_ptr;
			}
			else if (GETSIZE(traverse_ptr) < fit_size)
			{
				fit_size = GETSIZE(traverse_ptr);
				result_ptr = traverse_ptr;
			}
		}
		traverse_ptr = GETPNEXT(traverse_ptr);
	}
	if (result_ptr == NULL)
		return NULL;
	PUT(result_ptr,COMBINE(complete_size,1));
	PUT(CAST_TO_BYTE_PTR(result_ptr)+WORDSIZE+rounded_size,COMBINE(complete_size,1));
	return GETPAYLOADPTR(result_ptr);
}
void* allocateMoreHeap(size_t rounded_size)
{
	size_t complete_size = rounded_size + INFO_SIZE;
	void* new_heap_ptr = mem_sbrk(complete_size);
	PUT(new_heap_ptr, COMBINE(complete_size, 1));
	PUT(CAST_TO_BYTE_PTR(free_pHead) + WORDSIZE + rounded_size, COMBINE(complete_size, 1));
	return GETPAYLOADPTR(new_heap_ptr);
}
void* join(void*pointer)
{
	/*Check if the next block is free, coalescene and fix link*/
	void* next_header_ptr = CAST_TO_BYTE_POINTER(pointer) + GETSIZE(pointer);
	int isallocated = GETSIZE(next_header_ptr) & 1;
	if(isallocated == 0)
	{
		void* tmp_ptr = *(CAST_TO_BYTE_POINTER(next_header_ptr) + WORDSIZE);//get pnext of next_block
		void* tmp_ptr_1 = *(CAST_TO_BYTE_POINTER(next_header_ptr)+2*WORDSIZE);//get prev of next_block
		void* tmp_ptr_2 = *(CAST_TO_BYTE_POINTER(tmp_ptr_1)+WORDSIZE); //get pnext of prev of next_block
		void* tmp_ptr_3 = *(CAST_TO_BYTE_POINTER(tmp_ptr)+2*WORDSIZE); //get prev of pnext of next_block
		tmp_ptr_2 = *(CAST_TO_BYTE_POINTER(next_header_ptr)+WORDSIZE); //pnext of prev of next_block = current block->pnext
		tmp_ptr_3 = *(CAST_TO_BYTE_POINTER(next_header_ptr)+2*WORDSIZE); //pprev of pnext of next_block = current block->prev
		void* next_footer_ptr = CAST_TO_BYTE_POINTER(next_header_ptr)+GETSIZE(next_header_ptr)-WORDSIZE;
		int newsize = GETSIZE(pointer)+GETSIZE(next_header_ptr);
		PUT(pointer,COMBINE(newsize,0));
		PUT(next_footer_ptr,COMBINE(newsize,0));
	}
	/*Check if prev block is free,coalescene and fix link*/
	void* prev_footer_ptr = CAST_TO_BYTE_POINTER(pointer)-WORDSIZE;
	isallocated = GETSIZE(prev_footer_ptr) & 1;
	void* prev_header_ptr = CAST_TO_BYTE_POINTER(prev_footer_ptr) - (GETSIZE(prev_footer_ptr) - WORDSIZE);
	if(isallocated == 0)
	{
		void *tmp_ptr = *(CAST_TO_BYTE_POINTER(prev_header_ptr)+WORDSIZE); //get pnext of prev block
		void* tmp_ptr_1 = *(CAST_TO_BYTE_POINTER(prev_header_ptr)+2*WORDSIZE); //get pprev of prev block
		void* tmp_ptr_2 = *(CAST_TO_BYTE_POINTER(tmp_ptr_1)+WORDSIZE);
		void* tmp_ptr_3 = *(CAST_TO_BYTE_POINTER(tmp_ptr)+2*WORDSIZE);
		tmp_ptr_2 = *(CAST_TO_BYTE_POINTER(prev_header_ptr)+WORDSIZE);
		tmp_ptr_3 = *(CAST_TO_BYTE_POINTER(prev_header_ptr)+2*WORDSIZE);
		int newsize = GETSIZE(pointer)+GETSIZE(prev_header_ptr);
		PUT(prev_header_ptr,COMBINE(newsize,0));
		PUT(CAST_TO_BYTE_POINTER(prev_header_ptr)+newsize - WORDSIZE,COMBINE(newsize,0));
		pointer = prev_header_ptr;
	}
	return pointer;
}
void push_front(void* pointer)
{
	PUT(CAST_TO_BYTE_POINTER(pointer) + WORDSIZE,free_pHead);
	PUT(CAST_TO_BYTE_POINTER(pointer) + 2*WORDSIZE,0x0);
	free_pHead = pointer;
}











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
	"LAM HOANG DUNG",
	/* First member's email address */
	"bovik@cs.cmu.edu",
	/* Second member's full name (leave blank if none) */
	"LIEU HUY CHANH",
	/* Second member's email address (leave blank if none) */
	"lhc@gmail.com"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define BYTE8 8
#define BYTE4 4
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define INFO_SIZE 28
#define PUTSIZE(pointer,value) ((*(size_t*)(pointer)) = value) //Put value at address that pointer holds
#define PUTPTR(pointer,another_ptr)  ((*(long*)(pointer)) = another_ptr)
#define GETPNEXT(pointer) (*(long*)((void*)(pointer)+BYTE8))
#define GETPPREV(pointer) (*(long*)((void*)(pointer)+2*BYTE8))
#define GETSIZE(pointer) (*(size_t*)(pointer) & (size_t)~0x7)
#define GETSIZE_ALLOCATED(pointer) (*(size_t*)(pointer))
#define GETPAYLOADPTR(pointer) ((void*)(pointer)+BYTE8)
#define COMBINE(value,isallocated) ((value) | (isallocated))
#define DEALLOCATE(value) ((value) & ~0x7)
/*void pointers are not allowed to perform arithmetic operation, this MACRO will transform*/
/*Use best fit policy when free list has little of free node*/
void* find_best_fit(void* free_pHead, int rounded_size);
/*Use first fit policy when free list has plenty of free node*/
void* find_first_fit(int rounded_size);
/*Allocate more heap when there are no more space in the free list*/
void* allocateMoreHeap(size_t rounded_size);
/*Join prev blocks and next blocks with current blocks when free memory and fix link.
The argument is the pointer to the HEADER, and return the pointer to the HEADER of new coalescing block
*/
void* join(void*pointer);
void* join_next(void*pointer);
void* join_prev(void* pointer);
void remove_(void* pointer);
void* fix_link(void* pointer);
void push_front(void* pointer);

void print_list();
void* free_pHead;
void print_list(){
	void* traverse_ptr = free_pHead;
	int count = 0;
	while(traverse_ptr != NULL)
	{

		printf("COUNT: %u SIZE: %u POINTER: %p\n",count,GETSIZE(traverse_ptr),traverse_ptr);
		traverse_ptr =  GETPNEXT(traverse_ptr);
	}
}
/*
* mm_init - initialize the malloc package.
*/
int mm_init(void)
{
	/*Initialize free list pointer with SPECIAL BLOCK indicates the signal of the end of the list
	With HEADER = 0
	*/
	free_pHead = 0x0;
	return 0;
}

/*
* mm_malloc - Allocate a block by incrementing the brk pointer.
*     Always allocate a block whose size is a multiple of the alignment.
*/
void *mm_malloc(size_t size)
{
	int newsize = ALIGN(size+INFO_SIZE);
	void* p = find_first_fit(newsize);
	if (p != NULL)
	{
		remove_(p);
		return GETPAYLOADPTR(p);
	}
	void* ptr = allocateMoreHeap(newsize);
	if(ptr == NULL)
		return NULL;
	return GETPAYLOADPTR(ptr);
}

/*
* mm_free - Freeing a block does nothing.
*/
void mm_free(void *ptr)
{
	if(ptr == NULL)
		return;
	ptr = ptr - BYTE8;
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
void* find_first_fit(int rounded_size)
{
	void* traverse_ptr = free_pHead;
	while (traverse_ptr != 0x0)
	{
		
		if (GETSIZE(traverse_ptr) >= rounded_size)
		{
			int old_size = GETSIZE(traverse_ptr);
			if(old_size - rounded_size >= INFO_SIZE)
			{
				int split_size = old_size - rounded_size;
				void* split_ptr = traverse_ptr + rounded_size;
				PUTSIZE(split_ptr,DEALLOCATE(split_size));//PUT SIZE TO HEADER
				PUTSIZE(split_ptr+split_size-BYTE4,DEALLOCATE(split_size));//put size to footer
				PUTPTR(split_ptr+BYTE8,GETPNEXT(traverse_ptr)); //split->pnext = traverse->pnext
				PUTPTR(split_ptr+2*BYTE8,GETPPREV(traverse_ptr)); //split->pprev = traverse->pprev
				if(GETPPREV(split_ptr)!=NULL)
				{
					PUTPTR(GETPNEXT(GETPPREV(split_ptr)),split_ptr);
				}
				if(GETPNEXT(split_ptr)!=NULL)
				{
					PUTPTR(GETPPREV(GETPNEXT(split_ptr)),split_ptr);
				}
				if(GETPPREV(split_ptr) == NULL)
				{
					free_pHead = split_ptr;
				}
		
				PUTSIZE(traverse_ptr,COMBINE(rounded_size,1));
				PUTSIZE(traverse_ptr+rounded_size - BYTE4,COMBINE(rounded_size,1));
				return traverse_ptr;
			}
			else
			{
				PUTSIZE(traverse_ptr,COMBINE(old_size,1));
				PUTSIZE(traverse_ptr+old_size - BYTE4,COMBINE(old_size,1));
			}
			return traverse_ptr;
		}
		else
		{
			traverse_ptr = GETPNEXT(traverse_ptr);
		}
	}
	return NULL;// return null pointer when there is no appropriate block
}
/*This is best fit policy placement policy returning the pointer to the payload*/

void* allocateMoreHeap(size_t rounded_size)
{
	void* new_heap_ptr = mem_sbrk(rounded_size);
	PUTSIZE(new_heap_ptr, COMBINE(rounded_size, 1));
	PUTSIZE(new_heap_ptr +rounded_size-BYTE4, COMBINE(rounded_size, 1));
	return new_heap_ptr;
}
void* join(void*pointer)
{
	int size = GETSIZE(pointer);
	PUTSIZE(pointer,DEALLOCATE(size));
	PUTSIZE(pointer+size-BYTE4,DEALLOCATE(size));
	pointer = join_next(pointer);
	pointer = join_prev(pointer);
	return pointer;
}
void* join_next(void*pointer)
{
	void* next_block_ptr = pointer + GETSIZE(pointer);
	if(next_block_ptr > mem_heap_hi())
		return pointer;	
	int isallocated = GETSIZE_ALLOCATED(next_block_ptr) & 1;
	if(isallocated == 0)
	{
		
		next_block_ptr = fix_link(next_block_ptr);
		int newsize = GETSIZE(pointer) + GETSIZE(next_block_ptr);
		PUTSIZE(pointer,DEALLOCATE(newsize));
		PUTSIZE(pointer+newsize-BYTE4,DEALLOCATE(newsize));
	}
	return pointer;
}
void* join_prev(void* pointer)
{
	void *prev_block_ptr = pointer - BYTE4;
	if (prev_block_ptr < mem_heap_lo())
		return pointer;
	int isallocated = GETSIZE_ALLOCATED(prev_block_ptr) & 1;
	if(isallocated == 0)
	{
		int prev_block_size = GETSIZE(prev_block_ptr);
		prev_block_ptr = pointer - prev_block_size;
		prev_block_ptr = fix_link(prev_block_ptr);
		int newsize = GETSIZE(pointer) + GETSIZE(prev_block_ptr);
		PUTSIZE(prev_block_ptr,DEALLOCATE(newsize));
		PUTSIZE(prev_block_ptr+newsize-BYTE4,DEALLOCATE(newsize));
		return prev_block_ptr;
	}
	return pointer;

}
void push_front(void* pointer)
{
	if(pointer != free_pHead)
	{
		PUTPTR(pointer + BYTE8,free_pHead);
		PUTPTR(pointer + 2*BYTE8,0x0);
		if(free_pHead!=NULL)
		{
			void* ptr_1 = free_pHead + 2*BYTE8;
			PUTPTR(ptr_1,pointer);
		}
		free_pHead = pointer;
	}

}
void remove_(void* pointer)
{
	pointer = fix_link(pointer);
}
void* fix_link(void* pointer)
{
	if(GETPNEXT(pointer) != NULL)
	{
		void* tmp = GETPNEXT(pointer)+2*BYTE8;
		PUTPTR(tmp,pointer);
	}
	if(GETPPREV(pointer) != NULL)
	{
		void* tmp = GETPPREV(pointer)+BYTE8;
		PUTPTR(tmp,pointer);
	}
	return pointer;
}
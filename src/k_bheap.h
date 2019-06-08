#ifndef __K_BHEAP__
#define __K_BHEAP__

#include "doomdef.h"

typedef struct bheapitem_s
{
	size_t  heapindex; // The index in the heap this item is
	bheap_t *owner;    // The heap that owns this item
	void    *data;     // data for this heap item
	UINT32  value;     // The value of this item, the lowest value item is first in the array
} bheapitem_t;

typedef struct bheap_s
{
	size_t      capacity;   // capacity of the heap
	size_t      count;      // number of items in the heap
	bheapitem_t *array;     // pointer to the heap items array
} bheap_t;


/*--------------------------------------------------
	boolean K_BHeapInit(bheap_t *const heap, size_t initialcapacity)

		Initialises a binary heap.

	Input Arguments:-
		heap            - The heap to initialise
		initialcapacity - The initial capacity the heap should hold

	Return:-
		True if the initialisation was successful, false if it wasn't.
--------------------------------------------------*/

boolean K_BHeapInit(bheap_t *const heap, size_t initialcapacity);


/*--------------------------------------------------
	boolean K_BHeapValid(bheap_t *const heap)

		Checks a binary heap for validity

	Input Arguments:-
		heap - The heap to validate

	Return:-
		True if the binary heap is valid, false if it isn't
--------------------------------------------------*/

boolean K_BHeapValid(bheap_t *const heap);


/*--------------------------------------------------
	boolean K_BHeapPush(bheap_t *const heap, void *const item, const UINT32 value)

		Adds a new item to a binary heap.

	Input Arguments:-
		heap  - The heap to add to.
		item  - The item to add to the heap.
		value - The value of this item for the heap, lowest is first in the heap

	Return:-
		True if the push to the heap was successful, false if it wasn't due to invalid parameters
--------------------------------------------------*/

boolean K_BHeapPush(bheap_t *const heap, void *const item, UINT32 value);


/*--------------------------------------------------
	boolean K_BHeapPop(bheap_t *const heap, bheapitem_t *const returnitemstorage)

		Pops the first item off of the heap, then orders it back to be correct.

	Input Arguments:-
		heap              - The heap to pop from.
		returnitemstorage - The first item on the Heap is placed in here

	Return:-
		true if the pop from the heap was successful, false if it wasn't.
--------------------------------------------------*/

boolean K_BHeapPop(bheap_t *const heap, bheapitem_t *const returnitemstorage);


/*--------------------------------------------------
	boolean K_UpdateHeapItemValue(bheapitem_t *const item, const UINT32 newvalue)

		Updates the heap item's value, and reorders it in the array appropriately. Only works if the item is in a heap
		validly. If it's a heapitem that is not currently in a heap (ie it's been popped off) just change the value
		manually.

	Input Arguments:-
		item     - The item to update the value of.
		newvalue - The new value the item will hold

	Return:-
		true if the update was successful, false if it wasn't
--------------------------------------------------*/

boolean K_UpdateHeapItemValue(bheapitem_t *const item, const UINT32 newvalue);

#endif

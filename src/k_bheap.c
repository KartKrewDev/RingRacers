#include "k_bheap.h"

#include "z_zone.h"

/*--------------------------------------------------
	static boolean K_BHeapItemValidate(bheap_t *heap, bheapitem_t *item)

		Validates an item on a heap to ensure it is correct and on that heap.

	Input Arguments:-
		heap - The heap to validate the item with
		item - The item to validate

	Return:-
		True if the item is valid, false if it isn't.
--------------------------------------------------*/
static boolean K_BHeapItemValidate(bheap_t *heap, bheapitem_t *item)
{
	boolean heapitemvalid = false;

	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapItemValidate.\n");
	}
	else if (item == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item in K_BHeapItemValidate.\n");
	}
	else
	{
		if ((item->data != NULL) && (item->heapindex < SIZE_MAX / 2) && (item->owner == heap))
		{
			heapitemvalid = true;
		}
	}

	return heapitemvalid;
}

/*--------------------------------------------------
	static bheapitem_t *K_BHeapItemsCompare(bheap_t *heap, bheapitem_t *item1, bheapitem_t *item2)

		Compares 2 items in the heap to find the better (lower) value one.

	Input Arguments:-
		heap  - The heap to compare items
		item1 - The first item to compare
		item2 - The second item to compare

	Return:-
		The item out of the 2 sent in that has the better value, returns item2 if they are identical
--------------------------------------------------*/
static bheapitem_t *K_BHeapItemsCompare(bheap_t *heap, bheapitem_t *item1, bheapitem_t *item2)
{
	bheapitem_t *lowervalueitem = NULL;
	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapItemsCompare.\n");
	}
	else if (K_BHeapValid(heap) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid heap in K_BHeapSwapItems.\n");
	}
	else if (item1 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item1 in K_BHeapItemsCompare.\n");
	}
	else if (item2 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item2 in K_BHeapItemsCompare.\n");
	}
	else if (K_BHeapItemValidate(heap, item1) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid item1 in K_BHeapItemsCompare.\n");
	}
	else if (K_BHeapItemValidate(heap, item2) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid item2 in K_BHeapItemsCompare.\n");
	}
	else
	{
		if (item1->value < item2->value)
		{
			lowervalueitem = item1;
		}
		else
		{
			lowervalueitem = item2;
		}
	}

	return lowervalueitem;
}

/*--------------------------------------------------
	static void K_BHeapSwapItems(bheap_t *heap, bheapitem_t *item1, bheapitem_t *item2)

		Swaps 2 items in the heap

	Input Arguments:-
		heap  - The heap to swap items in
		item1 - The first item to swap in the heap
		item2 - The second item to swap in the heap

	Return:-
		None
--------------------------------------------------*/
static void K_BHeapSwapItems(bheap_t *heap, bheapitem_t *item1, bheapitem_t *item2)
{
	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapSwapItems.\n");
	}
	else if (K_BHeapValid(heap) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid heap in K_BHeapSwapItems.\n");
	}
	else if (item1 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item1 in K_BHeapSwapItems.\n");
	}
	else if (item2 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item2 in K_BHeapSwapItems.\n");
	}
	else if (K_BHeapItemValidate(heap, item1) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid item1 in K_BHeapSwapItems.\n");
	}
	else if (K_BHeapItemValidate(heap, item2) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid item2 in K_BHeapSwapItems.\n");
	}
	else
	{
		size_t      tempitemindex = item1->heapindex;
		bheapitem_t tempitemstore = *item1;

		// Swap the items fully with each other
		*item1 = *item2;
		*item2 = tempitemstore;

		// Swap the heap index on each item to be correct
		item2->heapindex = item1->heapindex;
		item1->heapindex = tempitemindex;
	}
}

/*--------------------------------------------------
	static size_t K_BHeapItemGetParentIndex(bheapitem_t *item)

		Gets the parent index of a heap item

	Input Arguments:-
		item - The item to get the parent index of

	Return:-
		The parent index of the item
--------------------------------------------------*/
static size_t K_BHeapItemGetParentIndex(bheapitem_t *item)
{
	size_t parentindex = SIZE_MAX;

	if (item == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item in K_BHeapItemGetParentIndex.\n");
	}
	else if (item->heapindex >= (SIZE_MAX / 2))
	{
		CONS_Debug(DBG_GAMELOGIC, "Bad item heapindex in K_BHeapItemGetParentIndex.\n");
	}
	else
	{
		parentindex = (item->heapindex - 1U) / 2U;
	}

	return parentindex;
}

/*--------------------------------------------------
	static size_t K_BHeapItemGetLeftChildIndex(bheapitem_t *item)

		Gets the left child index of a heap item

	Input Arguments:-
		item - The item to get the left child index of

	Return:-
		The left child index of the item
--------------------------------------------------*/
static size_t K_BHeapItemGetLeftChildIndex(bheapitem_t *item)
{
	size_t leftchildindex = SIZE_MAX;

	if (item == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item in K_BHeapItemGetLeftChildIndex.\n");
	}
	else if (item->heapindex >= (SIZE_MAX / 2))
	{
		CONS_Debug(DBG_GAMELOGIC, "Bad item heapindex in K_BHeapItemGetLeftChildIndex.\n");
	}
	else
	{
		leftchildindex = (item->heapindex * 2U) + 1U;
	}

	return leftchildindex;
}

/*--------------------------------------------------
	static size_t K_BHeapItemGetRightChildIndex(bheapitem_t *item)

		Gets the right child index of a heap item

	Input Arguments:-
		item - The item to get the right child index of

	Return:-
		The right child index of the item
--------------------------------------------------*/
static size_t K_BHeapItemGetRightChildIndex(bheapitem_t *item)
{
	size_t rightchildindex = SIZE_MAX;

	if (item == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item in K_BHeapItemGetRightChildIndex.\n");
	}
	else if (item->heapindex >= (SIZE_MAX / 2))
	{
		CONS_Debug(DBG_GAMELOGIC, "Bad item heapindex in K_BHeapItemGetRightChildIndex.\n");
	}
	else
	{
		rightchildindex = (item->heapindex * 2U) + 2U;
	}

	return rightchildindex;
}

/*--------------------------------------------------
	static void K_BHeapSortUp(bheap_t *heap, bheapitem_t *item)

		Sorts a heapitem up the list to its correct index, lower value items are higher up

	Input Arguments:-
		heap - The heap to sort the item up.
		item - The item to sort up the heap

	Return:-
		None
--------------------------------------------------*/
static void K_BHeapSortUp(bheap_t *heap, bheapitem_t *item)
{
	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapSortUp.\n");
	}
	else if (K_BHeapValid(heap) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid heap in K_BHeapSortUp.\n");
	}
	else if (item == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item in K_BHeapSortUp.\n");
	}
	else
	{
		if (item->heapindex > 0U)
		{
			size_t parentindex = SIZE_MAX;
			do
			{
				parentindex = K_BHeapItemGetParentIndex(item);

				// Swap the nodes if the parent has a higher value
				if (K_BHeapItemsCompare(heap, item, &heap->array[parentindex]) == item)
				{
					K_BHeapSwapItems(heap, item, &heap->array[parentindex]);
				}
				else
				{
					break;
				}
			} while (parentindex > 0U);
		}
	}
}

/*--------------------------------------------------
	static void K_BHeapSortDown(bheap_t *heap, bheapitem_t *item)

		Sorts a heapitem down the list to its correct index, higher value items are further down

	Input Arguments:-
		heap - The heap to sort the item down.
		item - The item to sort down the heap

	Return:-
		None
--------------------------------------------------*/
static void K_BHeapSortDown(bheap_t *heap, bheapitem_t *item)
{
	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapSortDown.\n");
	}
	else if (K_BHeapValid(heap) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid heap in K_BHeapSortDown.\n");
	}
	else if (item == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item in K_BHeapSortDown.\n");
	}
	else
	{
		if (heap->count > 0U)
		{
			size_t      leftchildindex  = SIZE_MAX;
			size_t      rightchildindex = SIZE_MAX;
			bheapitem_t *leftchild      = NULL;
			bheapitem_t *rightchild     = NULL;
			bheapitem_t *swapchild      = NULL;
			boolean     noswapneeded    = false;

			do
			{
				leftchildindex  = K_BHeapItemGetLeftChildIndex(item);
				rightchildindex = K_BHeapItemGetRightChildIndex(item);

				if (leftchildindex < heap->count)
				{
					leftchild = &heap->array[leftchildindex];
					swapchild = leftchild;
					if (rightchildindex < heap->count)
					{
						rightchild = &heap->array[rightchildindex];
						// Choose the lower child node to swap with
						if (K_BHeapItemsCompare(heap, leftchild, rightchild) == rightchild)
						{
							swapchild = rightchild;
						}
					}

					// Swap with the lower child, if it's lower than item
					if (K_BHeapItemsCompare(heap, swapchild, item) == swapchild)
					{
						K_BHeapSwapItems(heap, item, swapchild);
					}
					else
					{
						noswapneeded = true;
					}

				}
				else
				{
					noswapneeded = true;
				}

				if (noswapneeded)
				{
					break;
				}

			} while (item->heapindex < (heap->count - 1U));
		}
	}
}

/*--------------------------------------------------
	boolean K_BHeapInit(bheap_t *const heap, size_t initialcapacity)

		See header file for description.
--------------------------------------------------*/
boolean K_BHeapInit(bheap_t *const heap, size_t initialcapacity)
{
	boolean initsuccess = false;

	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapInit.\n");
	}
	else if (initialcapacity == 0U)
	{
		CONS_Debug(DBG_GAMELOGIC, "initialcapacity is 0 in K_BHeapInit.\n");
	}
	else
	{
		heap->array = Z_Calloc(initialcapacity * sizeof(bheapitem_t), PU_STATIC, NULL);

		if (heap->array == NULL)
		{
			I_Error("K_BHeapInit: Out of Memory.");
		}

		heap->capacity = initialcapacity;
		heap->count    = 0U;

		initsuccess = true;
	}

	return initsuccess;
}

/*--------------------------------------------------
	boolean K_BHeapValid(bheap_t *const heap)

		See header file for description.
--------------------------------------------------*/
boolean K_BHeapValid(bheap_t *const heap)
{
	boolean heapvalid = false;

	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapValid.\n");
	}
	else
	{
		if ((heap->capacity > 0U) && (heap->array != NULL))
		{
			heapvalid = true;
		}
	}

	return heapvalid;
}

/*--------------------------------------------------
	boolean K_BHeapPush(bheap_t *const heap, void *const item, UINT32 value)

		See header file for description.
--------------------------------------------------*/
boolean K_BHeapPush(bheap_t *const heap, void *const item, UINT32 value)
{
	boolean pushsuccess = false;
	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapPush.\n");
	}
	else if (!K_BHeapValid(heap))
	{
		CONS_Debug(DBG_GAMELOGIC, "Uninitialised heap in K_BHeapPush.\n");
	}
	else if (item == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item in K_BHeapPush.\n");
	}
	else if (heap->count >= (SIZE_MAX / 2))
	{
		CONS_Debug(DBG_GAMELOGIC, "Tried to push too many items on binary heap in K_BHeapPush.\n");
	}
	else
	{
		// If the capacity of the heap has been reached, a realloc is needed
		// I'm just doing a basic double of capacity for simplicity
		if (heap->count >= heap->capacity)
		{
			size_t newarraycapacity = heap->capacity * 2;
			heap->array = Z_Realloc(heap->array, newarraycapacity, PU_STATIC, NULL);

			if (heap->array == NULL)
			{
				I_Error("K_BHeapPush: Out of Memory.");
			}

			heap->capacity = newarraycapacity;
		}

		heap->array[heap->count].heapindex = heap->count;
		heap->array[heap->count].owner     = heap;
		heap->array[heap->count].data      = item;
		heap->array[heap->count].value     = value;

		heap->count++;

		K_BHeapSortUp(heap, &heap->array[heap->count - 1U]);

		pushsuccess = true;
	}

	return pushsuccess;
}

/*--------------------------------------------------
	boolean K_BHeapPop(bheap_t *const heap, bheapitem_t *const returnitemstorage)

		See header file for description.
--------------------------------------------------*/
boolean K_BHeapPop(bheap_t *const heap, bheapitem_t *const returnitemstorage)
{
	boolean popsuccess = false;
	if (heap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL heap in K_BHeapPop.\n");
	}
	else if (!K_BHeapValid(heap))
	{
		CONS_Debug(DBG_GAMELOGIC, "Uninitialised heap in K_BHeapPop.\n");
	}
	else if (heap->count = 0U)
	{
		CONS_Debug(DBG_GAMELOGIC, "Tried to Pop from empty heap in K_BHeapPop.\n");
	}
	else if (returnitemstorage == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL returnitemstorage in K_BHeapPop.\n");
	}
	else
	{
		*returnitemstorage = heap->array[0];

		// Invalidate the heap related data from the return item
		returnitemstorage->owner = NULL;
		returnitemstorage->heapindex = SIZE_MAX;

		heap->count--;

		heap->array[0] = heap->array[heap->count];
		heap->array[0].heapindex = 0U;
		memset(&heap->array[heap->count], 0x00, sizeof(bheapitem_t));

		K_BHeapSortDown(heap, &heap->array[0]);
		popsuccess = true;
	}

	return popsuccess;
}

/*--------------------------------------------------
	boolean K_UpdateHeapItemValue(bheapitem_t *const item, const UINT32 newvalue)

		See header file for description.
--------------------------------------------------*/
boolean K_UpdateHeapItemValue(bheapitem_t *const item, const UINT32 newvalue)
{
	boolean updatevaluesuccess = false;
	if (item == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL item in K_UpdateHeapItemValue.\n");
	}
	else if (item->owner == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "item has NULL owner in K_UpdateHeapItemValue.\n");
	}
	else if (K_BHeapItemValidate(item->owner, item) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid item in K_UpdateHeapItemValue.\n");
	}
	else if (K_BHeapValid(item->owner) == false)
	{
		CONS_Debug(DBG_GAMELOGIC, "Invalid item owner in K_UpdateHeapItemValue.\n");
	}
	else
	{
		size_t oldvalue = item->value;
		item->value = newvalue;
		if (newvalue < oldvalue)
		{
			K_BHeapSortUp(item->owner, item);
		}
		else if (newvalue > oldvalue)
		{
			K_BHeapSortDown(item->owner, item);
		}
		else
		{
			// No change is needed as the value is the same
		}

	}

	return updatevaluesuccess;
}

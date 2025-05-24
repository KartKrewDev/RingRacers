// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sean "Sryder" Ryder
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_pathfind.c
/// \brief A* Pathfinding algorithm implementation for SRB2 code base.

#include "k_pathfind.h"

#include "doomdef.h"
#include "z_zone.h"
#include "k_bheap.h"

static const size_t DEFAULT_NODEARRAY_CAPACITY = 8U;
static const size_t DEFAULT_OPENSET_CAPACITY   = 8U;
static const size_t DEFAULT_CLOSEDSET_CAPACITY = 8U;


/*--------------------------------------------------
	static UINT32 K_NodeGetFScore(const pathfindnode_t *const node)

		Gets the FScore of a node. The FScore is the GScore plus the HScore.

	Input Arguments:-
		node - The node to get the FScore of

	Return:-
		The FScore of the node.
--------------------------------------------------*/
static UINT32 K_NodeGetFScore(const pathfindnode_t *const node)
{
	UINT32 fscore = UINT32_MAX;

	I_Assert(node != NULL);

	fscore = node->gscore + node->hscore;

	return fscore;
}

/*--------------------------------------------------
	static void K_NodeUpdateHeapIndex(void *const node, const size_t newheapindex)

		A callback for the Openset Binary Heap to be able to update the heapindex of the pathfindnodes when they are
		moved.

	Input Arguments:-
		node         - The node that has been updated, should be a pointer to a pathfindnode_t
		newheapindex - The new heapindex of the node.

	Return:-
		None
--------------------------------------------------*/
static void K_NodeUpdateHeapIndex(void *const node, const size_t newheapindex)
{
	if (node == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL node in K_NodeUpdateHeapIndex.\n");
	}
	else
	{
		pathfindnode_t *truenode = (pathfindnode_t*)node;
		truenode->heapindex = newheapindex;
	}
}

/*--------------------------------------------------
	static pathfindnode_t *K_NodesArrayContainsNodeData(
		pathfindnode_t *nodesarray,
		void* nodedata,
		size_t nodesarraycount)

		Checks whether the Nodes Array contains a node with a waypoint. Searches from the end to the start for speed
			reasons.

	Input Arguments:-
		nodesarray      - The nodes array within the A* algorithm
		waypoint        - The waypoint to check is within the nodes array
		nodesarraycount - The current size of the nodes array

	Return:-
		The pathfind node that has the waypoint if there is one. NULL if the waypoint is not in the nodes array.
--------------------------------------------------*/
static pathfindnode_t *K_NodesArrayContainsNodeData(
	pathfindnode_t *nodesarray,
	void* nodedata,
	size_t nodesarraycount)
{
	pathfindnode_t *foundnode = NULL;
	size_t i = 0U;

	I_Assert(nodesarray != NULL);
	I_Assert(nodedata != NULL);

	// It is more likely that we'll find the node we are looking for from the end of the array
	// Yes, the for loop looks weird, remember that size_t is unsigned and we want to check 0, after it hits 0 it
	// will loop back up to SIZE_MAX
	for (i = nodesarraycount - 1U; i < nodesarraycount; i--)
	{
		if (nodesarray[i].nodedata == nodedata)
		{
			foundnode = &nodesarray[i];
			break;
		}
	}
	return foundnode;
}

/*--------------------------------------------------
	static boolean K_ClosedsetContainsNode(pathfindnode_t **closedset, pathfindnode_t *node, size_t closedsetcount)

		Checks whether the Closedset contains a node. Searches from the end to the start for speed reasons.

	Input Arguments:-
		closedset      - The closed set within the A* algorithm
		node           - The node to check is within the closed set
		closedsetcount - The current size of the closedset

	Return:-
		True if the node is in the closed set, false if it isn't
--------------------------------------------------*/
static boolean K_ClosedsetContainsNode(pathfindnode_t **closedset, pathfindnode_t *node, size_t closedsetcount)
{
	boolean nodeisinclosedset = false;
	size_t i = 0U;

	I_Assert(closedset != NULL);
	I_Assert(node != NULL);
	// It is more likely that we'll find the node we are looking for from the end of the array
	// Yes, the for loop looks weird, remember that size_t is unsigned and we want to check 0, after it hits 0 it
	// will loop back up to SIZE_MAX
	for (i = closedsetcount - 1U; i < closedsetcount; i--)
	{
		if (closedset[i] == node)
		{
			nodeisinclosedset = true;
			break;
		}
	}
	return nodeisinclosedset;
}

/*--------------------------------------------------
	static boolean K_PathfindSetupValid(const pathfindsetup_t *const pathfindsetup)

		Checks that the setup given for pathfinding is valid and can be used.

	Input Arguments:-
		pathfindsetup - The setup for the pathfinding given

	Return:-
		True if pathfinding setup is valid, false if it isn't.
--------------------------------------------------*/
static boolean K_PathfindSetupValid(const pathfindsetup_t *const pathfindsetup)
{
	boolean pathfindsetupvalid = false;
	size_t sourcenodenumconnectednodes = 0U;
	size_t endnodenumconnectednodes    = 0U;

	if (pathfindsetup == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL pathfindsetup in K_PathfindSetupValid.\n");
	}
	else if (pathfindsetup->startnodedata == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Pathfindsetup has NULL startnodedata.\n");
	}
	else if (pathfindsetup->endnodedata == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Pathfindsetup has NULL endnodedata.\n");
	}
	else if (pathfindsetup->getconnectednodes == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Pathfindsetup has NULL getconnectednodes function.\n");
	}
	else if (pathfindsetup->getconnectioncosts == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Pathfindsetup has NULL getconnectioncosts function.\n");
	}
	else if (pathfindsetup->getheuristic == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Pathfindsetup has NULL getheuristic function.\n");
	}
	else if (pathfindsetup->gettraversable == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Pathfindsetup has NULL gettraversable function.\n");
	}
	else if (pathfindsetup->getfinished == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Pathfindsetup has NULL getfinished function.\n");
	}
	else if (pathfindsetup->getconnectednodes(pathfindsetup->startnodedata, &sourcenodenumconnectednodes) == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_PathfindSetupValid: Source node returned NULL connecting nodes.\n");
	}
	else if (sourcenodenumconnectednodes == 0U)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_PathfindSetupValid: Source node has 0 connecting nodes.\n");
	}
	else if (pathfindsetup->getconnectednodes(pathfindsetup->endnodedata, &endnodenumconnectednodes) == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_PathfindSetupValid: End node returned NULL connecting nodes.\n");
	}
	else if (endnodenumconnectednodes == 0U)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_PathfindSetupValid: End node has 0 connecting nodes.\n");
	}
	else
	{
		pathfindsetupvalid = true;
	}

	return pathfindsetupvalid;
}

static boolean K_ReconstructPath(path_t *const path, pathfindnode_t *const destinationnode)
{
	boolean reconstructsuccess = false;

	I_Assert(path != NULL);
	I_Assert(destinationnode != NULL);

	{
		size_t numnodes = 0U;
		pathfindnode_t *thisnode = destinationnode;

		// If the path we're placing our new path into already has data, free it
		if (path->array != NULL)
		{
			Z_Free(path->array);
			path->numnodes = 0U;
			path->totaldist = 0U;
		}

		// Do a fast check of how many nodes there are so we know how much space to allocate
		for (thisnode = destinationnode; thisnode; thisnode = thisnode->camefrom)
		{
			numnodes++;
		}

		if (numnodes > 0U)
		{
			// Allocate memory for the path
			path->numnodes  = numnodes;
			path->array     = Z_Calloc(numnodes * sizeof(pathfindnode_t), PU_STATIC, NULL);
			path->totaldist = destinationnode->gscore;
			if (path->array == NULL)
			{
				I_Error("K_ReconstructPath: Out of memory.");
			}

			// Put the nodes into the return array
			for (thisnode = destinationnode; thisnode; thisnode = thisnode->camefrom)
			{
				path->array[numnodes - 1U] = *thisnode;
				// Correct the camefrom element to point to the previous element in the array instead
				if ((path->array[numnodes - 1U].camefrom != NULL) && (numnodes > 1U))
				{
					path->array[numnodes - 1U].camefrom = &path->array[numnodes - 2U];
				}
				else
				{
					path->array[numnodes - 1U].camefrom = NULL;
				}

				numnodes--;
			}

			reconstructsuccess = true;
		}
	}

	return reconstructsuccess;
}

/*--------------------------------------------------
	boolean K_PathfindAStar(path_t *const path, pathfindsetup_t *const pathfindsetup)

		See header file for description.
--------------------------------------------------*/
boolean K_PathfindAStar(path_t *const path, pathfindsetup_t *const pathfindsetup)
{
	boolean pathfindsuccess = false;

	if (path == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL path in K_PathfindAStar.\n");
	}
	else if (pathfindsetup == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL pathfindsetup in K_PathfindAStar.\n");
	}
	else if (!K_PathfindSetupValid(pathfindsetup))
	{
		CONS_Debug(DBG_GAMELOGIC, "K_PathfindAStar: Pathfinding setup is not valid.\n");
	}
	else
	{
		pathfindnode_t singlenode = {0};
		singlenode.camefrom  = NULL;
		singlenode.nodedata = pathfindsetup->startnodedata;
		singlenode.heapindex = SIZE_MAX;
		singlenode.hscore    = 0U;
		singlenode.gscore    = 0U;

		if (pathfindsetup->getfinished(&singlenode, pathfindsetup) == true)
		{
			// At the destination, return a simple 1 node path
			K_ReconstructPath(path, &singlenode);
			pathfindsuccess = true;
		}
		else
		{
			bheap_t        openset                 = {0};
			bheapitem_t    poppedbheapitem         = {0};
			pathfindnode_t *nodesarray             = NULL;
			pathfindnode_t **closedset             = NULL;
			pathfindnode_t *newnode                = NULL;
			pathfindnode_t *currentnode            = NULL;
			pathfindnode_t *connectingnode         = NULL;
			void           **connectingnodesdata   = NULL;
			void           *checknodedata          = NULL;
			UINT32         *connectingnodecosts    = NULL;
			size_t         numconnectingnodes      = 0U;
			size_t         connectingnodeheapindex = 0U;
			size_t         nodesarraycount         = 0U;
			size_t         closedsetcount          = 0U;
			size_t         i                       = 0U;
			UINT32         tentativegscore         = 0U;

			// Set the dynamic structure capacites to defaults if they are 0
			if (pathfindsetup->nodesarraycapacity == 0U)
			{
				pathfindsetup->nodesarraycapacity = DEFAULT_NODEARRAY_CAPACITY;
			}
			if (pathfindsetup->opensetcapacity == 0U)
			{
				pathfindsetup->opensetcapacity = DEFAULT_OPENSET_CAPACITY;
			}
			if (pathfindsetup->closedsetcapacity == 0U)
			{
				pathfindsetup->closedsetcapacity = DEFAULT_CLOSEDSET_CAPACITY;
			}

			// Allocate the necessary memory
			nodesarray = Z_Calloc(pathfindsetup->nodesarraycapacity * sizeof(pathfindnode_t), PU_STATIC, NULL);
			if (nodesarray == NULL)
			{
				I_Error("K_PathfindAStar: Out of memory allocating nodes array.");
			}
			closedset = Z_Calloc(pathfindsetup->closedsetcapacity * sizeof(pathfindnode_t*), PU_STATIC, NULL);
			if (closedset == NULL)
			{
				I_Error("K_PathfindAStar: Out of memory allocating closed set.");
			}
			K_BHeapInit(&openset, pathfindsetup->opensetcapacity);

			// Create the first node and add it to the open set
			newnode            = &nodesarray[nodesarraycount];
			newnode->heapindex = SIZE_MAX;
			newnode->nodedata  = pathfindsetup->startnodedata;
			newnode->camefrom  = NULL;
			newnode->gscore    = 0U;
			newnode->hscore    = pathfindsetup->getheuristic(newnode->nodedata, pathfindsetup->endnodedata);
			nodesarraycount++;
			K_BHeapPush(&openset, newnode, K_NodeGetFScore(newnode), K_NodeUpdateHeapIndex);

			// update openset capacity if it changed
			if (openset.capacity != pathfindsetup->opensetcapacity)
			{
				pathfindsetup->opensetcapacity = openset.capacity;
			}

			// Go through each node in the openset, adding new ones from each node to it
			// this continues until a path is found or there are no more nodes to check
			while (openset.count > 0U)
			{
				// pop the best node off of the openset
				K_BHeapPop(&openset, &poppedbheapitem);
				currentnode = (pathfindnode_t*)poppedbheapitem.data;

				if (pathfindsetup->getfinished(currentnode, pathfindsetup) == true)
				{
					pathfindsuccess = K_ReconstructPath(path, currentnode);
					break;
				}

				// Place the node we just popped into the closed set, as we are now evaluating it
				if (closedsetcount >= pathfindsetup->closedsetcapacity)
				{
					// Need to reallocate closedset to fit another node
					pathfindsetup->closedsetcapacity = pathfindsetup->closedsetcapacity * 2;
					closedset =
						Z_Realloc(closedset, pathfindsetup->closedsetcapacity * sizeof(pathfindnode_t*), PU_STATIC, NULL);
					if (closedset == NULL)
					{
						I_Error("K_PathfindAStar: Out of memory reallocating closed set.");
					}
				}
				closedset[closedsetcount] = currentnode;
				closedsetcount++;

				// Get the needed data for the next nodes from the current node
				connectingnodesdata = pathfindsetup->getconnectednodes(currentnode->nodedata, &numconnectingnodes);
				connectingnodecosts = pathfindsetup->getconnectioncosts(currentnode->nodedata);

				if (connectingnodesdata == NULL)
				{
					CONS_Debug(DBG_GAMELOGIC, "K_PathfindAStar: A Node returned NULL connecting node data.\n");
				}
				else if (connectingnodecosts == NULL)
				{
					CONS_Debug(DBG_GAMELOGIC, "K_PathfindAStar: A Node returned NULL connecting node costs.\n");
				}
				else
				{
					// For each connecting node add it to the openset if it's unevaluated and not there,
					// skip it if it's in the closedset or not traversable
					for (i = 0; i < numconnectingnodes; i++)
					{
						checknodedata = connectingnodesdata[i];

						if (checknodedata == NULL)
						{
							CONS_Debug(DBG_GAMELOGIC, "K_PathfindAStar: A Node has a NULL connecting node.\n");
						}
						else
						{
							// skip this node if it isn't traversable
							if (pathfindsetup->gettraversable(checknodedata, currentnode->nodedata) == false)
							{
								continue;
							}

							// Figure out what the gscore of this route for the connecting node is
							tentativegscore = currentnode->gscore + connectingnodecosts[i];

							// find this data in the nodes array if it's been generated before
							connectingnode = K_NodesArrayContainsNodeData(nodesarray, checknodedata, nodesarraycount);

							if (connectingnode != NULL)
							{
								// The connecting node has been seen before, so it must be in either the closedset (skip it)
								// or the openset (re-evaluate it's gscore)
								if (K_ClosedsetContainsNode(closedset, connectingnode, closedsetcount) == true)
								{
									continue;
								}
								else if (tentativegscore < connectingnode->gscore)
								{
									// The node is not in the closedset, update it's gscore if this path to it is faster
									connectingnode->gscore   = tentativegscore;
									connectingnode->camefrom = currentnode;

									connectingnodeheapindex =
										K_BHeapContains(&openset, connectingnode, connectingnode->heapindex);
									if (connectingnodeheapindex != SIZE_MAX)
									{
										K_UpdateBHeapItemValue(
											&openset.array[connectingnodeheapindex], K_NodeGetFScore(connectingnode));
									}
									else
									{
										// SOMEHOW the node is not in either the closed set OR the open set
										CONS_Debug(DBG_GAMELOGIC, "K_PathfindAStar: A Node is not in either set.\n");
									}
								}
							}
							else
							{
								// Node is not created yet, so it hasn't been seen so far
								// Reallocate nodesarray if it's full
								if (nodesarraycount >= pathfindsetup->nodesarraycapacity)
								{
									pathfindnode_t *nodesarrayrealloc = NULL;
									pathfindsetup->nodesarraycapacity = pathfindsetup->nodesarraycapacity * 2;
									nodesarrayrealloc = Z_Realloc(nodesarray, pathfindsetup->nodesarraycapacity * sizeof(pathfindnode_t), PU_STATIC, NULL);

									if (nodesarrayrealloc == NULL)
									{
										I_Error("K_PathfindAStar: Out of memory reallocating nodes array.");
									}

									// Need to update pointers in closedset, openset, and node "camefrom" if nodesarray moved.
									if (nodesarray != nodesarrayrealloc)
									{
										size_t j = 0U;
										size_t arrayindex = 0U;
										for (j = 0U; j < closedsetcount; j++)
										{
											arrayindex = closedset[j] - nodesarray;
											closedset[j] = &nodesarrayrealloc[arrayindex];
										}
										for (j = 0U; j < openset.count; j++)
										{
											arrayindex = ((pathfindnode_t *)(openset.array[j].data)) - nodesarray;
											openset.array[j].data = &nodesarrayrealloc[arrayindex];
										}
										for (j = 0U; j < nodesarraycount; j++)
										{
											if (nodesarrayrealloc[j].camefrom != NULL)
											{
												arrayindex = nodesarrayrealloc[j].camefrom - nodesarray;
												nodesarrayrealloc[j].camefrom = &nodesarrayrealloc[arrayindex];
											}
										}

										arrayindex = currentnode - nodesarray;
										currentnode = &nodesarrayrealloc[arrayindex];
									}

									nodesarray = nodesarrayrealloc;
								}

								// Create the new node and add it to the nodes array and open set
								newnode            = &nodesarray[nodesarraycount];
								newnode->heapindex = SIZE_MAX;
								newnode->nodedata  = checknodedata;
								newnode->camefrom  = currentnode;
								newnode->gscore    = tentativegscore;
								newnode->hscore    = pathfindsetup->getheuristic(newnode->nodedata, pathfindsetup->endnodedata);
								nodesarraycount++;
								K_BHeapPush(&openset, newnode, K_NodeGetFScore(newnode), K_NodeUpdateHeapIndex);
							}
						}
					}
				}
			}

			// Clean up the memory
			K_BHeapFree(&openset);
			Z_Free(closedset);
			Z_Free(nodesarray);
		}
	}

	return pathfindsuccess;
}

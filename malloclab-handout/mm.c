#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* Macros for unscaled pointer arithmetic to keep other code cleaner.
   Casting to a char* has the effect that pointer arithmetic happens at
   the byte granularity (i.e. POINTER_ADD(0x1, 1) would be 0x2).  (By
   default, incrementing a pointer in C has the effect of incrementing
   it by the size of the type to which it points (e.g. Block).)
   We cast the result to void* to force you to cast back to the
   appropriate type and ensure you don't accidentally use the resulting
   pointer as a char* implicitly.
*/
#define UNSCALED_POINTER_ADD(p, x) ((void*)((char*)(p) + (x)))
#define UNSCALED_POINTER_SUB(p, x) ((void*)((char*)(p) - (x)))

/* An BlockInfo contains information about a block, including the size
   as well as pointers to the next and previous blocks in the free list.
   This is similar to the "explicit free list" structure illustrated in
   the lecture slides.

   Note that the next pointer are only needed when the block is free. To
   achieve better utilization, mm_malloc should use the space for next as
   part of the space it returns.

   +--------------+
   |     size     |  <-  Block pointers in free list point here
   |              |
   |   (header)   |
   |              |
   |     prev     |
   +--------------+
   |   nextFree   |  <-  Pointers returned by mm_malloc point here
   |   prevFree   |
   +--------------+      (allocated blocks do not have a 'nextFree' field)
   |  space and   |      (this is a space optimization...)
   |   padding    |
   |     ...      |      Free blocks write their nextFree/prevFree pointers in
   |     ...      |      this space.
   +--------------+

*/
typedef struct _BlockInfo {
  // Size of the block and whether or not the block is in use or free.
  // When the size is negative, the block is currently free.
  long int size;
  // Pointer to the previous block in the list.
  struct _Block* prev;
} BlockInfo;

/* A FreeBlockInfo structure contains metadata just for free blocks.
 * When you are ready, you can improve your naive implementation by
 * using these to maintain a separate list of free blocks.
 *
 * These are "kept" in the region of memory that is normally used by
 * the program when the block is allocated. That is, since that space
 * is free anyway, we can make good use of it to improve our malloc.
 */
typedef struct _FreeBlockInfo {
  // Pointer to the next free block in the list.
  struct _Block* nextFree;
  // Pointer to the previous free block in the list.
  struct _Block* prevFree;
} FreeBlockInfo;

/* This is a structure that can serve as all kinds of nodes.
 */
typedef struct _Block {
  BlockInfo info;
  FreeBlockInfo freeNode;
} Block;

/* Pointer to the first FreeBlockInfo in the free list, the list's head. */
static Block* free_list_head = NULL;
static Block* malloc_list_tail = NULL;

static size_t heap_size = 0;

/* Size of a word on this architecture. */
#define WORD_SIZE sizeof(void*)

/* Alignment of blocks returned by mm_malloc.
 * (We need each allocation to at least be big enough for the free space
 * metadata... so let's just align by that.)  */
#define ALIGNMENT (sizeof(FreeBlockInfo))


/* This function will have the OS allocate more space for our heap.
 *
 * It returns a pointer to that new space. That pointer will always be
 * larger than the last request and be continuous in memory.
 */
void* requestMoreSpace(size_t reqSize);

/* This function will get the first block or returns NULL if there is not
 * one.
 *
 * You can use this to start your through search for a block.
 */
Block* first_block();

/* This function will get the adjacent block or returns NULL if there is not
 * one.
 *
 * You can use this to move along your malloc list one block at a time.
 */
Block* next_block(Block* block);

/* Use this function to print a thorough listing of your heap data structures.
 */
void examine_heap();

/* Checks the heap for any issues and prints out errors as it finds them.
 *
 * Use this when you are debugging to check for consistency issues. */
int check_heap();

Block* searchList(size_t reqSize) {
  Block* ptrFreeBlock = first_block();
  long int checkSize = -reqSize;
  //code starts here...
  if(ptrFreeBlock != NULL){
    while(ptrFreeBlock != NULL){ //while ptrFreeBlock is NOT null
      if(checkSize >= ptrFreeBlock->info.size) return ptrFreeBlock; //return the block since we have found a fit
    ptrFreeBlock = next_block(ptrFreeBlock); //iterate to next available block
    }
  }
  return NULL; //if ptrfreeBlock is NULL, that means we cant find space so return null (we'll need to request more space inside mm function if this happens)
}

/* Find a free block of at least the requested size in the free list.  Returns
   NULL if no free block is large enough. */
Block* searchFreeList(size_t reqSize) {
  Block* ptrFreeBlock = free_list_head;
  long int checkSize = -reqSize;
  
  //code starts here...
  // When you are ready, you can implement the free list.
  return NULL; //return null if we cant find anything
}

// TOP-LEVEL ALLOCATOR INTERFACE ------------------------------------

/* Allocate a block of size size and return a pointer to it. If size is zero,
 * returns null.
 */
void* mm_malloc(size_t size) {
  Block* ptrFreeBlock = NULL;
  Block* splitBlock = NULL;
  Block* tmpFreeBlock = NULL;
  long int reqSize;
  // Zero-size requests get NULL.
  if (size == 0) {
    return NULL;
  }
  reqSize = size;
  reqSize = ALIGNMENT * ((reqSize + ALIGNMENT - 1) / ALIGNMENT);
 
 //code starts here
  ptrFreeBlock = searchList(reqSize); //checks if there is a single block available in the heap  I.E [-1] [SPOT FOUND] [-1] [-1] [-1]

    //<<<<<<<<>>> IF WE FOUND A BLOCK <<<<<<<<<>>>
    if(ptrFreeBlock != NULL){ //if we found a block
        long int positiveSize = -(ptrFreeBlock->info.size); //compute positiveSize which is negation of ptrFreeBlock size (positive value we can use to deduce correct size)
        long int actualBlockSize = positiveSize - reqSize; //calculate the actualBlockSize by subtracting reqSize from positiveSize (which is the negation of ptrFreeBlock->info.size)
        //if we found a match that can fit the size we simply negate to show its "allocated"
        if(actualBlockSize <= sizeof(Block)) ptrFreeBlock->info.size*=-1; 
        //if actualBlockSize is larger then sizeOfBlock, the block is too large to allocate so we must start splitting
        if(actualBlockSize > sizeof(Block)){ //if we found a match but the new block is too big (we need to begin splitting)
          //first lets return the first address that the program can safely write to
          splitBlock = UNSCALED_POINTER_ADD(ptrFreeBlock, sizeof(BlockInfo) + reqSize); //save block pointer to splitBlock which is second part (or right side part) of split block. IE [Splitblock].
          splitBlock->info.prev = ptrFreeBlock; //this sets splitBlocks previous (which is the 1st part of splitlock, or left part) to be ptrFreeBlock, IE [ptrFreeBlock]][splitBlock]
          splitBlock->info.size = -(actualBlockSize); //we set splitBlock size to negation of actualBlock size to mark as "allocated"  
          //make sure when you split a block, it has enough space for all of the metadata: you MUST add the sizeOfBlockInfo to the splitBlockInfo.size because of this to account for metadata
          splitBlock->info.size += sizeof(BlockInfo); //updating the block info metadata since we split the block (we need to also update the split blocks metadata)
          ptrFreeBlock->info.size = reqSize; //set size since has content
          //if the next_block after SplitBlock exists, set that blocks previous to splitBlock
          tmpFreeBlock = next_block(splitBlock);
          if(tmpFreeBlock != NULL) tmpFreeBlock->info.prev = splitBlock; //merge next_block(splitBlock) with its previous by setting nextSplitBlock(splitBlock) previous to be splitBlock
        }
        return UNSCALED_POINTER_ADD(ptrFreeBlock, sizeof(BlockInfo)); //return addres for a block we found a block that diddnt need space requested 
      }
      //<<<<<<<<>>> IF WE DID NOT FIND A BLOCK <<<<<<<<<>>>
      ptrFreeBlock = requestMoreSpace(reqSize + sizeof(BlockInfo)); //if theres  no space, we need to allocate extra space for the block (this created a new block with metadata) 
      ptrFreeBlock->info.prev = malloc_list_tail; //adding block to end by makings its prevoius point to current malloc list tail
      malloc_list_tail = ptrFreeBlock; //updating the malloc_list_tail pointer to point the new tail latest block
      ptrFreeBlock->info.size = reqSize; //updating the size of the allocated block that was just initialized (everytime we make block we need to set the size)
      return UNSCALED_POINTER_ADD(ptrFreeBlock, sizeof(BlockInfo)); //return addres for a block we found a block that diddnt need space requested 
}

void coalesce(Block* blockInfo) {
  Block* nextBlock = next_block(blockInfo);
  Block* previousBlock = blockInfo->info.prev;
  Block* tmpBlock = NULL;

 //checking nextBlock for possible coalesce 
  if(nextBlock != NULL && nextBlock->info.size < 0){
    blockInfo->info.size += nextBlock->info.size - sizeof(BlockInfo); //must add the difference between nextBlockInfoSize - sizeOf(BlockInfo to blockInfo size to accomodate space
    tmpBlock = next_block(nextBlock); //acquire block to the right of nextBlock (right of the next block - aka next_next block)
    if(tmpBlock != NULL){ //if we are not at Tail  (not coalescing tail)
      tmpBlock->info.prev = blockInfo; //set nextNextBox previous to be blockInfo (merging the two)
      malloc_list_tail = tmpBlock; //setting the last item as temp tail

    }
  }
  //checking previousBlock for possible coalesce
  if(previousBlock != NULL && previousBlock->info.size < 0){
    previousBlock->info.size += blockInfo->info.size - sizeof(BlockInfo); //accounting for metadata
    tmpBlock = next_block(blockInfo); //acquire block to the right of blockInfo
    if(tmpBlock != NULL){//if not at tail set 
      tmpBlock->info.prev = previousBlock; //grabbing the block to the right and pointing it to the left (merging the two)
      malloc_list_tail = tmpBlock; //setting last item as temp tail
    }
  }
}
//DONE

/* Free the block referenced by ptr. */
void mm_free(void* ptr) {
  Block* blockInfo = (Block*)UNSCALED_POINTER_SUB(ptr, sizeof(BlockInfo));
  blockInfo->info.size*=-1;
  coalesce(blockInfo);
}

// PROVIDED FUNCTIONS -----------------------------------------------
// You do not need to modify these, but they might be helpful to read
// over.

/* Get more heap space of exact size reqSize. */
void* requestMoreSpace(size_t reqSize) {
  void* ret = UNSCALED_POINTER_ADD(mem_heap_lo(), heap_size);
  heap_size += reqSize;

  void* mem_sbrk_result = mem_sbrk(reqSize);
  if ((size_t)mem_sbrk_result == -1) {
    printf("ERROR: mem_sbrk failed in requestMoreSpace\n");
    exit(0);
  }

  return ret;
}

/* Initialize the allocator. */
int mm_init() {
  free_list_head = NULL;
  malloc_list_tail = NULL;
  heap_size = 0;

  return 0;
}

/* Gets the first block in the heap or returns NULL if there is not one. */
Block* first_block() {
  Block* first = (Block*)mem_heap_lo();
  if (heap_size == 0) {
    return NULL;
  }
  return first;
}

/* Gets the adjacent block or returns NULL if there is not one. */
Block* next_block(Block* block) {
  size_t distance = (block->info.size > 0) ? block->info.size : -block->info.size;

  Block* end = (Block*)UNSCALED_POINTER_ADD(mem_heap_lo(), heap_size);
  Block* next = (Block*)UNSCALED_POINTER_ADD(block, sizeof(BlockInfo) + distance);
  if (next >= end) {
    return NULL;
  }
  return next;
}

/* Print the heap by iterating through it as an implicit free list. */
void examine_heap() {
  /* print to stderr so output isn't buffered and not output if we crash */
  Block* curr = (Block*)mem_heap_lo();
  Block* end = (Block*)UNSCALED_POINTER_ADD(mem_heap_lo(), heap_size);
  fprintf(stderr, "heap size:\t0x%lx\n", heap_size);
  fprintf(stderr, "heap start:\t%p\n", curr);
  fprintf(stderr, "heap end:\t%p\n", end);

  fprintf(stderr, "free_list_head: %p\n", (void*)free_list_head);

  fprintf(stderr, "malloc_list_tail: %p\n", (void*)malloc_list_tail);

  while(curr && curr < end) {
    /* print out common block attributes */
    fprintf(stderr, "%p: %ld\t", (void*)curr, curr->info.size);

    /* and allocated/free specific data */
    if (curr->info.size > 0) {
      fprintf(stderr, "ALLOCATED\tprev: %p\n", (void*)curr->info.prev);
    } else {
      fprintf(stderr, "FREE\tnextFree: %p, prevFree: %p, prev: %p\n", (void*)curr->freeNode.nextFree, (void*)curr->freeNode.prevFree, (void*)curr->info.prev);
    }

    curr = next_block(curr);
  }
  fprintf(stderr, "END OF HEAP\n\n");

  curr = free_list_head;
  fprintf(stderr, "Head ");
  while(curr) {
    fprintf(stderr, "-> %p ", curr);
    curr = curr->freeNode.nextFree;
  }
  fprintf(stderr, "\n");
}

/* Checks the heap data structure for consistency. */
int check_heap() {
  Block* curr = (Block*)mem_heap_lo();
  Block* end = (Block*)UNSCALED_POINTER_ADD(mem_heap_lo(), heap_size);
  Block* last = NULL;
  long int free_count = 0;

  while(curr && curr < end) {
    if (curr->info.prev != last) {
      fprintf(stderr, "check_heap: Error: previous link not correct.\n");
      examine_heap();
    }

    if (curr->info.size <= 0) {
      // Free
      free_count++;
    }

    last = curr;
    curr = next_block(curr);
  }

  curr = free_list_head;
  last = NULL;
  while(curr) {
    if (curr == last) {
      fprintf(stderr, "check_heap: Error: free list is circular.\n");
      examine_heap();
    }
    last = curr;
    curr = curr->freeNode.nextFree;
    if (free_count == 0) {
      fprintf(stderr, "check_heap: Error: free list has more items than expected.\n");
      examine_heap();
    }
    free_count--;
  }
  return 0;
}

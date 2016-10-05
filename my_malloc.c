#include "my_malloc.h"

/* You *MUST* use this macro when calling my_sbrk to allocate the
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* Please use this value as your canary! */
#define CANARY 0x2110CAFE

/* If you want to use debugging printouts, it is HIGHLY recommended
 * to use this macro or something similar. If you produce output from
 * your code then you may receive a 20 point deduction. You have been
 * warned.
 */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif


/* our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * Technically this should be declared static for the same reasons
 * as above, but DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */
metadata_t* freelist;

static int stack;
static int start = 0;

void* my_malloc(size_t size) {

    ERRNO = NO_ERROR;
    int total = size + sizeof(metadata_t) + 2 * sizeof(CANARY);

    

    if (total + stack > 8192) {
	   
	   ERRNO = OUT_OF_MEMORY;
	   return NULL;
    
    }

    if (total > 2048) {
	   
	   ERRNO = SINGLE_REQUEST_TOO_LARGE;
	   return NULL;
    
    }

    if (freelist == NULL) {

	   if (SBRK_SIZE + stack > 8192) {

		  ERRNO = OUT_OF_MEMORY;
		  return NULL;

	   }

	   freelist = my_sbrk(SBRK_SIZE);

	   if (freelist == NULL) {

		  ERRNO = OUT_OF_MEMORY;
		  return NULL;

	   }

	   stack += SBRK_SIZE;
	   freelist->prev = NULL;
	   freelist->next = NULL;
	   freelist->request_size = 0;
	   freelist->block_size = 2048;

    }

    if (!start) {

	   freelist = my_sbrk(SBRK_SIZE);

	   if (freelist == NULL) {

		  ERRNO = OUT_OF_MEMORY;
		  return NULL;

	   } else {

		  stack += SBRK_SIZE;
		  freelist->prev = NULL;
		  freelist->next = NULL;
		  freelist->request_size = 0;
		  freelist->block_size = 2048;
		  start = 1;

	   }
    }

    metadata_t* aBlock = getSmallest(total);
  //  char* temp = (char*) aBlock;
  //  int canary = CANARY;
  //  temp[sizeof(metadata_t) + 1] = canary;
  //  temp[sizeof(metadata_t) + sizeof(int) + size + 1] = canary;

    if (aBlock == NULL) {

	   if (SBRK_SIZE + stack > 8192) {

		  ERRNO = OUT_OF_MEMORY;
		  return NULL;
	   }
	   
	   metadata_t* smallBlock = my_sbrk(SBRK_SIZE);

	   if (smallBlock == NULL) {

		  ERRNO = OUT_OF_MEMORY;
		  return NULL;
	   } 

	   smallBlock->block_size = 2048;
	   smallBlock->request_size = size;  
	   push(smallBlock);
	   stack += SBRK_SIZE;

	   return my_malloc(size);

    } else if (aBlock->block_size == total) {

	   aBlock->request_size = size;
	   removeBlock(aBlock);
	   return aBlock + 1;

    } else {

	   if (aBlock->block_size < total + sizeof(metadata_t) + 1) {

		  removeBlock(aBlock);
		  return aBlock + 1;

	   }

	   metadata_t* totalChunk = aBlock + total;
	   size_t new_size = aBlock->block_size - total;
	   aBlock->block_size = total;
	   aBlock->request_size = size;
	   totalChunk->block_size = new_size;
	   totalChunk->request_size = 0;
	   totalChunk->next = NULL;
	   totalChunk->prev = NULL;
	   removeBlock(aBlock);
	   push(totalChunk);
	   return ((metadata_t*) aBlock) + 1;

    }
    
    return NULL;
}

void my_free(void* ptr) {

    metadata_t* chunk = ((metadata_t*) ptr) - 1;

    chunk->prev = NULL;
    chunk->request_size = 0;
    chunk->next = NULL;

    metadata_t* r = getRight(chunk);
    metadata_t* l = getLeft(chunk);

    if (ptr == NULL) {

	   return;

    }

    if (l != NULL) {

	   metadata_t* aChunk = fuse(l, chunk);

	   // both left and right side are non-empty
	   if (r == NULL) {

		  push(aChunk);
		  return;

	   } else if (r != NULL) {

		  aChunk = fuse(r, aChunk);
		  push(aChunk);
		  return;

	   }
    }

    if (r == NULL) {

	   push(chunk);
	   return;

    } else {

	   metadata_t* aChunk = fuse(chunk, r);
	   push(aChunk);
	   return;

    }

    if (r == NULL && l == NULL) {

        push(chunk);

    }
}

int contains(metadata_t* chunk) {

    metadata_t* aChunk = freelist;

    if (aChunk == NULL) {

        return 0;

    }
    while (aChunk != NULL) {

	   if (aChunk == chunk) {

		  return 1;
	   }

	   aChunk = aChunk->next;

    }


    return 0;
}

void* fuse(metadata_t* aChunk, metadata_t* currentChunk) {

    metadata_t* resultChunk;
    removeBlock(aChunk);
    removeBlock(currentChunk);

    if (aChunk > currentChunk) {

	   resultChunk = currentChunk;
	   resultChunk->block_size = aChunk->block_size + currentChunk->block_size;
	   return resultChunk;

    } else {

	   resultChunk = aChunk;
	   resultChunk->block_size = aChunk->block_size + currentChunk->block_size;
	   return resultChunk;

    }

}

void removeBlock(metadata_t* chunk) {

    if (chunk == NULL) {

        return;

    }
    if (contains(chunk) == 0) {

	   return;

    }

    if (freelist == chunk) {

	   freelist = chunk->next;

	   if (chunk->next != NULL) {

		  metadata_t* temp = chunk->next;
                  temp->prev = NULL;

	   }

           chunk->prev = NULL;
	   chunk->next = NULL;
	   chunk->request_size = 1;
	   return;

    }

    chunk->request_size = 1;

    if (chunk->next == NULL && chunk->prev == NULL) {

	   freelist = NULL;

    } else if (chunk->next != NULL && chunk->prev == NULL) {

	   chunk->next->prev = NULL;

    } else if (chunk->next == NULL && chunk->prev != NULL) {

	   chunk->prev->next = NULL;

    } else if (chunk->next != NULL && chunk->prev != NULL) {

	   metadata_t* next = chunk->next;
           metadata_t* prev = chunk->prev;
	   next->prev = chunk->prev;
	   prev->next = next;

    }

    chunk->next = NULL;
    chunk->prev = NULL;

}

void push(metadata_t* chunk) {

    if (freelist == NULL) {

	   chunk->request_size = 0;
	   freelist = chunk;
	   chunk->next = NULL;
	   chunk->prev = NULL;
	   return;

    }

    chunk->request_size = 0;
    

    // if block size is smaller than our freelist, then we attach block behind freelist
    if (freelist->block_size > chunk->block_size) {

	   metadata_t* aChunk = freelist;
	   chunk->next = aChunk;
	   chunk->prev = NULL;  
	   aChunk->prev = chunk;
	   freelist = chunk;
	   return;

    }

    metadata_t* aChunk = freelist;
    // iterate through the freelist (linked list)
    while (aChunk->next != NULL) {
	   
	   if (aChunk->block_size > chunk->block_size) {

                  metadata_t* temp = aChunk->prev;
		  temp->next = chunk;
		  chunk->next = aChunk;
		  chunk->prev = temp;
		  aChunk->prev = chunk;
		  return;

	   }

	   aChunk = aChunk->next;
    }
    // last block/item in linked list
    metadata_t* last = aChunk;

    if (last->next == NULL) {

	   if (last->block_size > chunk->block_size) {

                  metadata_t* temp = last->prev;
	    	  temp->next = chunk;
	    	  chunk->next = last;
		  chunk->prev = temp;  
		  last->prev = chunk;
		  return;

	   }

	   last->next = chunk;
	   chunk->prev = last;

    }

}

void* getLeft(metadata_t* chunk) {

    if (freelist == NULL) {

	   return NULL;

    }

    metadata_t* aChunk = freelist;

    while (aChunk != NULL) {

	   if (chunk == aChunk + aChunk->block_size) {

		  return aChunk;

	   }

	   aChunk = aChunk->next;
    }

    return NULL;
}

void* getRight(metadata_t* chunk) {

    if (freelist == NULL) {

	   return NULL;

    }

    metadata_t* aChunk = freelist;

    while (aChunk != NULL) {

		  if (chunk + chunk->block_size == aChunk) {

			 return aChunk;

		  }

	   aChunk = aChunk->next;
    }

    return NULL;
}

void* getSmallest(size_t size) {

    metadata_t* aChunk = freelist; 

    while (aChunk != NULL) {
	   
	   if (aChunk->request_size == 0) {

		  if (size <= aChunk->block_size) {

			 return aChunk;

		  }

	   }

	   aChunk = aChunk->next;
    }

    return NULL;
}

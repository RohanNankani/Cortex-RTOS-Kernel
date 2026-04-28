#include "k_mem.h"
#include "k_task.h"

U32 MAGIC_CONSTANT = 0xDEADBEEF;

struct metadata* head = NULL;
int mem_inited = 0;

size_t METADATA_SIZE = sizeof(struct metadata);


int k_mem_init() {
	int result;
    __asm volatile (
        "svc #7\n"
        "mov %0, r0\n"
        : "=r"(result)
        :
        : "memory"
    );
    return result;
}

void* k_mem_alloc(size_t size) {
    int status;
    __asm volatile (
        "mov r0, %1\n" // put task pointer into R0
        "svc #8\n" // trigger SVC
        "mov %0, r0\n" // move R0 (return value) into C variable
        : "=r"(status) // output is whatever R0 contains after SVC
        : "r"(size) // input is task
        : "r0", "memory" // clobbered register
    );
    return status;
}

int k_mem_dealloc(void* ptr) {
    int status;
    __asm volatile (
        "mov r0, %1\n" // put task pointer into R0
        "svc #9\n" // trigger SVC
        "mov %0, r0\n" // move R0 (return value) into C variable
        : "=r"(status) // output is whatever R0 contains after SVC
        : "r"(ptr) // input is task
        : "r0", "memory" // clobbered register
    );
    return status;
}

int k_mem_count_extfrag(size_t size) { 
    int status;
    __asm volatile (
        "mov r0, %1\n" // put task pointer into R0
        "svc #10\n" // trigger SVC
        "mov %0, r0\n" // move R0 (return value) into C variable
        : "=r"(status) // output is whatever R0 contains after SVC
        : "r"(size) // input is task
        : "r0", "memory" // clobbered register
    );
    return status;
}

int osMemInit() {
	extern g_inited;
	if (mem_inited == 1 || g_inited == 0) {
		return RTX_ERR;
	}
    extern U32 _estack;
	extern U32 _img_end;
	head = &_img_end;

	U32* heap_top = ((U32)&_estack - MAX_STACK_SIZE);
	U32* heap_end = (U32*)&_img_end;

	struct metadata* meta = (struct metadata*) head;
	meta -> owner = -1;
	meta -> next = NULL;
	meta -> size = (U32*)((U32)heap_top - (U32)heap_end);
    meta -> magic = MAGIC_CONSTANT;
	mem_inited = 1;
	return RTX_OK;
}

void* osMemAlloc(size_t size) {

    if (!mem_inited || head == NULL || size == 0) {
        return NULL;
    }

    // pad so its always multiple of 4 bytes
    if (size % 4 != 0) {
        size += (4 - (size % 4));
    }


    // iterate through the free list and find the first possible block
    struct metadata* curr = head;
    struct metadata* prev = NULL;

    while (curr != NULL){

        size_t blockSize = curr->size;

        // check if the size can fit (accounting for metadata struct space)
        if (size + METADATA_SIZE <= blockSize){
            extern g_current_tid;

            // 2 cases

            // 1. full space needed
            if (blockSize - METADATA_SIZE - size < METADATA_SIZE + 4){
                if (prev == NULL){
                    head = curr -> next;
                }
                else {
                    prev -> next = curr -> next;
                }
                curr -> owner = g_current_tid;
                curr -> next = NULL;
                curr -> magic = MAGIC_CONSTANT;
            }
            // 2. partial space needed
            else {
                struct metadata* newMeta = (struct metadata*) ((U32)curr + METADATA_SIZE + size);
                newMeta -> owner = -1;
                newMeta -> next = curr -> next;
                newMeta -> size = blockSize - size - METADATA_SIZE;
                newMeta -> magic = MAGIC_CONSTANT;

                curr -> owner = g_current_tid;
                curr -> next = NULL;
                curr -> size = size + METADATA_SIZE;
                curr -> magic = MAGIC_CONSTANT;

                if (prev == NULL){
                    head = newMeta;
                }
                else {
                    prev -> next = newMeta;
                }
            }
            return (U32*)((U32)curr + METADATA_SIZE); 
        }

        // not enough space 
        prev = curr;
        curr = curr -> next;
    }
    return NULL;
    // if first_free_block is null, return null.
    // else, start at first_free_block. Traverse to next block until one has >= requested size.
    // Update its metadata to mark as allocated. 
    // Update first_free_block if needed. Return pointer to allocated block.
}

int osMemDealloc(void* ptr) {
    // if ptr is null, return RTX_OK
    if (ptr == NULL) {
        return RTX_OK;
    }
    ptr = (struct metadata*)((U32)ptr - METADATA_SIZE);
    struct metadata* pointer = (struct metadata*) ptr;
    
    
    // check in valid range (ptr should be less than estack - min_size and greater than imageEnd)
    extern U32 _estack;
    extern U32 _Min_Stack_Size;
    extern U32 _img_end;

    U32 heapStart = (U32) &_img_end;
    U32 heapEnd = (U32) &_estack - _Min_Stack_Size;
    U32 ptrU32 = (U32) ptr;

    if (!(ptrU32 >= heapStart && ptrU32 < heapEnd)){
        return RTX_ERR;
    }
    
    // check that pointer is at start of block by checking magic number
	if (pointer -> magic != MAGIC_CONSTANT) {
		return RTX_ERR;
	}

    extern g_current_tid;
    // check that pointer is owned by current tid
    if (pointer -> owner != g_current_tid) {
        return RTX_ERR;
    }
    // Update block to be free and coalesce with adjacent free blocks.
    pointer -> owner = -1;
    pointer -> next = NULL; 

    struct metadata* curr = head;

    if (head == NULL || pointer < head) {
        pointer->next = head;
        head = pointer; 
    } else {
        while (curr != NULL) {
            if (curr->next == NULL || curr->next > pointer) {
                // curr = previous to ptr, so we must update curr->next = ptr            
                if (curr->next) {
                    pointer->next = curr->next;
                }
                curr->next = pointer;
                break;
            }
            curr = curr->next;
        }
    }
    // find blocks to coalesce
    curr = head;
    struct metadata* prev = NULL;
    struct metadata* next = NULL;

    while (curr != NULL) {
        if ((struct metadata*)((U32)curr + curr->size) == pointer ) {
            prev = curr;
        }
        if ((struct metadata*)((U32)pointer + pointer->size) == curr) {
            // pointer | curr
            next = curr;
            break;
        }
        curr = curr->next;
    }
    if (prev != NULL) {
        struct metadata* nxt = pointer->next;
        prev->size += pointer->size;
        pointer->magic = 0;
        prev->next = nxt;
        pointer = prev;
    } if (next != NULL) {
        struct metadata* nxtnxt = next->next;
        pointer->size += next->size;
        next->magic = 0;
        pointer->next = nxtnxt;
    }
    
    return RTX_OK;
}

int kMemCountExtFrag(size_t size) {
    int count = 0;

    struct metadata* curr = head;
    while (curr != NULL) {
        if (curr->size < size) {
            count++;
        }
        curr = curr->next;
    }
    return count;
}

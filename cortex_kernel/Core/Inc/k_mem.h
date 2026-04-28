/*
 * k_mem.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_K_MEM_H_
#define INC_K_MEM_H_
#include "common.h"
#include <stddef.h>

int k_mem_init();

void* k_mem_alloc(size_t size);

int k_mem_dealloc(void* ptr);

int k_mem_count_extfrag(size_t size);

struct metadata {
    task_t owner;
    struct metadata* next;
    size_t size;
    U32 magic;
};

// Kernel functions
int osMemInit();
void* osMemAlloc(size_t size);
int osMemDealloc(void* ptr);
int kMemCountExtFrag(size_t size);

#endif /* INC_K_MEM_H_ */

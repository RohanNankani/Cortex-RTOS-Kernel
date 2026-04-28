/*
 * common.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: If you feel that there are common
 *      C functions corresponding to this
 *      header, then any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */
#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#define SVC_KERNEL_INIT 0
#define SVC_CREATE_TASK 1
#define SVC_KERNEL_START 2
#define SVC_YIELD 3
#define SVC_TASK_INFO 4
#define SVC_GET_TID 5
#define SVC_TASK_EXIT 6
#define SVC_MEM_INIT 7
#define SVC_MEM_ALLOC 8
#define SVC_MEM_DEALLOC 9
#define SVC_MEM_COUNT_EXTFRAG 10
#define SVC_SLEEP 11
#define SVC_PERIOD_YIELD 12
#define SVC_SET_DEADLINE 13
#define SVC_CREATE_DEADLINE_TASK 14

#define RTX_OK   0
#define RTX_ERR -1
#define KERNEL_TID 16

#define MAIN_STACK_SIZE 0x400 
#define MAX_STACK_SIZE 0x4000 
#define STACK_SIZE 0x200
#define TID_NULL 0 //predefined Task ID for the NULL task
#define MAX_TASKS 16 //maximum number of tasks in the system
#define DORMANT 0 //state of terminated task
#define READY 1 //state of task that can be scheduled but is not running
#define RUNNING 2 //state of running task
#define SLEEPING 3 // state of sleeping task

void print_continuously(void);

typedef unsigned int U32;
typedef unsigned short U16;
typedef char U8;
typedef unsigned int task_t;

typedef struct task_control_block{
    void (*ptask)(void* args); //entry address
    U32 stack_high; //starting address of stack (high address)
    task_t tid; //task ID
    U8 state; //task's state
    U16 stack_size; //stack size. Must be a multiple of 8
    //your own fields at the end
    U32 *psp; // saved process stack pointer (top of saved context)
    int default_timeslice; // default time slice for this task in ms
    int time_remaining; // length of time slice for this task in ms BOTH OF THESE ARE RELATIVE
} TCB;

#endif /* INC_COMMON_H_ */

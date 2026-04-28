#include "k_task.h"
#include <stdio.h> //You are permitted to use this library, but currently only printf is implemented. Anything else is up to you!
#include "common.h"
#include "stm32f4xx.h"


TCB g_tcbs[MAX_TASKS];
int g_inited = 0;
task_t g_current_tid = TID_NULL;
int g_started = 0;
int isBlocking = 0; 

void osYield(void) {
    __asm("SVC #3");
}

void null_task_function(void) {
    while (1) {
        // osYield();
	}
}

U32* buildStackFrame(TCB* tcb, U32* stackptr) {
    *(--stackptr) = 1 << 24;
    *(--stackptr) = (U32)(tcb->ptask);
    *(--stackptr) = 0XA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;
    *(--stackptr) = 0xA;

    return stackptr;
}


void k_osKernelInit(void)
{
    // Return if kernel already initialized
    if (g_inited) {
        return;
    }

    // Set all TCBs to dormant values
    for (int i = 0; i < MAX_TASKS; i++) {
        g_tcbs[i].ptask = NULL;
        g_tcbs[i].stack_high = 0;
        g_tcbs[i].tid = (task_t)i;
        g_tcbs[i].state = DORMANT;
        g_tcbs[i].stack_size = 0;
        g_tcbs[i].default_timeslice = 5;
        g_tcbs[i].time_remaining = 5;
        g_tcbs[i].psp = NULL;
    }

    NVIC_SetPriority(PendSV_IRQn, 0xFF); // Test this
    NVIC_SetPriority(SysTick_IRQn, 0x00);

    // Mark kernel initialized
    g_inited = 1;
    g_current_tid = TID_NULL;
}


void osKernelInit(void) {
	__asm volatile("SVC #0" ::: "memory");
}

int osCreateTask(TCB* task) {
    int status;
    __asm volatile (
        "mov r0, %1\n" // put task pointer into R0
        "svc #1\n" // trigger SVC
        "mov %0, r0\n" // move R0 (return value) into C variable
        : "=r"(status) // output is whatever R0 contains after SVC
        : "r"(task) // input is task
        : "r0", "memory" // clobbered register
    );
    osYield();
    return status;
}

int k_osCreateTask(TCB* task) {

    // If kernel hasn't been inited or task is NULL, return error
    if (!g_inited || task == NULL){
        return RTX_ERR;
    }

    if (task -> ptask == NULL){
        return RTX_ERR;
    }

    // If not enough stack space, or stack size is invalid, return error
    if (task->stack_size < STACK_SIZE) {
        return RTX_ERR;
    }

    // check if reached max number of tasks

    // Find a free (dormant) block in the TCB array
    int free_tcb_index = -1;

    for (int i = 1; i < MAX_TASKS; i++) {
        if (g_tcbs[i].state == DORMANT) {
            free_tcb_index = i;
            break;
        }
    }

    // Check if there was no free tcb found
    if (free_tcb_index == -1) {
        return RTX_ERR;
    }
    
    int temp = g_current_tid;
    g_current_tid = free_tcb_index;
    U32* stack_low = osMemAlloc(task->stack_size);
    g_current_tid = temp;

    if (stack_low == NULL) {
        return RTX_ERR;
    }
    U32* stack_high = (U32*)((U32)stack_low + task -> stack_size);

    // Copy the TCB into kernel array 
    g_tcbs[free_tcb_index] = *task; 
    g_tcbs[free_tcb_index].tid = free_tcb_index; 
    g_tcbs[free_tcb_index].state = READY;
    g_tcbs[free_tcb_index].time_remaining = 5;
    g_tcbs[free_tcb_index].default_timeslice = 5;

    g_tcbs[free_tcb_index].stack_high = stack_high;
    U32* psp = buildStackFrame(&g_tcbs[free_tcb_index], stack_high);
    g_tcbs[free_tcb_index].psp = psp;
    
    task -> tid = free_tcb_index;

    return RTX_OK;
}

int osKernelStart(void) {
    __asm("SVC #2");

    // Shouldn't reach here but just in case
    return RTX_ERR; 
}

int k_osTaskInfo(task_t tid, TCB* copy){

    // check if kernal is inited, or if the copy ptr is NULL
    if (!g_inited || copy == NULL){
        return RTX_ERR;
    }

    // check if the tid is valid (must be between 0 and MAX_TASKS-1 both inclusive)
    if (tid >= MAX_TASKS){
        return RTX_ERR;
    }

    // check if the task is dormant
    if (g_tcbs[tid].state == DORMANT){
        return RTX_ERR;
    }

    // fill TCB copy
    *copy = g_tcbs[tid];
    return RTX_OK;
}

int osTaskInfo(task_t tid, TCB* task_copy){

    int status;
    __asm volatile (
        "mov r0, %1\n" // put tid into R0
        "mov r1, %2\n" // put task_copy pointer into R1
        "SVC #4\n" // trigger SVC
        "mov %0, r0\n" // move R0 (return value) into variable
        : "=r"(status) // output (status is whatever R0 contains after SVC)
        : "r"(tid), // input (tid)
          "r"(task_copy) // input (task_copy)
        : "r0", "r1", "memory" // clobbered registers
    );

    return status;
}

task_t osGetTID(void) {
	int current_tid;
    __asm volatile (
        "svc #5\n"
        "mov %0, r0\n"
        : "=r"(current_tid)
        :
        : "memory"
    );
    return current_tid;
}

task_t k_osGetTID(void) {
	if (!g_started) {
		return 0;
	}
	return g_current_tid;
}

extern void osStartFirstTask(void);

int k_osKernelStart(void) {
    if (!(g_inited) || g_started) {
        return RTX_ERR; 
    }
    g_started = 1;

    U32* stack_low = osMemAlloc(0x400);
    U32* stack_top = (U32*)((U32)stack_low + 0x400);

    // Create NULL task
    g_tcbs[0].ptask = null_task_function;
    g_tcbs[0].stack_high = stack_top;
    g_tcbs[0].tid = 0;
    g_tcbs[0].state = READY;
    g_tcbs[0].stack_size = 0x400;
    g_tcbs[0].default_timeslice = 5;
    g_tcbs[0].time_remaining = 5;

    U32* psp = buildStackFrame(&g_tcbs[0], stack_top);
    g_tcbs[0].psp = psp;

    SysTick->VAL = 0;
    osStartFirstTask();
    // Shouldn't reach here but just in case
    return RTX_ERR;
}

void k_yield(void){
    if (!g_started) {
        return;
    }
    g_tcbs[g_current_tid].time_remaining = g_tcbs[g_current_tid].default_timeslice;
    g_tcbs[g_current_tid].state = READY;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; 
}

int osTaskExit(void){
    __asm("SVC #6");

    // Shouldn't reach here but just in case
    return RTX_ERR;
}

int k_task_exit(void){

    if (!g_started || g_current_tid == TID_NULL) {
        return RTX_ERR;
    }

    g_tcbs[g_current_tid].state = DORMANT;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; 
    return RTX_ERR;
}

void osSleep(int timeInMs) {
    int status;
    __asm volatile (
        "mov r0, %1\n" // put task pointer into R0
        "svc #11\n" // trigger SVC
        "mov %0, r0\n" // move R0 (return value) into C variable
        : "=r"(status) // output is whatever R0 contains after SVC
        : "r"(timeInMs) // input is task
        : "r0", "memory" // clobbered register
    );
}

void k_os_sleep(int timeInMs) {   
    if (!g_started) {
        return;
    }

    // sleep the current task
    g_tcbs[g_current_tid].state = SLEEPING;
    g_tcbs[g_current_tid].time_remaining = timeInMs;


    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; 
    
}

void osPeriodYield() {
    __asm("SVC #12");
}

void k_os_period_yield() {
    if (!g_started) {
        return;
    }

    // sleep the current task
    g_tcbs[g_current_tid].state = SLEEPING;

    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; 
}

int osSetDeadline(int deadline, task_t TID) {
    int status;
    __asm volatile (
        "mov r0, %1\n" // put tid into R0
        "mov r1, %2\n" // put task_copy pointer into R1
        "SVC #13\n" // trigger SVC
        "mov %0, r0\n" // move R0 (return value) into variable
        : "=r"(status) // output (status is whatever R0 contains after SVC)
        : "r"(deadline), // input (tid)
          "r"(TID) // input (TID)
        : "r0", "r1", "memory" // clobbered registers
    );
    return status;
}
int k_os_set_deadline(int deadline, task_t TID) {
    if (g_current_tid == TID) {
        return RTX_ERR;
    }
    if (deadline <= 0) {
        return RTX_ERR;
    }

    if (g_tcbs[TID].state != READY) {
        return RTX_ERR;
    }

    isBlocking = 1;
    g_tcbs[TID].time_remaining = deadline;
    g_tcbs[TID].default_timeslice = deadline;
    isBlocking = 0; // Very sus
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; 
    return RTX_OK;
}

int osCreateDeadlineTask(int deadline, TCB* task) {
    int status;
    __asm volatile (
        "mov r0, %1\n" // put tid into R0
        "mov r1, %2\n" // put task_copy pointer into R1
        "SVC #14\n" // trigger SVC
        "mov %0, r0\n" // move R0 (return value) into variable
        : "=r"(status) // output (status is whatever R0 contains after SVC)
        : "r"(deadline), // input (tid)
          "r"(task) // input (task)
        : "r0", "r1", "memory" // clobbered registers
    );

    return status;
}
int k_os_create_deadline_task(int deadline, TCB* task) {
    
    // If kernel hasn't been inited or task is NULL, return error
    if (!g_inited || task == NULL){
        return RTX_ERR;
    }

    if (task -> ptask == NULL){
        return RTX_ERR;
    }

    // If not enough stack space, or stack size is invalid, return error
    if (task->stack_size < STACK_SIZE) {
        return RTX_ERR;
    }

    // Deadline cannot be less than or equal to zero
    if (deadline <= 0) {
        return RTX_ERR;
    }

    // check if reached max number of tasks

    // Find a free (dormant) block in the TCB array
    int free_tcb_index = -1;

    for (int i = 1; i < MAX_TASKS; i++) {
        if (g_tcbs[i].state == DORMANT) {
            free_tcb_index = i;
            break;
        }
    }

    // Check if there was no free tcb found
    if (free_tcb_index == -1) {
        return RTX_ERR;
    }
    
    int temp = g_current_tid;
    g_current_tid = free_tcb_index;
    U32* stack_low = osMemAlloc(task->stack_size);
    g_current_tid = temp;

    if (stack_low == NULL) {
        return RTX_ERR;
    }
    U32* stack_high = (U32*)((U32)stack_low + task -> stack_size);

    // Copy the TCB into kernel array 
    g_tcbs[free_tcb_index] = *task; 
    g_tcbs[free_tcb_index].tid = free_tcb_index; 
    g_tcbs[free_tcb_index].state = READY;
    g_tcbs[free_tcb_index].time_remaining = deadline;
    g_tcbs[free_tcb_index].default_timeslice = deadline;

    g_tcbs[free_tcb_index].stack_high = stack_high;
    U32* psp = buildStackFrame(&g_tcbs[free_tcb_index], stack_high);
    g_tcbs[free_tcb_index].psp = psp;
    
    task -> tid = free_tcb_index;

    return RTX_OK;
}

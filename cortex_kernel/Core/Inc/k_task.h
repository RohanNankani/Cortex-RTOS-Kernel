/*
 * k_task.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_K_TASK_H_
#define INC_K_TASK_H_
#include "stm32f4xx_hal.h"

#include "common.h"

// Kernel Initialization Function
void osKernelInit(void);

// Task Creation Function
int osCreateTask(TCB* task);

// Kernel Start Function
int osKernelStart(void);
int k_osKernelStart(void);

// Co-Operative Yielding Function
void osYield(void);

// Task Information Function
int osTaskInfo(task_t tid, TCB* task_copy);

// Get TID Function
task_t osGetTID(void);

// Function to Exit from Tasks
int osTaskExit(void);

void k_osKernelInit(void);

int k_osCreateTask(TCB* task);

void k_yield(void);

int k_task_exit(void);
task_t k_osGetTID(void);
void osSleep(int timeInMs);
void k_os_sleep(int timeInMs);

void osPeriodYield();
void k_os_period_yield();

int osSetDeadline(int deadline, task_t TID);
int k_os_set_deadline(int deadline, task_t TID);

int osCreateDeadlineTask(int deadline, TCB* task);
int k_os_create_deadline_task(int deadline, TCB* task);

#endif /* INC_K_TASK_H_ */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h> //You are permitted to use this library, but currently only printf is implemented. Anything else is up to you!
#include <stdint.h>
#include "stm32f4xx_it.h"
#include "common.h"
#include "k_task.h"
#include "k_mem.h"
#include <math.h>

// Kernel-internal functions implemented in k_task.c
extern TCB g_tcbs[];
extern task_t g_current_tid;
extern int k_kernel_start(void);
extern void k_yield(void);
extern int k_osTaskInfo(task_t tid, TCB* out);
extern task_t k_get_tid(void);
extern int k_task_exit(void);
extern void k_osKernelInit(void);
extern int k_osCreateTask(TCB* task);
extern int k_osKernelStart();
extern int isBlocking;

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
//void SVC_Handler(void)
//{
//  /* USER CODE BEGIN SVCall_IRQn 0 */
//
//  /* USER CODE END SVCall_IRQn 0 */
//  /* USER CODE BEGIN SVCall_IRQn 1 */
//
//  /* USER CODE END SVCall_IRQn 1 */
//}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

// /**
//   * @brief This function handles Pendable request for system service.
//   */
// PendSV_Handler is in Core/Startup/pendsv_handler.s (pure assembly)



// Called from assembly PendSV_Handler
uint32_t* PendSV_Scheduler(uint32_t* current_psp)
{
  extern TCB g_tcbs[];
  extern task_t g_current_tid; 

  // If there's already a running task (not first run from kernel start)
//  if (g_current_tid != TID_NULL && g_tcbs[g_current_tid].state == RUNNING) {

  g_tcbs[g_current_tid].psp = current_psp;

  if (g_tcbs[g_current_tid].state == DORMANT) {
    osMemDealloc((void*)((U32)g_tcbs[g_current_tid].stack_high - g_tcbs[g_current_tid].stack_size));
  }

  task_t next_tid = TID_NULL;
  int min_deadline = INFINITY;
  
  if (g_tcbs[g_current_tid].state == RUNNING) {
      g_tcbs[g_current_tid].state = READY;
  }
  for (int candidate = 1; candidate < MAX_TASKS; candidate++) {
    if (g_tcbs[candidate].state == READY || g_tcbs[candidate].state == RUNNING) {
      if (g_tcbs[candidate].time_remaining < min_deadline) {
        min_deadline = g_tcbs[candidate].time_remaining;
        next_tid = candidate;
      }
    }
  }

  // Second pass: run null task only if no non-null task is ready.
  if (next_tid == TID_NULL && g_tcbs[TID_NULL].state == READY) {
    next_tid = TID_NULL;
  }

  g_current_tid = next_tid;
  g_tcbs[next_tid].state = RUNNING;
  return g_tcbs[next_tid].psp;

  // TOOD: only run the null task if no otehr tasks are ready
  // first do a full scan of all tasks, if none can be run, then run the null task
}

uint32_t* getInitialPsp()
{
  extern TCB g_tcbs[];
  extern task_t g_current_tid; 

  task_t next_tid = TID_NULL;
  int min_deadline = INFINITY;
  for (int candidate = 1; candidate < MAX_TASKS; candidate++) {
    if (g_tcbs[candidate].state == READY) {
      if (g_tcbs[candidate].time_remaining < min_deadline) {
        min_deadline = g_tcbs[candidate].time_remaining;
        next_tid = candidate;
      }
    }
  }

  // If no user task is ready at startup, fall back to the null task.
  if (next_tid == TID_NULL) {
    next_tid = TID_NULL;
  }

  g_current_tid = next_tid;
  g_tcbs[next_tid].state = RUNNING;
  return g_tcbs[next_tid].psp;
}

/**
  * @brief This function handles System tick timer.
  */

void SVC_Handler_Main(unsigned int* svc_args)
{
	  unsigned int svc_number;
	  void* arg;

	  /*
	  * Stack contains:
	  * r0, r1, r2, r3, r12, r14, the return address and xPSR
	  * First argument (r0) is svc_args[0]
	  */
	  arg = (void *)svc_args[0]; // R0

	  svc_number = ( ( char * )svc_args[ 6 ] )[ -2 ] ;
	  int status;
	  switch( svc_number )
	  {
      case SVC_KERNEL_INIT:
        k_osKernelInit();
        break;
      case SVC_CREATE_TASK:
        TCB* arg = (TCB *)svc_args[0];
        status = k_osCreateTask(arg);
        svc_args[0] = status;
        break;
      case SVC_KERNEL_START:
        k_osKernelStart();
        break;

      case SVC_TASK_INFO:
        task_t tid = (task_t)svc_args[0]; // extract args
        TCB* task_copy = (TCB *)svc_args[1];

        status = k_osTaskInfo(tid, task_copy);
        svc_args[0] = status;
        break;
      case SVC_GET_TID:
        task_t running_tid = k_osGetTID();
        svc_args[0] = running_tid;
        break;
	  case SVC_YIELD:
        k_yield();
        break;

      case SVC_TASK_EXIT:
        status = k_task_exit();
        svc_args[0] = status;
        break;
      case SVC_MEM_INIT:
        int result = osMemInit();
        svc_args[0] = result;
        break;
      case SVC_MEM_ALLOC:
        size_t size = (size_t)svc_args[0];
        void * ptr = osMemAlloc(size);
        svc_args[0] = ptr;
        break;
      case SVC_MEM_DEALLOC:
        void* pointer = (void*)svc_args[0];
        int status = osMemDealloc(pointer);
        svc_args[0] = status;
        break;
      case SVC_MEM_COUNT_EXTFRAG:
        size_t size2 = (size_t)svc_args[0];
        int count = kMemCountExtFrag(size2);
        svc_args[0] = count;
        break;
      case SVC_SLEEP:
        int timeInMs = (int)svc_args[0];
        k_os_sleep(timeInMs);
        break;
      case SVC_PERIOD_YIELD:
    	  k_os_period_yield();
    	  break;
      case SVC_SET_DEADLINE:
    	  int deadline = (int)svc_args[0];
    	  task_t TID = (task_t)svc_args[1];
    	  status = k_os_set_deadline(deadline, TID);
    	  svc_args[0] = status;
    	  break;
      case SVC_CREATE_DEADLINE_TASK:
        int deadline2 = (int)svc_args[0];
        TCB* task = (TCB*)svc_args[1];
        status = k_os_create_deadline_task(deadline2, task);
        svc_args[0] = status;
        break; 
      default:    /* unknown SVC */
        break;
    }


}


void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  /* USER CODE END SysTick_IRQn 1 */
  if (isBlocking == 1) {
    return;
  }

  //
  extern int g_started;
  if (!g_started) return;
  for (int i = 0; i < MAX_TASKS; i++) {
    g_tcbs[i].time_remaining -= 1;
  }

  for (int i = 0; i < MAX_TASKS; i++) {
    if (g_tcbs[i].time_remaining <= 0) {
      if (g_tcbs[i].state == SLEEPING || g_tcbs[i].state == RUNNING) {
        g_tcbs[i].state = READY;
        g_tcbs[i].time_remaining = g_tcbs[i].default_timeslice; 
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; 
      }
    }
  }
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

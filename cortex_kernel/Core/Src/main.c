#include "main.h"
#include <stdio.h>
#include "common.h"
#include "k_task.h"
#include "k_mem.h"

volatile int g_A_counter = 0;
volatile int g_B_counter = 0;

static void TaskA(void *args) { // 7
  (void)args;
  // Intentionally never yields. The kernel should pre-empt this task
  // when its deadline is reached.
  while (1) {
    g_A_counter++;
  }
}

static void TaskB(void *args) { // 1
  (void)args;
  printf("---- test2 ----\r\n");
  printf("A0 =%d, B0 =%d\r\n", g_A_counter, g_B_counter);

  for (int i = 1; i <= 4; i++) {
    osPeriodYield();
    g_B_counter++;
    printf("A%d =%d, B%d =%d\r\n", i, g_A_counter, i, g_B_counter);
  }

  while (1) {
    osPeriodYield();
  }
}


int main(void)
{

  /* MCU Configuration: Don't change this or the whole chip won't work!*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* MCU Configuration is now complete. Start writing your code below this line */


  osKernelInit();
  k_mem_init();
  TCB st_mytask;
  st_mytask.stack_size = STACK_SIZE;
  st_mytask.ptask = &TaskB;
  osCreateDeadlineTask(1, &st_mytask);

  st_mytask.ptask = &TaskA;
  osCreateDeadlineTask(7, &st_mytask);

  osKernelStart();
}


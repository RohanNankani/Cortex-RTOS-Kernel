# Cortex-RTOS-Kernel — A Real-Time Executive for the ARM Cortex-M4

A small real-time kernel written from scratch for the STM32F401RE / STM32F411RE
(ARM Cortex-M4) running on the ST Nucleo-64 board. The kernel supports
multitasking with separate per-task stacks, a dynamic memory allocator, and a
pre-emptive Earliest-Deadline-First (EDF) scheduler.

The project is built with STM32CubeIDE. Open the `cortex_kernel/` folder in the IDE and
build/flash from there. Test results from a previous run live in
`test_results/test_report.md`.

## Layout

```
cortex_kernel/
  Core/Inc/        common.h, k_task.h, k_mem.h, ...
  Core/Src/        k_task.c, k_mem.c, main.c, util.c, stm32f4xx_it.c
  Core/Startup/    pendsv_handler.s, svc_handler.s, startup_stm32f401retx.s
  Drivers/         STM32 HAL + CMSIS
  STM32F401RETX_FLASH.ld    linker script (defines _img_end, _estack, etc.)
test_results/
  test_report.md   expected vs. actual serial output for the test programs
```

## What's implemented

The kernel was built up in three stages. Each stage builds on the previous one.

### Stage 1 — Cooperative multitasking

- A fixed-capacity Task Control Block (TCB) table with up to `MAX_TASKS`
  entries. Each TCB carries the entry function, stack high address, stack
  size, TID, and state (`DORMANT` / `READY` / `RUNNING` / `SLEEPING`).
- `osKernelInit` sets up the TCB table and reserves TID 0 for a null task that
  runs whenever there is nothing else to do.
- `osCreateTask` registers a new task and assigns the lowest free TID.
  `osTaskExit` returns the task's TID/stack to the kernel for reuse.
- `osKernelStart` switches to unprivileged mode on the PSP and jumps into the
  first task.
- `osYield` triggers a context switch through PendSV. The currently-running
  task's registers are pushed to its own stack, the scheduler picks the next
  task, and the next task's context is popped off its stack.
- All kernel entry points go through SVC. Application code traps into the
  kernel; the SVC handler dispatches based on the SVC number. This keeps
  kernel state isolated from a future timer interrupt.
- Context-switch core is in `Core/Startup/pendsv_handler.s` (assembly) so that
  saving/restoring registers does not get clobbered by the C ABI. The
  scheduler itself is a regular C function called via `BL` from the handler.

### Stage 2 — Dynamic memory management

- The heap lives between `_img_end` (defined in the linker script) and
  `_estack - _Min_Stack_Size`. `k_mem_init` walks that range and writes the
  initial free-block header.
- `k_mem_alloc(size)` does a first-fit search over the free list, splits the
  block if there is leftover space, and returns a pointer past the metadata.
  All allocations are 4-byte aligned, with an 8-byte header per block storing
  size, free/allocated flag, owner TID, and freelist links.
- `k_mem_dealloc(ptr)` validates the header, refuses to free a block owned by
  a different task, and immediately coalesces with adjacent free neighbours
  (both directions) so the free list stays compact.
- `k_mem_count_extfrag(size)` reports how many free blocks are strictly
  smaller than `size` — a quick way to measure external fragmentation from a
  test program.

### Stage 3 — Pre-emptive EDF scheduling

- TCBs gained a `deadline` field (in ms) and a per-task remaining-time
  counter. SysTick fires every 1 ms and decrements the running task's
  remaining time; when it hits zero the kernel pends a PendSV to re-schedule.
- The scheduler picks the READY task with the earliest deadline, breaking
  ties by lowest TID.
- Stacks are now allocated with `k_mem_alloc` instead of a fixed pool, so
  `osCreateDeadlineTask` honours the caller's `stack_size` request and the
  stack is owned by the new task (so it gets freed on `osTaskExit`).
- `osSleep(ms)` puts a task into the SLEEPING state with a wake-up tick;
  SysTick wakes it when the timer expires.
- `osPeriodYield()` is the periodic-task variant — the kernel computes the
  remaining time in the current period and sleeps the task until the next
  release.
- `osSetDeadline(deadline, tid)` reassigns another task's deadline and, if
  the new deadline is sooner than the caller's, immediately pre-empts the
  caller.
- The `osSetDeadline` and `osCreateDeadlineTask` paths mask interrupts while
  touching scheduler state to avoid races with SysTick.

## API summary

```c
// task management (k_task.h)
void    osKernelInit(void);
int     osKernelStart(void);
int     osCreateTask(TCB *task);
int     osCreateDeadlineTask(int deadline, TCB *task);
void    osYield(void);
void    osSleep(int ms);
void    osPeriodYield(void);
int     osSetDeadline(int deadline, task_t tid);
int     osTaskInfo(task_t tid, TCB *out);
task_t  osGetTID(void);
int     osTaskExit(void);

// memory (k_mem.h)
int     k_mem_init(void);
void   *k_mem_alloc(size_t size);
int     k_mem_dealloc(void *ptr);
int     k_mem_count_extfrag(size_t size);
```

## Building

1. Open STM32CubeIDE and import `cortex_kernel/` as an existing project.
2. Connect a Nucleo-F401RE / F411RE over USB.
3. Run. Open a serial console at 115200-8-N-1 to see output.

The `Debug/` directory is regenerated by the IDE on first build.

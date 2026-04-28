.syntax unified
.cpu cortex-m4
.thumb

.global PendSV_Handler
.global osStartFirstTask
.extern getInitialPsp
.extern PendSV_Scheduler

.section .text

// Called from k_osKernelStart to launch first task (no context to save)
.type osStartFirstTask, %function
osStartFirstTask:

    BL      getInitialPsp
    
    LDMIA   R0!, {R4-R11}       
    MSR     PSP, R0             
    
    MOV     LR, #0xFFFFFFFD     
    BX      LR

// Called via PendSV interrupt for context switching (saves current context)
.type PendSV_Handler, %function
PendSV_Handler:
    CPSID   I 
    MRS     R0, PSP             
    STMDB   R0!, {R4-R11}       

    BL      PendSV_Scheduler    

    LDMIA   R0!, {R4-R11}       
    MSR     PSP, R0             

    CPSIE   I
    MOV     LR, #0xFFFFFFFD     
    BX      LR                  

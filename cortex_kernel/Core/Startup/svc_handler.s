  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb
  .type  SVC_Handler, %function

  .global SVC_Handler

  SVC_Handler:
  TST lr, #4
  ITE EQ
  MRSEQ r0, MSP
  MRSNE r0, PSP
  B SVC_Handler_Main




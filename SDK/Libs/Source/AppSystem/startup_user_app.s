/**
  ******************************************************************************
  * @file      startup_stm32u575xx.s
  * @author    MCD Application Team
  * @brief     STM32U575xx devices vector table GCC toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address,
  *                - Configure the clock system
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Cortex-M33 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  *******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *******************************************************************************
  */

  .syntax unified
	.cpu cortex-m33
	.fpu softvfp
	.thumb

.global AppStart

/* start address for the .bss section. defined in linker script */
.extern 	_sbss
/* end address for the .bss section. defined in linker script */
.extern 	_ebss

/* Declare external function exit */
.extern     exit

.section  .AppStart
.type  AppStart, %function
AppStart:
    mov r9, r0       /* set GOT table address to r9. The value will passed as argument to the function */

/* Zero fill the bss segment. */
	ldr	r2, =_sbss
	b	LoopFillZerobss
FillZerobss:
	movs	r3, #0
	str	r3, [r2], #4

LoopFillZerobss:
	ldr	r3, = _ebss
	cmp	r2, r3
	bcc	FillZerobss

/* Call static constructors */
    bl  __una_init_array

/* Call to check the kernel pointer */
    bl una_check_kernel

/* Call the application's entry point.*/
	bl	main
	push {r0}

/* Call static destructors */
	bl  __una_fini_array

/* Call the exit function with status -1 */
	pop {r0}
    bl  exitA

    bx  lr

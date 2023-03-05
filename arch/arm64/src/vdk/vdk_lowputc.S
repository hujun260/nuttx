/****************************************************************************
 * arch/arm64/src/vdk/vdk_lowputc.S
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************
 *
 *    DESCRIPTION
 *       Wrapper for early printk
 *
 ***************************************************************************/

#include <nuttx/config.h>

#include "arm64_macro.inc"

/****************************************************************************
 * Public Symbols
 ****************************************************************************/

    .file    "vdk_lowputc.S"

/****************************************************************************
 * Assembly Macros
 ****************************************************************************/

 /* 32-bit register definition for vdk 16550 uart */

#define UART_BASE_ADDRESS CONFIG_16550_UART0_BASE

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* U16550 UART wait the controller ready to transmit
 * xb: register which contains the UART base address
 * c: scratch register number
 */

.macro early_uart_ready xb, wt
1:
    ldrb  \wt, [\xb, #5]      /* UART_LSR_OFFSET */
    tbz   \wt, #5, 1b         /* Check BUSY bit */
.endm

/* U16550 UART transmit character
 * xb: register which contains the UART base address
 * wt: register which contains the character to transmit
 */

.macro early_uart_transmit xb, wt
    strb  \wt, [\xb]             /* -> UARTDR (Data Register) */
.endm

/* Print a character on the UART - this function is called by C
 * x0: character to print
 *
 * disassemble code is:
 * u16550_putc(priv, '\r');
 *    5d0:	f9400022 	ldr	x2, [x1]
 * return *((FAR volatile uart_datawidth_t *)priv->uartbase + offset);
 *    5d4:	39401444 	ldrb	w4, [x2, #5]
 * while ((u16550_serialin(priv, UART_LSR_OFFSET) & UART_LSR_THRE) == 0);
 *    5d8:	362fffe4 	tbz	w4, #5, 5d4 <up_putc+0x2c>
 * *((FAR volatile uart_datawidth_t *)priv->uartbase + offset) = value;
 *    5dc:	528001a4 	mov	w4, #0xd                   	// #13
 *    5e0:	39000044 	strb	w4, [x2]
 */

GTEXT(arm64_lowputc)
SECTION_FUNC(text, arm64_lowputc)
    ldr   x15, =UART_BASE_ADDRESS
    early_uart_ready x15, w2
    early_uart_transmit x15, w0
    ret

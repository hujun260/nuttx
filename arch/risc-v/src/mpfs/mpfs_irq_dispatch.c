/****************************************************************************
 * arch/risc-v/src/mpfs/mpfs_irq_dispatch.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <assert.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <arch/board/board.h>

#include "riscv_arch.h"
#include "riscv_internal.h"

#include "group/group.h"
#include "hardware/mpfs_memorymap.h"
#include "hardware/mpfs_plic.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern void up_fault(int irq, uint64_t *regs);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * riscv_dispatch_irq
 ****************************************************************************/

void *riscv_dispatch_irq(uint64_t vector, uint64_t *regs)
{
  uint32_t irq = (vector & 0x3f);
  uint64_t *mepc = regs;

  board_autoled_on(LED_INIRQ);

  /* Check if fault happened  */

  if (vector < RISCV_IRQ_ECALLU ||
      vector == RISCV_IRQ_INSTRUCTIONPF ||
      vector == RISCV_IRQ_LOADPF ||
      vector == RISCV_IRQ_SROREPF ||
      vector == RISCV_IRQ_RESERVED)
    {
      up_fault((int)irq, regs);
    }

  if (vector & 0x8000000000000000)
    {
       irq += MPFS_IRQ_ASYNC;
    }

  /* Firstly, check if the irq is machine external interrupt */

  uint64_t hart_id = READ_CSR(mhartid);
  uintptr_t claim_address;

  if (hart_id == 0)
    {
      claim_address = MPFS_PLIC_H0_MCLAIM;
    }
  else
    {
      claim_address = MPFS_PLIC_H1_MCLAIM +
        ((hart_id - 1) * MPFS_PLIC_NEXTHART_OFFSET);
    }

  if (irq == RISCV_IRQ_MEXT)
    {
      uint32_t ext = getreg32(claim_address);

      /* Add the value to nuttx irq which is offset to the mext */

      irq = MPFS_IRQ_EXT_START + ext;
    }

  /* NOTE: In case of ecall, we need to adjust mepc in the context */

  if (irq == RISCV_IRQ_ECALLM || irq == RISCV_IRQ_ECALLU)
    {
      *mepc += 4;
    }

  /* Acknowledge the interrupt */

  riscv_ack_irq(irq);

#ifdef CONFIG_SUPPRESS_INTERRUPTS
  PANIC();
#else
  /* Current regs non-zero indicates that we are processing an interrupt;
   * CURRENT_REGS is also used to manage interrupt level context switches.
   *
   * Nested interrupts are not supported
   */

  ASSERT(CURRENT_REGS == NULL);
  CURRENT_REGS = regs;

  /* MEXT means no interrupt */

  if (irq != RISCV_IRQ_MEXT && irq != MPFS_IRQ_INVALID)
    {
      /* Deliver the IRQ */

      irq_dispatch(irq, regs);
    }

  if (irq > MPFS_IRQ_EXT_START)
    {
      /* Then write PLIC_CLAIM to clear pending in PLIC */

      putreg32(irq - MPFS_IRQ_EXT_START, claim_address);
    }

#if defined(CONFIG_ARCH_FPU) || defined(CONFIG_ARCH_ADDRENV)
  /* Check for a context switch.  If a context switch occurred, then
   * CURRENT_REGS will have a different value than it did on entry.  If an
   * interrupt level context switch has occurred, then restore the floating
   * point state and the establish the correct address environment before
   * returning from the interrupt.
   */

  if (regs != CURRENT_REGS)
    {
#ifdef CONFIG_ARCH_FPU
      /* Restore floating point registers */

      riscv_restorefpu((uint64_t *)CURRENT_REGS);
#endif

#ifdef CONFIG_ARCH_ADDRENV
      /* Make sure that the address environment for the previously
       * running task is closed down gracefully (data caches dump,
       * MMU flushed) and set up the address environment for the new
       * thread at the head of the ready-to-run list.
       */

      group_addrenv(NULL);
#endif
    }
#endif

#endif

  /* If a context switch occurred while processing the interrupt then
   * CURRENT_REGS may have change value.  If we return any value different
   * from the input regs, then the lower level will know that a context
   * switch occurred during interrupt processing.
   */

  regs = (uint64_t *)CURRENT_REGS;
  CURRENT_REGS = NULL;

  board_autoled_off(LED_INIRQ);

  return regs;
}

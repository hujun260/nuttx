/****************************************************************************
 * arch/xtensa/src/esp32s3/esp32s3_cpustart.c
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

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/sched.h>
#include <nuttx/sched_note.h>
#include <nuttx/spinlock.h>
#include <sched/sched.h>

#include "xtensa.h"
#include "esp32s3_irq.h"
#include "esp32s3_region.h"
#include "esp32s3_smp.h"
#include "hardware/esp32s3_rtccntl.h"
#include "hardware/esp32s3_system.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_appcpu_started;
static volatile spinlock_t g_appcpu_interlock;

/****************************************************************************
 * ROM function prototypes
 ****************************************************************************/

extern void ets_set_appcpu_boot_addr(uint32_t start);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: xtensa_attach_fromcpu0_interrupt
 ****************************************************************************/

static inline void xtensa_attach_fromcpu0_interrupt(void)
{
  int cpuint;

  /* Connect all CPU peripheral source to allocated CPU interrupt */

  cpuint = esp32s3_setup_irq(1, ESP32S3_PERIPH_INT_FROM_CPU0, 1,
                             ESP32S3_CPUINT_LEVEL);
  DEBUGASSERT(cpuint >= 0);

  /* Attach the inter-CPU interrupt. */

  irq_attach(ESP32S3_IRQ_INT_FROM_CPU0, (xcpt_t)esp32s3_fromcpu0_interrupt,
             NULL);

  /* Enable the inter 0 CPU interrupts. */

  up_enable_irq(ESP32S3_IRQ_INT_FROM_CPU0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: xtensa_appcpu_start
 *
 * Description:
 *   This is the entry point used for the APP CPU when it's started  via
 *   up_cpu_start().  The actual start-up logic is in ROM and we boot up
 *   in C code.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None, does not return
 *
 ****************************************************************************/

void xtensa_appcpu_start(void)
{
  struct tcb_s *tcb = this_task();
  register uint32_t sp;

  /* Move to the stack assigned to us by up_smp_start immediately.  Although
   * we were give a stack pointer at start-up, we don't know where that stack
   * pointer is positioned respect to our memory map.  The only safe option
   * is to switch to a well-known IDLE thread stack.
   */

  sp = (uint32_t)tcb->stack_base_ptr + tcb->adj_stack_size;
  __asm__ __volatile__("mov sp, %0\n" : : "r"(sp));

  sinfo("CPU%d Started\n", up_cpu_index());

#ifdef CONFIG_SCHED_INSTRUMENTATION
  /* Notify that this CPU has started */

  sched_note_cpu_started(tcb);
#endif

  /* Release the spinlock to signal to the PRO CPU that the APP CPU has
   * started.
   */

  g_appcpu_started = true;
  spin_unlock(&g_appcpu_interlock);

  /* Reset scheduler parameters */

  nxsched_resume_scheduler(tcb);

  /* Move CPU0 exception vectors to IRAM */

  __asm__ __volatile__ ("wsr %0, vecbase\n"::"r" (&_init_start));

  /* Make page 0 access raise an exception */

  esp32s3_region_protection();

  /* Initialize CPU interrupts */

  esp32s3_cpuint_initialize();

  /* Attach and enable the inter-CPU interrupt */

  xtensa_attach_fromcpu0_interrupt();

  /* Enable the software interrupt */

  up_enable_irq(XTENSA_IRQ_SWINT);

#ifndef CONFIG_SUPPRESS_INTERRUPTS
  /* And Enable interrupts */

  up_irq_enable();
#endif

  /* Then switch contexts. This instantiates the exception context of the
   * tcb at the head of the assigned task list.  In this case, this should
   * be the CPUs NULL task.
   */

  xtensa_context_restore(tcb->xcp.regs);
}

/****************************************************************************
 * Name: up_cpu_start
 *
 * Description:
 *   In an SMP configuration, only one CPU is initially active (CPU 0).
 *   System initialization occurs on that single thread. At the completion of
 *   the initialization of the OS, just before beginning normal multitasking,
 *   the additional CPUs would be started by calling this function.
 *
 *   Each CPU is provided the entry point to its IDLE task when started.  A
 *   TCB for each CPU's IDLE task has been initialized and placed in the
 *   CPU's g_assignedtasks[cpu] list.  No stack has been allocated or
 *   initialized.
 *
 *   The OS initialization logic calls this function repeatedly until each
 *   CPU has been started, 1 through (CONFIG_SMP_NCPUS-1).
 *
 * Input Parameters:
 *   cpu - The index of the CPU being started.  This will be a numeric
 *         value in the range of one to (CONFIG_SMP_NCPUS-1).
 *         (CPU 0 is already active)
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int up_cpu_start(int cpu)
{
  DEBUGASSERT(cpu >= 0 && cpu < CONFIG_SMP_NCPUS && cpu != this_cpu());

  if (!g_appcpu_started)
    {
      uint32_t regval;

      /* Start CPU1 */

      sinfo("Starting CPU%d\n", cpu);

#ifdef CONFIG_SCHED_INSTRUMENTATION
      /* Notify of the start event */

      sched_note_cpu_start(this_task(), cpu);
#endif

      /* This spinlock will be used as a handshake between the two CPUs.
       * It's first initialized to its locked state, later the PRO CPU will
       * try to lock it but spins until the APP CPU starts and unlocks it.
       */

      spin_initialize(&g_appcpu_interlock, SP_LOCKED);

      /* Unstall the APP CPU */

      regval  = getreg32(RTC_CNTL_RTC_SW_CPU_STALL_REG);
      regval &= ~RTC_CNTL_SW_STALL_APPCPU_C1_M;
      putreg32(regval, RTC_CNTL_RTC_SW_CPU_STALL_REG);

      regval  = getreg32(RTC_CNTL_RTC_OPTIONS0_REG);
      regval &= ~RTC_CNTL_SW_STALL_APPCPU_C0_M;
      putreg32(regval, RTC_CNTL_RTC_OPTIONS0_REG);

      /* Enable clock gating for the APP CPU */

      regval  = getreg32(SYSTEM_CORE_1_CONTROL_0_REG);
      regval |= SYSTEM_CONTROL_CORE_1_CLKGATE_EN;
      putreg32(regval, SYSTEM_CORE_1_CONTROL_0_REG);

      regval  = getreg32(SYSTEM_CORE_1_CONTROL_0_REG);
      regval &= ~SYSTEM_CONTROL_CORE_1_RUNSTALL;
      putreg32(regval, SYSTEM_CORE_1_CONTROL_0_REG);

      /* Reset the APP CPU */

      regval  = getreg32(SYSTEM_CORE_1_CONTROL_0_REG);
      regval |= SYSTEM_CONTROL_CORE_1_RESETING;
      putreg32(regval, SYSTEM_CORE_1_CONTROL_0_REG);

      regval  = getreg32(SYSTEM_CORE_1_CONTROL_0_REG);
      regval &= ~SYSTEM_CONTROL_CORE_1_RESETING;
      putreg32(regval, SYSTEM_CORE_1_CONTROL_0_REG);

      /* Set the CPU1 start address */

      ets_set_appcpu_boot_addr((uint32_t)xtensa_appcpu_start);

      /* And wait until the APP CPU starts and releases the spinlock. */

      spin_lock(&g_appcpu_interlock);
      DEBUGASSERT(g_appcpu_started);
    }

  return OK;
}


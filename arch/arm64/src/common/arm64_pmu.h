/****************************************************************************
 * arch/arm64/src/common/arm64_pmu.h
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

#ifndef __ARCH_ARM64_SRC_COMMON_ARM64_PMU_H
#define __ARCH_ARM64_SRC_COMMON_ARM64_PMU_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "arm64_arch.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* PMCCFILTR_EL0 */

#define PMCCFILTR_EL0_P          (1ul << 31)  /* Privileged filtering bit */
#define PMCCFILTR_EL0_U          (1ul << 30)  /* User filtering bit */
#define PMCCFILTR_EL0_NSK        (1ul << 29)  /* Non-secure EL1 (kernel) modes filtering bit */
#define PMCCFILTR_EL0_NSU        (1ul << 28)  /* Non-secure EL0 (Unprivileged) filtering */
#define PMCCFILTR_EL0_NSH        (1ul << 27)  /* Non-secure EL2 (Hypervisor) filtering bit */
#define PMCCFILTR_EL0_M          (1ul << 26)  /* Secure EL3 filtering bit */

/* PMCNTENCLR_EL0 */

#define PMCNTENCLR_EL0_C         (1ul << 31)  /* PMCCNTR_EL0 disable bit */

/* PMCNTENSET_EL0 */

#define PMCNTENSET_EL0_C         (1ul << 31)  /* Enables the cycle counter register */

/* PMCR_EL0 */

#define PMCR_EL0_LC              (1ul << 6)   /* Long cycle counter enable */
#define PMCR_EL0_DP              (1ul << 5)   /* Disable cycle counter when event counting is prohibited */
#define PMCR_EL0_X               (1ul << 4)   /* Enable export of events */
#define PMCR_EL0_D               (1ul << 3)   /* Clock divider */
#define PMCR_EL0_C               (1ul << 2)   /* Cycle counter reset */
#define PMCR_EL0_P               (1ul << 1)   /* Event counter reset */
#define PMCR_EL0_E               (1ul << 0)   /* All counters that are accessible at Non-secure EL1 are enabled by PMCNTENSET_EL0 */

/* PMINTENCLR_EL1 */

#define PMINTENCLR_EL1_C         (1ul << 31)  /* PMCCNTR_EL0 overflow interrupt request disable bit */

/* PMINTENSET_EL1 */

#define PMINTENSET_EL1_C         (1ul << 31)  /* PMCCNTR_EL0 overflow interrupt request enable bit */

/* PMOVSCLR_EL0 */

#define PMOVSCLR_EL0_C           (1ul << 31)  /* PMCCNTR_EL0 overflow bit */

/* PMSELR_EL0 */

#define PMSELR_EL0_SEL_C         (0x1ful << 0) /* When PMSELR_EL0.SEL is 0b11111, it selects the cycle counter */

/* PMUSERENR_EL0 */

#define PMUSERENR_EL0_ER         (1ul << 3)    /* Event counter read trap control */
#define PMUSERENR_EL0_CR         (1ul << 2)    /* Cycle counter read trap control */
#define PMUSERENR_EL0_SW         (1ul << 1)    /* Software Increment write trap control */
#define PMUSERENR_EL0_EN         (1ul << 0)    /* Software can access all PMU registers at EL0 */

/* PMEVTYPER<n>_EL0 */

#define PMEVTYPERN_EL0_P         (1ul << 31)   /* Privileged filtering bit */
#define PMEVTYPERN_EL0_U         (1ul << 30)   /* User filtering bit */
#define PMEVTYPERN_EL0_NSK       (1ul << 29)   /* Non-secure EL1 (kernel) modes filtering bit */
#define PMEVTYPERN_EL0_NSU       (1ul << 28)   /* Non-secure EL0 (Unprivileged) filtering bit */
#define PMEVTYPERN_EL0_NSH       (1ul << 27)   /* EL2 (Hypervisor) filtering bit */
#define PMEVTYPERN_EL0_M         (1ul << 26)   /* Secure EL3 filtering bit */
#define PMEVTYPERN_EL0_MT        (1ul << 25)   /* Multithreading */

/* Common architectural and microarchitectural event numbers. */

#define PMUV3_EVNUM_SW_INCR                 0x0000
#define PMUV3_EVNUM_L1I_CACHE_REFILL        0x0001
#define PMUV3_EVNUM_L1I_TLB_REFILL          0x0002
#define PMUV3_EVNUM_L1D_CACHE_REFILL        0x0003
#define PMUV3_EVNUM_L1D_CACHE               0x0004
#define PMUV3_EVNUM_L1D_TLB_REFILL          0x0005
#define PMUV3_EVNUM_LD_RETIRED              0x0006
#define PMUV3_EVNUM_ST_RETIRED              0x0007
#define PMUV3_EVNUM_INST_RETIRED            0x0008
#define PMUV3_EVNUM_EXC_TAKEN               0x0009
#define PMUV3_EVNUM_EXC_RETURN              0x000A
#define PMUV3_EVNUM_CID_WRITE_RETIRED       0x000B
#define PMUV3_EVNUM_PC_WRITE_RETIRED        0x000C
#define PMUV3_EVNUM_BR_IMMED_RETIRED        0x000D
#define PMUV3_EVNUM_BR_RETURN_RETIRED       0x000E
#define PMUV3_EVNUM_UNALIGNED_LDST_RETIRED  0x000F
#define PMUV3_EVNUM_BR_MIS_PRED             0x0010
#define PMUV3_EVNUM_CPU_CYCLES              0x0011
#define PMUV3_EVNUM_BR_PRED                 0x0012
#define PMUV3_EVNUM_MEM_ACCESS              0x0013
#define PMUV3_EVNUM_L1I_CACHE               0x0014
#define PMUV3_EVNUM_L1D_CACHE_WB            0x0015
#define PMUV3_EVNUM_L2D_CACHE               0x0016
#define PMUV3_EVNUM_L2D_CACHE_REFILL        0x0017
#define PMUV3_EVNUM_L2D_CACHE_WB            0x0018
#define PMUV3_EVNUM_BUS_ACCESS              0x0019
#define PMUV3_EVNUM_MEMORY_ERROR            0x001A
#define PMUV3_EVNUM_INST_SPEC               0x001B
#define PMUV3_EVNUM_TTBR_WRITE_RETIRED      0x001C
#define PMUV3_EVNUM_BUS_CYCLES              0x001D
#define PMUV3_EVNUM_CHAIN                   0x001E
#define PMUV3_EVNUM_L1D_CACHE_ALLOCATE      0x001F
#define PMUV3_EVNUM_L2D_CACHE_ALLOCATE      0x0020
#define PMUV3_EVNUM_BR_RETIRED              0x0021
#define PMUV3_EVNUM_BR_MIS_PRED_RETIRED     0x0022
#define PMUV3_EVNUM_STALL_FRONTEND          0x0023
#define PMUV3_EVNUM_STALL_BACKEND           0x0024
#define PMUV3_EVNUM_L1D_TLB                 0x0025
#define PMUV3_EVNUM_L1I_TLB                 0x0026
#define PMUV3_EVNUM_L2I_CACHE               0x0027
#define PMUV3_EVNUM_L2I_CACHE_REFILL        0x0028
#define PMUV3_EVNUM_L3D_CACHE_ALLOCATE      0x0029
#define PMUV3_EVNUM_L3D_CACHE_REFILL        0x002A
#define PMUV3_EVNUM_L3D_CACHE               0x002B
#define PMUV3_EVNUM_L3D_CACHE_WB            0x002C
#define PMUV3_EVNUM_L2D_TLB_REFILL          0x002D
#define PMUV3_EVNUM_L2I_TLB_REFILL          0x002E
#define PMUV3_EVNUM_L2D_TLB                 0x002F
#define PMUV3_EVNUM_L2I_TLB                 0x0030
#define PMUV3_EVNUM_REMOTE_ACCESS           0x0031
#define PMUV3_EVNUM_LL_CACHE                0x0032
#define PMUV3_EVNUM_LL_CACHE_MISS           0x0033
#define PMUV3_EVNUM_DTLB_WALK               0x0034
#define PMUV3_EVNUM_ITLB_WALK               0x0035
#define PMUV3_EVNUM_LL_CACHE_RD             0x0036
#define PMUV3_EVNUM_LL_CACHE_MISS_RD        0x0037
#define PMUV3_EVNUM_REMOTE_ACCESS_RD        0x0038
#define PMUV3_EVNUM_L1D_CACHE_LMISS_RD      0x0039
#define PMUV3_EVNUM_OP_RETIRED              0x003A
#define PMUV3_EVNUM_OP_SPEC                 0x003B
#define PMUV3_EVNUM_STALL                   0x003C
#define PMUV3_EVNUM_STALL_SLOT_BACKEND      0x003D
#define PMUV3_EVNUM_STALL_SLOT_FRONTEND     0x003E
#define PMUV3_EVNUM_STALL_SLOT              0x003F

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* ID corresponds to the number <n> in the event counter PMEVCNTR<n>_EL0 */

enum pmu_event_id
{
  PMU_EVENT_ID_INST_RETIRED,
  PMU_EVENT_ID_MAX,
};

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pmu_evcntr_read_ceid0
 *
 * Description:
 *   Read common Event Identification register 0.
 *
 * Return Value:
 *   The value of the pmceid0_el0 register
 *
 ****************************************************************************/

static inline uint64_t pmu_evcntr_read_ceid0(void)
{
  return read_sysreg(pmceid0_el0);
}

/****************************************************************************
 * Name: pmu_evcntr_read_ceid1
 *
 * Description:
 *   Read common Event Identification register 1.
 *
 * Return Value:
 *   The value of the pmceid1_el0 register
 *
 ****************************************************************************/

static inline uint64_t pmu_evcntr_read_ceid1(void)
{
  return read_sysreg(pmceid1_el0);
}

/****************************************************************************
 * Name: pmu_evcntr_softinc
 *
 * Description:
 *   Trigger software counting.
 *   The counter increments on writes to the pmswinc_el0 register.
 *
 * Parameters:
 *   mask - Corresponds to the event counter bit.
 *
 ****************************************************************************/

static inline void pmu_evcntr_softinc(uint64_t mask)
{
  write_sysreg(mask, pmswinc_el0);
}

/****************************************************************************
 * Name: pmu_cntr_ovsclr_config
 *
 * Description:
 *   Clear counter overflow bits.
 *
 * Parameters:
 *   mask - Corresponds to the counter overflow bit to clear.
 *
 ****************************************************************************/

static inline void pmu_cntr_ovsclr_config(uint64_t mask)
{
  write_sysreg(mask, pmovsclr_el0);
}

/****************************************************************************
 * Name: pmu_get_ccntr
 *
 * Description:
 *   Read cycle counter.
 *
 * Return Value:
 *   Cycle count.
 *
 ****************************************************************************/

static inline uint64_t pmu_get_ccntr(void)
{
  return read_sysreg(pmccntr_el0);
}

/****************************************************************************
 * Name: pmu_ccntr_ccfiltr_config
 *
 * Description:
 *   Determines the modes in which the Cycle Counter.
 *
 * Parameters:
 *   mask - Filter flags for Cycle Counters.
 *
 ****************************************************************************/

static inline void pmu_ccntr_ccfiltr_config(uint64_t mask)
{
  write_sysreg(mask, pmccfiltr_el0);
}

/****************************************************************************
 * Name: pmu_cntr_select
 *
 * Description:
 *   Selects the current event counter or the cycle counter.
 *
 * Parameters:
 *   mask - Select counter flag.
 *
 ****************************************************************************/

static inline void pmu_cntr_select(uint64_t mask)
{
  write_sysreg(mask, pmselr_el0);
}

/****************************************************************************
 * Name: pmu_cntr_get_xevtyper
 *
 * Description:
 *   Gets the selected counter type.
 *
 * Return Value:
 *   Select the value of the counter type.
 *
 ****************************************************************************/

static inline uint64_t pmu_cntr_get_xevtyper(void)
{
  return read_sysreg(pmxevtyper_el0);
}

/****************************************************************************
 * Name: pmu_cntr_set_xevtyper
 *
 * Description:
 *   Sets the selected counter type.
 *
 * Parameters:
 *   mask - The value of the type counter.
 *
 ****************************************************************************/

static inline void pmu_cntr_set_xevtyper(uint64_t mask)
{
  write_sysreg(mask, pmxevtyper_el0);
}

/****************************************************************************
 * Name: pmu_cntr_get_xevcntr
 *
 * Description:
 *   Reads the value of the selected counter.
 *
 * Return Value:
 *   Select the value of the counter.
 *
 ****************************************************************************/

static inline uint64_t pmu_cntr_get_xevcntr(void)
{
  return read_sysreg(pmxevcntr_el0);
}

/****************************************************************************
 * Name: pmu_cntr_trap_control
 *
 * Description:
 *   Enables or disables EL0 access to the Performance Monitors.
 *
 * Parameters:
 *   mask - traped caused by operate counters through mask bit control
 *
 ****************************************************************************/

static inline void pmu_cntr_trap_control(uint64_t mask)
{
  write_sysreg(mask, pmuserenr_el0);
}

/****************************************************************************
 * Name: pmu_cntr_control_config
 *
 * Description:
 *   Config counters.
 *
 * Parameters:
 *   mask - Configuration flags for counters.
 *
 ****************************************************************************/

static inline void pmu_cntr_control_config(uint64_t mask)
{
  write_sysreg(mask, pmcr_el0);
}

/****************************************************************************
 * Name: pmu_cntr_enable
 *
 * Description:
 *   Enable counters.
 *
 * Parameters:
 *   mask - Counters to enable.
 *
 * Note:
 *   Enables one or more of the following:
 *     event counters (0-30)
 *     cycle counter
 *
 ****************************************************************************/

static inline void pmu_cntr_enable(uint64_t mask)
{
  write_sysreg(mask, pmcntenset_el0);
}

/****************************************************************************
 * Name: pmu_cntr_irq_enable
 *
 * Description:
 *   Enable counter overflow interrupt request.
 *
 * Parameters:
 *   mask - Counter overflow interrupt request bits to set.
 *
 * Note:
 *   Sets overflow interrupt request bits for one or more of the following:
 *     event counters (0-30)
 *     cycle counter
 *
 ****************************************************************************/

static inline void pmu_cntr_irq_enable(uint64_t mask)
{
  write_sysreg(mask, pmintenset_el1);
}

/****************************************************************************
 * Name: pmu_cntr_irq_disable
 *
 * Description:
 *   Disable counter overflow interrupt request.
 *
 * Parameters:
 *   mask - Counter overflow interrupt request bits to clear.
 *
 * Note:
 *   Sets overflow interrupt request bits for one or more of the following:
 *     event counters (0-30)
 *     cycle counter
 *
 ****************************************************************************/

static inline void pmu_cntr_irq_disable(uint64_t mask)
{
  write_sysreg(mask, pmintenclr_el1);
}

#endif /* __ARCH_ARM64_SRC_COMMON_ARM64_PMU_H */

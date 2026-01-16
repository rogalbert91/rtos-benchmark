/*
 * SPDX-License-Identifier: Apache-2.0
 * Minimal CMSIS-like definitions for Cortex-M33 (RP2350)
 * Only includes what's needed for the benchmark suite.
 */

#ifndef __CORE_CM33_H__
#define __CORE_CM33_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* I/O definitions for compiler */
#define __I  volatile const   /* Read only */
#define __O  volatile         /* Write only */
#define __IO volatile         /* Read/Write */

/* ================== SysTick ================== */

/* SysTick Control/Status Register Definitions */
#define SysTick_CTRL_COUNTFLAG_Pos   16U
#define SysTick_CTRL_COUNTFLAG_Msk   (1UL << SysTick_CTRL_COUNTFLAG_Pos)

#define SysTick_CTRL_CLKSOURCE_Pos   2U
#define SysTick_CTRL_CLKSOURCE_Msk   (1UL << SysTick_CTRL_CLKSOURCE_Pos)

#define SysTick_CTRL_TICKINT_Pos     1U
#define SysTick_CTRL_TICKINT_Msk     (1UL << SysTick_CTRL_TICKINT_Pos)

#define SysTick_CTRL_ENABLE_Pos      0U
#define SysTick_CTRL_ENABLE_Msk      (1UL << SysTick_CTRL_ENABLE_Pos)

/* SysTick Reload Register Definitions */
#define SysTick_LOAD_RELOAD_Pos      0U
#define SysTick_LOAD_RELOAD_Msk      (0xFFFFFFUL << SysTick_LOAD_RELOAD_Pos)

/* SysTick Current Value Register Definitions */
#define SysTick_VAL_CURRENT_Pos      0U
#define SysTick_VAL_CURRENT_Msk      (0xFFFFFFUL << SysTick_VAL_CURRENT_Pos)

/* SysTick Calibration Register Definitions */
#define SysTick_CALIB_NOREF_Pos      31U
#define SysTick_CALIB_NOREF_Msk      (1UL << SysTick_CALIB_NOREF_Pos)

#define SysTick_CALIB_SKEW_Pos       30U
#define SysTick_CALIB_SKEW_Msk       (1UL << SysTick_CALIB_SKEW_Pos)

#define SysTick_CALIB_TENMS_Pos      0U
#define SysTick_CALIB_TENMS_Msk      (0xFFFFFFUL << SysTick_CALIB_TENMS_Pos)

/* SysTick Structure */
typedef struct {
    __IO uint32_t CTRL;   /* Offset: 0x000 Control and Status Register */
    __IO uint32_t LOAD;   /* Offset: 0x004 Reload Value Register */
    __IO uint32_t VAL;    /* Offset: 0x008 Current Value Register */
    __I  uint32_t CALIB;  /* Offset: 0x00C Calibration Register */
} SysTick_Type;

/* SysTick Base Address (Cortex-M Private Peripheral Bus) */
#define SCS_BASE            (0xE000E000UL)
#define SysTick_BASE        (SCS_BASE + 0x0010UL)
#define SysTick             ((SysTick_Type *)SysTick_BASE)

/* ================== SCB (System Control Block) ================== */

/* SCB VTOR Register Definitions */
#define SCB_VTOR_TBLOFF_Pos         7U
#define SCB_VTOR_TBLOFF_Msk         (0x1FFFFFFUL << SCB_VTOR_TBLOFF_Pos)

/* SCB Application Interrupt and Reset Control Register Definitions */
#define SCB_AIRCR_VECTKEY_Pos       16U
#define SCB_AIRCR_VECTKEY_Msk       (0xFFFFUL << SCB_AIRCR_VECTKEY_Pos)
#define SCB_AIRCR_PRIGROUP_Pos      8U
#define SCB_AIRCR_PRIGROUP_Msk      (7UL << SCB_AIRCR_PRIGROUP_Pos)
#define SCB_AIRCR_SYSRESETREQ_Pos   2U
#define SCB_AIRCR_SYSRESETREQ_Msk   (1UL << SCB_AIRCR_SYSRESETREQ_Pos)

/* Alignment attribute */
#ifndef __aligned
#define __aligned(x) __attribute__((aligned(x)))
#endif

/* System Control Block Structure */
typedef struct {
    __I  uint32_t CPUID;    /* Offset: 0x000 CPUID Base Register */
    __IO uint32_t ICSR;     /* Offset: 0x004 Interrupt Control and State Register */
    __IO uint32_t VTOR;     /* Offset: 0x008 Vector Table Offset Register */
    __IO uint32_t AIRCR;    /* Offset: 0x00C Application Interrupt and Reset Control */
    __IO uint32_t SCR;      /* Offset: 0x010 System Control Register */
    __IO uint32_t CCR;      /* Offset: 0x014 Configuration Control Register */
    __IO uint8_t  SHP[12U]; /* Offset: 0x018 System Handlers Priority Registers */
    __IO uint32_t SHCSR;    /* Offset: 0x024 System Handler Control and State */
    __IO uint32_t CFSR;     /* Offset: 0x028 Configurable Fault Status Register */
    __IO uint32_t HFSR;     /* Offset: 0x02C HardFault Status Register */
    __IO uint32_t DFSR;     /* Offset: 0x030 Debug Fault Status Register */
    __IO uint32_t MMFAR;    /* Offset: 0x034 MemManage Fault Address Register */
    __IO uint32_t BFAR;     /* Offset: 0x038 BusFault Address Register */
    __IO uint32_t AFSR;     /* Offset: 0x03C Auxiliary Fault Status Register */
} SCB_Type;

#define SCB_BASE            (SCS_BASE + 0x0D00UL)
#define SCB                 ((SCB_Type *)SCB_BASE)

/* ================== NVIC ================== */

/* NVIC Structure */
typedef struct {
    __IO uint32_t ISER[16U]; /* Offset: 0x000 Interrupt Set Enable Register */
         uint32_t RESERVED0[16U];
    __IO uint32_t ICER[16U]; /* Offset: 0x080 Interrupt Clear Enable Register */
         uint32_t RESERVED1[16U];
    __IO uint32_t ISPR[16U]; /* Offset: 0x100 Interrupt Set Pending Register */
         uint32_t RESERVED2[16U];
    __IO uint32_t ICPR[16U]; /* Offset: 0x180 Interrupt Clear Pending Register */
         uint32_t RESERVED3[16U];
    __IO uint32_t IABR[16U]; /* Offset: 0x200 Interrupt Active bit Register */
         uint32_t RESERVED4[16U];
    __IO uint32_t ITNS[16U]; /* Offset: 0x280 Interrupt Target Non-secure */
         uint32_t RESERVED5[16U];
    __IO uint8_t  IP[496U];  /* Offset: 0x300 Interrupt Priority Register */
         uint32_t RESERVED6[580U];
    __O  uint32_t STIR;      /* Offset: 0xE00 Software Trigger Interrupt Register */
} NVIC_Type;

#define NVIC_BASE           (SCS_BASE + 0x0100UL)
#define NVIC                ((NVIC_Type *)NVIC_BASE)

/* ================== Helper Functions ================== */

static inline void __enable_irq(void)
{
    __asm volatile ("cpsie i" : : : "memory");
}

static inline void __disable_irq(void)
{
    __asm volatile ("cpsid i" : : : "memory");
}

static inline void __DSB(void)
{
    __asm volatile ("dsb 0xF":::"memory");
}

static inline void __ISB(void)
{
    __asm volatile ("isb 0xF":::"memory");
}

static inline void __DMB(void)
{
    __asm volatile ("dmb 0xF":::"memory");
}

static inline void __NOP(void)
{
    __asm volatile ("nop");
}

static inline void __WFI(void)
{
    __asm volatile ("wfi");
}

/* NVIC helper functions */
static inline void NVIC_EnableIRQ(int IRQn)
{
    if (IRQn >= 0) {
        NVIC->ISER[(((uint32_t)IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
    }
}

static inline void NVIC_DisableIRQ(int IRQn)
{
    if (IRQn >= 0) {
        NVIC->ICER[(((uint32_t)IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
    }
}

static inline void NVIC_SetPriority(int IRQn, uint32_t priority)
{
    if (IRQn >= 0) {
        NVIC->IP[((uint32_t)IRQn)] = (uint8_t)((priority << 4U) & 0xFFUL);
    } else {
        SCB->SHP[(((uint32_t)IRQn) & 0xFUL)-4UL] = (uint8_t)((priority << 4U) & 0xFFUL);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* __CORE_CM33_H__ */

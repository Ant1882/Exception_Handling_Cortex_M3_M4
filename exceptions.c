/*
 * exceptions.c
 *
 *  Created on: 8 May 2019
 *      Author: anthony.marshall
 *
 *  Provides initialisation and handling of STM32 M3/M4 core exceptions
 *  - Done here as no useful implementations are provided by ST in "stm32f4xx_it.c"
 *  - These are specific to Cortex M3/M4 cores, M0 cores have a different exception model
 */
#include <stdint.h>

#include "exceptions.h"
#include "kernelPrintf.h"

#if defined(STM32F413xx)
#include "stm32f413xx.h"
#else
#error *** ERROR - Cortex M4 Vectors CPU type not defined.
#endif

// Usage fault status bits.
#define SCB_CFSR_DIVBYZERO      (1u<<25)    // Division by zero trapped.
#define SCB_CFSR_UNALIGNED      (1u<<24)    // Data misalignment detected.
#define SCB_CFSR_UNDEFINSTR     (1u<<16)    // Executed an undefined instruction.

// Bus fault status bits.
#define SCB_CFSR_BFARVALID      (1u<<15)    // BusFault Address Register (BFAR) valid flag.
#define SCB_CFSR_LSPERR         (1u<<13)    // BusFault during floating point lazy state preservation.
#define SCB_CFSR_STKERR         (1u<<12)    // BusFault on stacking for exception entry.
#define SCB_CFSR_UNSTKERR       (1u<<11)    // BusFault on unstacking for a return from exception.
#define SCB_CFSR_IMPRECISERR    (1u<<10)    // Imprecise data bus error.
#define SCB_CFSR_PRECISERR      (1u<<9)     // Precise data bus error.
#define SCB_CFSR_IBUSERR        (1u<<8)     // Instruction bus error.

// Memory manager fault status bits.
#define SCB_CFSR_MMARVALID      (1u<<7)     // Fault Address Register (MMFAR) valid flag.
#define SCB_CFSR_MLSPERR        (1u<<5)     // Fault during floating point lazy state preservation.
#define SCB_CFSR_MSTKERR        (1u<<4)     // Fault on stacking for exception entry.
#define SCB_CFSR_MUNSTKERR      (1u<<3)     // Fault on unstacking for a return from exception.
#define SCB_CFSR_DACCVIOL       (1u<<1)     // Invalid data address.
#define SCB_CFSR_IACCVIOL       (1u<<0)     // Invalid execution address.

/* Alignment trapping can be more problematic so option to avoid */
#define TRAP_DIVIDE_BY_ZERO_ONLY

static void printExtraInfo(const CortexExceptionCpuFrameType* aFrame, exceptionType eType);

void hardFault(const CortexExceptionCpuFrameType* aFrame);
void memMangFault(const CortexExceptionCpuFrameType* aFrame);
void busFault(const CortexExceptionCpuFrameType* aFrame);
void usageFault(const CortexExceptionCpuFrameType* aFrame);

/* Initialise the exception handlers
 * - If not initialised they will be escalated to a hard fault
 */
void exceptionsInit()
{
#if !defined(TRAP_DIVIDE_BY_ZERO_ONLY)
    // Enable division by zero and alignment trapping.
    SCB->CCR|=SCB_CCR_UNALIGN_TRP_Msk|SCB_CCR_DIV_0_TRP_Msk;
#else
    // Enable division by zero trapping only.
    SCB->CCR|=SCB_CCR_DIV_0_TRP_Msk;
#endif // !TRAP_DIVIDE_BY_ZERO_ONLY

    // Enable other faults of interest,
    // To test HardFault_Handler comment out the line below & call generateHardFault();
    SCB->SHCSR|=SCB_SHCSR_USGFAULTENA_Msk|SCB_SHCSR_BUSFAULTENA_Msk|SCB_SHCSR_MEMFAULTENA_Msk;
}

/* Fault generation functions
 * - Use these to test exception handling
*/

/* Generate a hard fault
 * - Jump to a function that doesn't exist
 * - Actually generates Usage Fault if we have that enabled
*/
void generateHardFault()
{
    void (*fp)(void) = (void (*)(void))(0x00000000);
    fp();
}

/* Generate a hard fault
 * - Try to execute code from an XN region
*/
void generateMemMangFault()
{
    void (*fp)(void) = (void (*)(void))(0xFFFFFFFF);
    fp();
}

/* Generate a precise bus fault
 * - Write to invalid an memory address
*/
void generateBusFault()
{
    volatile unsigned int* p;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    unsigned int n;
#pragma GCC diagnostic pop
    p = (unsigned int*)0xCCCCCCCC;
    n = *p;
}

/* Generate a usage fault
 * - divide by zero exception
*/
int generateUsageFault()
{
    int a = 1;
    int b = 0;
    return (a / b);
}

static void printExtraInfo(const CortexExceptionCpuFrameType* aFrame, exceptionType eType)
{
    uint32_t cfsr  = SCB->CFSR;

    uint32_t hfsr = EXCEPTION_HANDLER_FIELD_IS_INVALID;
    uint32_t faultAdd = EXCEPTION_HANDLER_FIELD_IS_INVALID;

    KernelPrintf("**** EXCEPTION OCCURRED ****\r\n");

    // Print out the exception type and reason
    switch (eType)
    {
        case Usage_Fault:
        {
            KernelPrintf("Type: Usage Fault\r\n");

            if(cfsr & SCB_CFSR_DIVBYZERO)
                KernelPrintf("Reason: Division by zero\r\n\n");
            else if(cfsr & SCB_CFSR_UNALIGNED)
                KernelPrintf("Reason: Misaligned data access\r\n\n");
            else if(cfsr & SCB_CFSR_UNDEFINSTR)
                KernelPrintf("Reason: Undefined instruction\r\n\n");
            else
                KernelPrintf("Reason: Unknown\r\n\n");
        }
        break;
        case Bus_Fault:
        {
            KernelPrintf("Type: Bus Fault\r\n");

            if(cfsr & SCB_CFSR_IBUSERR)
                KernelPrintf("Reason: Invalid code address\r\n\n");
            else if(cfsr & (SCB_CFSR_PRECISERR|SCB_CFSR_IMPRECISERR))
                KernelPrintf("Reason: Invalid data address\r\n\n");
            else if(cfsr & (SCB_CFSR_STKERR|SCB_CFSR_UNSTKERR))
                KernelPrintf("Reason: Exception stack fault\r\n\n");
            else if(cfsr & SCB_CFSR_LSPERR)
                KernelPrintf("Reason: Floating point fault\r\n\n");
            else
                KernelPrintf("Reason: Unknown\r\n\n");
            // Check bus fault address valid flag
            if(cfsr & SCB_CFSR_BFARVALID)
                faultAdd  = SCB->BFAR;
        }
        break;
        case Hard_Fault:
        {
            KernelPrintf("Type: Hard Fault\r\n");
            KernelPrintf("Reason: Unknown\r\n\n");
            hfsr  = SCB->HFSR;
        }
        break;
        case MemMang_Fault:
        {
            KernelPrintf("Type: Memory Fault\r\n");

            if(cfsr & SCB_CFSR_IACCVIOL)
                KernelPrintf("Reason: Invalid code address\r\n\n");
            else if(cfsr & SCB_CFSR_DACCVIOL)
                KernelPrintf("Reason: Invalid data address\r\n\n");
            else if(cfsr & (SCB_CFSR_MSTKERR|SCB_CFSR_MUNSTKERR))
                KernelPrintf("Reason: Exception stack fault\r\n\n");
            else if(cfsr & SCB_CFSR_LSPERR)
                KernelPrintf("Reason: Floating point fault\r\n\n");
            else
                KernelPrintf("Reason: Unknown\r\n\n");
            // Check mem manager fault address valid flag
            if(cfsr & SCB_CFSR_MMARVALID)
                faultAdd = SCB->MMFAR;
        }
        break;
    }

    // Print registers
    KernelPrintf("R0=%x R1=%x\r\n", aFrame->m_R0, aFrame->m_R1);
    KernelPrintf("R2=%x R3=%x\r\n", aFrame->m_R2, aFrame->m_R3);
    KernelPrintf("R12=%x LR=%x\r\n", aFrame->m_R12, aFrame->m_LR);
    KernelPrintf("PC=%x PSR=%x\r\n", aFrame->m_PC, aFrame->m_PSR);

    // Print fault info
    KernelPrintf("HFSR=%x CFSR=%x\r\n", hfsr, cfsr);
    KernelPrintf("Fault address=%x\r\n", faultAdd);
}

/* fault handlers
 * - Provide some information on where the fault occurred
*/

void hardFault(const CortexExceptionCpuFrameType* aFrame)
{
#ifdef __DEBUG_KERNEL__
    printExtraInfo(aFrame, Hard_Fault);
#endif
    __asm__("BKPT");
}

void memMangFault(const CortexExceptionCpuFrameType* aFrame)
{
#ifdef __DEBUG_KERNEL__
    printExtraInfo(aFrame, MemMang_Fault);
#endif
    __asm__("BKPT");
}

void busFault(const CortexExceptionCpuFrameType* aFrame)
{
#ifdef __DEBUG_KERNEL__
    printExtraInfo(aFrame, Bus_Fault);
#endif
    __asm__("BKPT");
}

void usageFault(const CortexExceptionCpuFrameType* aFrame)
{
#ifdef __DEBUG_KERNEL__
    printExtraInfo(aFrame, Usage_Fault);
#endif
    __asm__("BKPT");
}


/* Low level fault handlers
 * - First port of call when an exception occurs
 * - These handle exceptions in a controlled manner and call a function of our choice
*/
__attribute__((naked))  void HardFault_Handler(void)
{
    asm volatile("tst lr, #4");                     // Check the exception return behaviour (EXC_RETURN)
    asm volatile("ite eq");
    asm volatile("mrseq r0, msp");                  // Bit 2 is low - Return behaviour is F9/E9 or F1/E1 so MSP stack.
    asm volatile("mrsne r0, psp");                  // Bit 2 is high - Return behaviour is FD/ED, so PSP stack.

    asm volatile("mov r1, #0");
    asm volatile("msr PRIMASK, r1");                // Disable all interrupts...
    asm volatile("msr FAULTMASK, r1");              // ... and subsequent faults.

    asm volatile("ldr r1, =hardFault");
    asm volatile("bx r1");                          // Call the real handler.
}

__attribute__((naked))  void MemManage_Handler(void)
{
    asm volatile("tst lr, #4");                     // Check the exception return behaviour (EXC_RETURN)
    asm volatile("ite eq");
    asm volatile("mrseq r0, msp");                  // Bit 2 is low - Return behaviour is F9/E9 or F1/E1 so MSP stack.
    asm volatile("mrsne r0, psp");                  // Bit 2 is high - Return behaviour is FD/ED, so PSP stack.

    asm volatile("mov r1, #0");
    asm volatile("msr PRIMASK, r1");                // Disable all interrupts...
    asm volatile("msr FAULTMASK, r1");              // ... and subsequent faults.

    asm volatile("ldr r1, =memMangFault");
    asm volatile("bx r1");                          // Call the real handler.
}

__attribute__((naked))  void BusFault_Handler(void)
{
    asm volatile("tst lr, #4");                     // Check the exception return behaviour (EXC_RETURN)
    asm volatile("ite eq");
    asm volatile("mrseq r0, msp");                  // Bit 2 is low - Return behaviour is F9/E9 or F1/E1 so MSP stack.
    asm volatile("mrsne r0, psp");                  // Bit 2 is high - Return behaviour is FD/ED, so PSP stack.

    asm volatile("mov r1, #0");
    asm volatile("msr PRIMASK, r1");                // Disable all interrupts...
    asm volatile("msr FAULTMASK, r1");              // ... and subsequent faults.

    asm volatile("ldr r1, =busFault");
    asm volatile("bx r1");                          // Call the real handler.
}

__attribute__((naked))  void UsageFault_Handler(void)
{
    asm volatile("tst lr, #4");                     // Check the exception return behaviour (EXC_RETURN)
    asm volatile("ite eq");
    asm volatile("mrseq r0, msp");                  // Bit 2 is low - Return behaviour is F9/E9 or F1/E1 so MSP stack.
    asm volatile("mrsne r0, psp");                  // Bit 2 is high - Return behaviour is FD/ED, so PSP stack.

    asm volatile("mov r1, #0");
    asm volatile("msr PRIMASK, r1");                // Disable all interrupts...
    asm volatile("msr FAULTMASK, r1");              // ... and subsequent faults.

    asm volatile("ldr r1, =usageFault");
    asm volatile("bx r1");                          // Call the real handler.
}

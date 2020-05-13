/*
 * exceptions.h
 *
 *  Created on: 8 May 2019
 *      Author: anthony.marshall
 */

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#define EXCEPTION_HANDLER_FIELD_IS_INVALID  0xDEADD0D0

// ARM Cortex CPU exception stack frame.
typedef struct
{
    uint32_t m_R0;      // Register r0.
    uint32_t m_R1;      // Register r1.
    uint32_t m_R2;      // Register r2.
    uint32_t m_R3;      // Register r3.
    uint32_t m_R12;     // Register r12 - Frame.
    uint32_t m_LR;      // Register r14 - Link (return address).
    uint32_t m_PC;      // Register r15 - Program counter.
    uint32_t m_PSR;     // Status register.
}  CortexExceptionCpuFrameType;

typedef enum
{
    Hard_Fault,
    MemMang_Fault,
    Bus_Fault,
    Usage_Fault
} exceptionType;

void exceptionsInit();

int generateUsageFault();
void generateBusFault();
void generateHardFault();
void generateMemMangFault();

#endif /* EXCEPTIONS_H_ */

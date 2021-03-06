/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Exception management support for IA-32 arch
 *
 * This module provides routines to manage exceptions (synchronous interrupts)
 * on the IA-32 architecture.
 *
 * This module provides the public routine nanoCpuExcConnect().
 *
 * INTERNAL
 * An exception is defined as a synchronous interrupt, i.e. an interrupt
 * asserted as a direct result of program execution as opposed to a
 * hardware device asserting an interrupt.
 *
 * Many (but not all) exceptions are handled by an "exception stub" whose code
 * is generated by the system itself.  The stub performs various actions before
 * and after invoking the application (or operating system) specific exception
 * handler; for example, a thread or ISR context save is performed prior to
 * invoking the exception handler.
 *
 * The IA-32 code that makes up a "full" exception stub is shown below.  A full
 * exception stub is one that pushes a dummy error code at the start of
 * exception processing.  Exception types where the processor automatically
 * pushes an error code when handling an exception utilize similar exception
 * stubs, however the first instruction is omitted.  The use of the dummy error
 * code means that _ExcEnt() and _ExcExit() do not have to worry about whether
 * an error code is present on the stack or not.
 *
 *
 *   0x00   pushl   $0			/@ push dummy error code @/
 *   Machine code:  0x68, 0x00, 0x00, 0x00, 0x00
 *
 *   0x05   call    _ExcEnt		/@ inform kernel of exception @/
 *   Machine code:  0xe8, 0x00, 0x00, 0x00, 0x00
 *
 *   0x0a   call    ExcHandler		/@ invoke exception handler @/
 *   Machine code: 0xe8, 0x00, 0x00, 0x00, 0x00
 *
 *   /@ _ExcExit() will adjust the stack to discard the error code @/
 *
 *   0x0f  jmp	   _ExcExit		/@ restore thread context @/
 *   Machine code: 0xe9, 0x00, 0x00, 0x00, 0x00
 *
 * NOTE: Be sure to update the arch specific definition of the _EXC_STUB_SIZE
 * macro to reflect the size of the full exception stub (as shown above).
 * The _EXC_STUB_SIZE macro is defined in arch/x86/include/nano_private.h.
 */


#include <nanokernel.h>
#include <nano_private.h>
#include <misc/__assert.h>

#if ALL_DYN_EXC_STUBS > 0

static void (*exc_handlers[ALL_DYN_EXC_STUBS])(NANO_ESF *pEsf);

static unsigned int next_exc_stub;
static unsigned int next_exc_noerr_stub;

extern void *_DynExcStubsBegin;
extern void *_DynExcStubsNoErrBegin;

void _NanoCpuExcConnectAtDpl(unsigned int vector,
			     void (*routine)(NANO_ESF * pEsf),
			     unsigned int dpl);

/**
 *
 * @brief Connect a C routine to an exception
 *
 * This routine connects an exception handler coded in C to the specified
 * interrupt vector.  An exception is defined as a synchronous interrupt, i.e.
 * an interrupt asserted as a direct result of program execution as opposed
 * to a hardware device asserting an interrupt.
 *
 * When the exception specified by <vector> is asserted, the current thread
 * is saved on the current stack, i.e. a switch to some other stack is not
 * performed, followed by executing <routine> which has the following signature:
 *
 *    void (*routine) (NANO_ESF *pEsf)
 *
 * The <pExcStubMem> argument points to memory that the system can use to
 * synthesize the exception stub that calls <routine>.  The memory need not be
 * initialized, but must be persistent (i.e. it cannot be on the caller's stack).
 * Declaring a global or static variable of type NANO_EXC_STUB will provide a
 * suitable area of the proper size.
 *
 * The handler is connected via an interrupt-gate descriptor having a
 * descriptor privilege level (DPL) equal to zero.
 *
 * @return N/A
 *
 * INTERNAL
 * The function prototype for nanoCpuExcConnect() only exists in nano_private.h,
 * in other words, it's still considered private since the definitions for
 * the NANO_ESF structures have not been completed.
 */

void nanoCpuExcConnect(unsigned int vector, /* interrupt vector: 0 to 255 on
					     * IA-32
					     */
		       void (*routine)(NANO_ESF * pEsf))
{
	_NanoCpuExcConnectAtDpl(vector, routine, 0);
}

/**
 *
 * @brief Connect a C routine to an exception
 *
 * This routine connects an exception handler coded in C to the specified
 * interrupt vector.  An exception is defined as a synchronous interrupt, i.e.
 * an interrupt asserted as a direct result of program execution as opposed
 * to a hardware device asserting an interrupt.
 *
 * When the exception specified by <vector> is asserted, the current thread
 * is saved on the current stack, i.e. a switch to some other stack is not
 * performed, followed by executing <routine> which has the following signature:
 *
 *    void (*routine) (NANO_ESF *pEsf)
 *
 * The <pExcStubMem> argument points to memory that the system can use to
 * synthesize the exception stub that calls <routine>.  The memory need not be
 * initialized, but must be persistent (i.e. it cannot be on the caller's stack).
 * Declaring a global or static variable of type NANO_EXC_STUB will provide a
 * suitable area of the proper size.
 *
 * The handler is connected via an interrupt-gate descriptor having the supplied
 * descriptor privilege level (DPL).
 *
 * WARNING memory will leak if the vector was already connected to a different
 * dynamic handler
 *
 * @return N/A
 *
 * INTERNAL
 * The function prototype for nanoCpuExcConnect() only exists in nano_private.h,
 * in other words, it's still considered private since the definitions for
 * the NANO_ESF structures have not been completed.
 */

void _NanoCpuExcConnectAtDpl(
	unsigned int vector, /* interrupt vector: 0 to 255 on IA-32 */
	void (*routine)(NANO_ESF * pEsf),
	unsigned int dpl /* priv level for interrupt-gate descriptor */
	)
{
	int stub_idx, limit, offset;
	unsigned int *next_p;
	void *base_ptr;

	/*
	 * Check to see if this exception type takes an error code, we
	 * have different stubs for that
	 */
	if (((1 << vector) & _EXC_ERROR_CODE_FAULTS) == 0) {
		base_ptr = &_DynExcStubsNoErrBegin;
		next_p = &next_exc_noerr_stub;
		limit = CONFIG_NUM_DYNAMIC_EXC_NOERR_STUBS;
		offset = CONFIG_NUM_DYNAMIC_EXC_STUBS;
	} else {
		base_ptr = &_DynExcStubsBegin;
		next_p = &next_exc_stub;
		limit = CONFIG_NUM_DYNAMIC_EXC_STUBS;
		offset = 0;
	}

	stub_idx = _stub_alloc(next_p, limit);
	__ASSERT(stub_idx != -1, "No available execption stubs");
	/*
	 * We have the same array for both error code and non error code
	 * exceptions, the second half is reserved for the non error code
	 * handlers
	 */
	exc_handlers[stub_idx + offset] = routine;
	_IntVecSet(vector, _get_dynamic_stub(stub_idx, base_ptr), dpl);
}

void _common_dynamic_exc_handler(uint32_t stub_idx, NANO_ESF *pEsf)
{
	exc_handlers[stub_idx](pEsf);
}

#endif /* ALL_DYN_EXC_STUBS */


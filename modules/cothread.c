/*
* Copyright (c) 2012, Alexander I. Mykyta
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*==================================================================================================
* File History:
* NAME          DATE         COMMENTS
* Alex M.       2012-12-12   born
* Alex M.       2013-05-11   Fixed: Startup function pointer gets stored in stack with mspgcc
*
*=================================================================================================*/

/**
* \addtogroup MOD_COTHREADS
* \{
**/

/**
* \file
* \brief Code for \ref MOD_COTHREADS "Cooperative Threads"
* \author Alex Mykyta
**/

#include <stdint.h>
#include <setjmp.h>
#include "cothread.h"

/*
 * -----------------------------------------
 *  For the home thread:
 * -----------------------------------------
 *
 * typedef struct cothread{
 *         struct cothread    *co_exit; == Null. Unique property of a home thread since it has nowhere to exit to.
 *         void *alt_stack; == Null. Stack is self-defined in home thread
 *         size_t alt_stack_size; Any nonzero value
 *         m_state_t m_state; the 'valid' element is nonzero
 * } cothread_t;
 *
 * -----------------------------------------
 *  For an alternate thread:
 * -----------------------------------------
 *
 * typedef struct cothread{
 *         struct cothread    *co_exit; ///< Thread to switch to once the current thread exits
 *         void *alt_stack; ///< Pointer to the base of an alternate stack.
 *         size_t alt_stack_size; ///< The size (in bytes) of the stack which 'alt_stack' points to.
 *         m_state_t m_state; the 'valid' element is nonzero
 * } cothread_t;
 *
 * ------------------------------------------
 *  For a dead thread: (one that has exited)
 * ------------------------------------------
 *
 * typedef struct cothread{
 *         struct cothread    *co_exit; ///< Thread to switch to once the current thread exits
 *         void *alt_stack; ///< Pointer to the base of an alternate stack.
 *         size_t alt_stack_size; ///< The size (in bytes) of the stack which 'alt_stack' points to.
 *         m_state_t m_state; the 'valid' element is zero
 * } cothread_t;
 *
 */

//--------------------------------------------------------------------------------------------------

static cothread_t *CurrentThread;
static int ThreadRetval;
//--------------------------------------------------------------------------------------------------
void cothread_init(cothread_t *home_thread)
{
    home_thread->co_exit = NULL;
    home_thread->alt_stack = NULL;
    home_thread->alt_stack_size = 1;
    home_thread->m_state.valid = 1;

    CurrentThread = home_thread;
}

//--------------------------------------------------------------------------------------------------
void cothread_create(cothread_t *thread, int (*func) (void))
{

    // Cant guarantee func will be preserved. Throw it into a variable that wont be referenced via
    // the stack. (thread becomes CurrentThread after the setjmp gets longjumped to)
    thread->func_start = func;

    if (setjmp(thread->m_state.env))
    {
        // new context startup routine

        // kick-off the new thread
        ThreadRetval = CurrentThread->func_start();

        // the thread is no longer valid
        CurrentThread->m_state.valid = 0;

        // if co_exit is valid, switch to it
        if (CurrentThread->co_exit)
        {

            CurrentThread = CurrentThread->co_exit;
            longjmp(CurrentThread->m_state.env, 1);
        }

        //Otherwise, I have no idea where to go! I guess its time for an infinite loop
        while (1);
        // ...Does not return
    }

    // got the startup context from setjump. modify it so it uses the alternate stack
    // Setup saved stack pointer in env to point to the base of the new stack
    uintptr_t sp_tmp;
    sp_tmp = (uintptr_t) thread->alt_stack;
    sp_tmp += thread->alt_stack_size;
#if defined(__GNUC__) && defined(__MSP430__)
    thread->m_state.env[0].__j_pc = sp_tmp; // is actually __j_sp (setjump.h has it mislabled)
#elif defined(__TI_COMPILER_VERSION__)
    thread->m_state.env[7] = sp_tmp;
#else
#error Compiler not supported
#endif

    // This thread is now officially valid
    thread->m_state.valid = 1;

}

//--------------------------------------------------------------------------------------------------
int cothread_switch(cothread_t *dest_thread)
{

    if (dest_thread == CurrentThread) return(-1); // already in the dest_thread. nothing to do

    if (!(dest_thread->m_state.valid)) return(-1); // dest_thread is not valid. Don't switch

    // save the current state
    if (!setjmp(CurrentThread->m_state.env))
    {
        // switch to the other thread
        CurrentThread = dest_thread;
        ThreadRetval = 0;
        longjmp(dest_thread->m_state.env, 1);
    }
    // Other thread will longjump here later

    return(ThreadRetval);
}

//--------------------------------------------------------------------------------------------------
void cothread_exit(int retval)
{
    // exit only if it has a valid exit destination
    if (CurrentThread->co_exit)
    {
        // Mark this thread as invalid
        CurrentThread->m_state.valid = 0;

        // Switch to the exit thread with retval
        CurrentThread = CurrentThread->co_exit;
        ThreadRetval = retval;
        longjmp(CurrentThread->m_state.env, 1);
    }
}

//--------------------------------------------------------------------------------------------------
#define LFSR_INIT    0x0001
static uint16_t lfsr16(uint16_t lfsr)
{
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
    return(lfsr);
}

//--------------------------------------------------------------------------------------------------
void stackmon_init(stack_t *stack, size_t stack_size)
{
    uint16_t *stack_w;
    size_t i;
    uint16_t lfsr = LFSR_INIT;

    stack_w = (void *)stack;
    stack_size = stack_size / sizeof(uint16_t);

    for (i = 0; i < stack_size; i++)
    {
        stack_w[i] = lfsr;
        lfsr = lfsr16(lfsr);
    }
}

//--------------------------------------------------------------------------------------------------
size_t stackmon_get_unused(stack_t *stack)
{
    uint16_t *stack_w;
    size_t i;
    uint16_t lfsr = LFSR_INIT;

    stack_w = (void *)stack;

    i = 0;
    while (lfsr == stack_w[i])
    {
        lfsr = lfsr16(lfsr);
        i++;
    }

    return(i * 2);
}

//--------------------------------------------------------------------------------------------------
///\}
///\}

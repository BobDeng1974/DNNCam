/*
 * Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Thread.h"
#include "log.hpp"
#include "defines.hpp"

namespace NvidiaUtils
{

Thread::Thread()
    : m_doShutdown(false)
    , m_threadID(0)
    , m_threadState(THREAD_INACTIVE)

{
}

Thread::~Thread()
{
    (void)shutdown();
}

bool Thread::initialize()
{
    if (m_threadID)
        return true;

    if (pthread_create(&m_threadID, NULL, threadFunctionStub, this) != 0)
    {
        bl_log_error("Failed to create thread");
        return false;
    }

    // wait for the thread to start up
    while (m_threadState == THREAD_INACTIVE)
        usleep(100);

    return true;

}

bool Thread::shutdown()
{
    if (m_threadID)
    {
        m_doShutdown = true;
        if (pthread_join(m_threadID, NULL) != 0)
        {
            bl_log_error("Failed to join thread");
            return false;
        }
        m_threadID = 0;
        m_doShutdown = false;
        m_threadState = THREAD_INACTIVE;
    }

   return true;
}

bool Thread::waitRunning(useconds_t timeoutUs)
{
    // Can only wait for a thread which is initializing or already running
    if ((m_threadState != THREAD_INITIALIZING) && (m_threadState != THREAD_RUNNING))
    {
        bl_log_error("Invalid thread state " << m_threadState.get());
        return false;
    }

    // wait for the thread to run
    const useconds_t sleepTimeUs = 100;
    while (m_threadState != THREAD_RUNNING)
    {
        usleep(sleepTimeUs);
#ifdef DEBUG
        // in debug mode wait indefinitely
#else
        if (timeoutUs < sleepTimeUs)
            return false;
        timeoutUs -= sleepTimeUs;
#endif
    }

    return true;
}

/**
 * Thread function stub, calls the real thread function.
 *
 * @param [in] dataPtr  Pointer to user data
 */
/* static */ void *Thread::threadFunctionStub(void *dataPtr)
{
    Thread *thread = static_cast<Thread*>(dataPtr);

    if (!thread->threadFunction())
        thread->m_threadState = Thread::THREAD_FAILED;
    else
        thread->m_threadState = Thread::THREAD_DONE;

    return NULL;
}

/**
 * Thread function
 */
bool Thread::threadFunction()
{
    m_threadState = THREAD_INITIALIZING;

    if (false == threadInitialize())
    {
        bl_log_error("threadInitialize failed");
        return false;
    }

    m_threadState = THREAD_RUNNING;

    while (!m_doShutdown)
    {
#ifdef USE_ARGUS_CONVERT
        if (false == threadArgusExecute())
#else
        if (false == threadExecute())
#endif
        {
            bl_log_error("threadExecute failed");
            return false;
        }
    }

    if (false == threadShutdown())
    {
        bl_log_error("threadShutdown failed");
        return false;
    }

    return true;
}

}; // namespace NvidiaUtils

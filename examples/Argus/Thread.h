

#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <unistd.h>

namespace ArgusSamples
{

    class Thread
    {
    public:
        Thread();
        virtual ~Thread();
        bool initialize();
        bool shutdown();
        bool waitRunning(useconds_t timeoutUs = 5 * 1000 * 1000);

    protected:
        virtual bool threadInitialize() = 0;
        virtual bool threadExecute() = 0;
        virtual bool threadShutdown() = 0;
        bool requestShutdown()
        {
            m_doShutdown = true;
            return true;
        }

        Ordered<bool> m_doShutdown;

    private:
        pthread_t m_threadID;

        enum ThreadState
        {
            THREAD_INACTIVE,
            THREAD_INITIALIZING,
            THREAD_RUNNING,
            THREAD_FAILED,
            THREAD_DONE,
        };
        Ordered<ThreadState> m_threadState;

        bool threadFunction();

        static void *threadFunctionStub(void *dataPtr);
    };

}

#endif

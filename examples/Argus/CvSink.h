

#ifndef LIBARGUSSTREAMER_CVSINK_H
#define LIBARGUSSTREAMER_CVSINK_H

#include <Argus/Argus.h>
#include <EGLStream/EGLStream.h>
#include "Thread.h"
#include "EGLGlobal.h"
#include "GLContext.h"
#include <NvEglRenderer.h>
#include "config.h"
#include  "concurrentqueue.h"




namespace ArgusSamples
{

    using namespace Argus;
    using namespace EGLStream;


    class CvSinkThread : public Thread
    {
    public:

        explicit CvSinkThread()
        {
        }

        CvSinkThread(OutputStream *stream,  NvEglRenderer *renderer, int id, Queue<frameBuffer> *q,Queue<int> * fdq, Queue<int> *msgq)
                : m_stream(stream),  m_renderer(renderer), m_id(id) , m_pinputFrameQ(q), m_pinputFrameFdQ(fdq),m_pcamCapture2NewOCVConsumerMsgQ(msgq)
        {
        }

        ~CvSinkThread()
        {
        }
        void init_members(OutputStream *stream, ARGUSSIZE size, NvEglRenderer *renderer);

    private:


        virtual bool threadInitialize();
        virtual bool threadExecute();
        virtual bool threadShutdown();
        OutputStream* m_stream;
        UniqueObj<FrameConsumer> m_consumer;
        ARGUSSIZE m_framesize;
        NvEglRenderer *m_renderer;
        int m_id;


        Queue<frameBuffer> *m_pinputFrameQ;
        Queue<int   > *m_pinputFrameFdQ;
        Queue<int   > *m_pcamCapture2NewOCVConsumerMsgQ;

        aaDebug       *m_pDebugObj;

    };



}
#endif //LIBARGUSSTREAMER_CVSINK_H

#ifndef CAPTURE_H
#define CAPTURE_H

#include <Argus/Argus.h>
#include <EGLStream/EGLStream.h>
#include "Thread.h"
#include "EGLGlobal.h"
#include "GLContext.h"
#include <NvEglRenderer.h>
#include "concurrentqueue.h"
#include "config.h"

namespace ArgusSamples
{

using namespace Argus;
using namespace EGLStream;


class CaptureThread : public Thread
{
public:

    explicit CaptureThread()
    {
    }

    explicit CaptureThread(OutputStream *stream,  NvEglRenderer *renderer, int id , Queue<frameBuffer> *q, Queue<int> * fdq, Queue<int> *msgq, int lastFrame)
        : m_stream(stream),  m_renderer(renderer), m_id(id) , m_pinputFrameQ(q), m_pinputFrameFdQ(fdq), m_pcamCapture2NewOCVConsumerMsgQ(msgq), m_lastFrameCount(lastFrame)
    {
	m_currentFrame = 0;
    }



    ~CaptureThread()
    {
    }
    void init_members(OutputStream *stream, ARGUSSIZE size, NvEglRenderer *renderer);
    int getCamCapture2NewOCVConsumerMsgQSize();



private:



    virtual bool threadInitialize();
    virtual bool threadExecute();
    virtual bool threadShutdown();


    OutputStream               *m_stream;
    UniqueObj<FrameConsumer>    m_consumer;
    ARGUSSIZE                   m_framesize;
    NvEglRenderer              *m_renderer;
    int m_id;

    Queue<frameBuffer>         *m_pinputFrameQ;
    Queue<int   >              *m_pinputFrameFdQ;
    Queue<int   >              *m_pcamCapture2NewOCVConsumerMsgQ;
    int                         m_lastFrameCount;
    int                         m_currentFrame;

};

}

#endif

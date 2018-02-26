#include "CvSink.h"
#include "Error.h"

#include <Argus/Argus.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <iomanip>

#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc_c.h>

#include <sys/time.h>


#include <nvbuf_utils.h>
#include <NvUtils.h>

#include "NvBuffer.h"
#include "NvLogging.h"

#include <cstring>
#include <errno.h>
#include <sys/mman.h>
#include <libv4l2.h>


#include <EGLStream/EGLStream.h>
#include <EGLStream/ArgusCaptureMetadata.h>
#include <EGLStream/NV/ImageNativeBuffer.h>


namespace ArgusSamples
{

#if 0
#define AANEWOCVCONSUMER_PRINT(...);    printf("NEW OCV CONSUMER: " __VA_ARGS__);
#else
#define AANEWOCVCONSUMER_PRINT(...);
#endif


#ifdef ANDROID
#define JPEG_PREFIX "/sdcard/DCIM/Argus_"
#else
#define JPEG_PREFIX "Argus_"
#endif


#define TEST_ERROR_RETURN(cond, _str, ...) \
    do { \
        if (cond) { \
            fprintf(stderr, "[ERROR] %s (%s:%d) : ", __FUNCTION__, __FILE__, __LINE__); \
            fprintf(stderr, _str "\n", ##__VA_ARGS__); \
            return false; \
}} while(0)



    void CvSinkThread::init_members(OutputStream *stream, ARGUSSIZE size, NvEglRenderer *renderer)
    {
        m_renderer  = renderer;
    }
    bool CvSinkThread::threadInitialize()
    {
        AANEWOCVCONSUMER_PRINT("Done initializing CvSinkThread\n");
        m_pDebugObj   = new aaDebug;
        return true;
    }

    bool CvSinkThread::threadExecute()
    {
        int frameCount = 0;
        char *data;

        AANEWOCVCONSUMER_PRINT("Starting aaNewOCVConsumer Thread Execution\n");

        while (true)
        {
            AANEWOCVCONSUMER_PRINT("Pop request initiated %d\n",m_pcamCapture2NewOCVConsumerMsgQ->isempty());
            if (!m_pcamCapture2NewOCVConsumerMsgQ->isempty())
                break;

            frameBuffer framedata = m_pinputFrameQ->pop();
            int framefd = m_pinputFrameFdQ->pop();

            AANEWOCVCONSUMER_PRINT("Pop request granted! Queue Size: %d Camera ID : %d\n", m_pinputFrameQ->getsize() ,m_pCamInfo->camId );

            if (framedata.dataY != NULL) {
                cv::Mat imgY = cv::Mat(FRAME_SIZE_Y ,FRAME_SIZE_X, CV_8UC1, framedata.dataY,PITCH_SIZE_X);
                cv::Mat imgU = cv::Mat(FRAME_SIZE_Y/2 ,FRAME_SIZE_X/2, CV_8UC1, framedata.dataU,(PITCH_SIZE_X/2));
                cv::Mat imgV = cv::Mat(FRAME_SIZE_Y/2 ,FRAME_SIZE_X/2, CV_8UC1, framedata.dataV,(PITCH_SIZE_X/2));

                m_pDebugObj->ocvConsumer2EncoderFrameBuffer.push(framedata);

                m_renderer->render(framefd);

            }
            AANEWOCVCONSUMER_PRINT("1 Popped frame no %d from Q\n",m_pcamCapture2NewOCVConsumerMsgQ->isempty());
            if (!m_pcamCapture2NewOCVConsumerMsgQ->isempty())
                break;

            AANEWOCVCONSUMER_PRINT("2 Popped frame no %d from Q\n",frameCount++);
        }

        AANEWOCVCONSUMER_PRINT("No more frames. Cleaning up. Camera id : %d\n", m_id);
        PROPAGATE_ERROR(requestShutdown());

        return true;
    }

    bool CvSinkThread::threadShutdown()
    {
        AANEWOCVCONSUMER_PRINT("aaNewOCVConsumer Done.\n");
        delete m_pDebugObj;
        return true;
    }



}
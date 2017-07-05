#pragma once

#include "camera.hpp"
#include "Thread.h"
#include <queue>
#include <Argus/Argus.h>
#include <EGLStream/EGLStream.h>
#include <EGLStream/NV/ImageNativeBuffer.h>
#include <NvVideoConverter.h>

class Imx377Camera : public Camera {
public:
    Imx377Camera(std::shared_ptr<camera_context> ctx);
    ~Imx377Camera();

    bool usesCallback() override { return true; }

    void stop() override;
    void set_exposure(const float exposure_time_ms);
    void set_gain(const int gain);
    void set_gain_red(const int gain);
    void set_gain_green(const int gain);
    void set_gain_blue(const int gain);
    void set_gain_boost(bool gain_boost);
    void set_frame_rate(double fps);
    void set_pixel_clock(unsigned int speed_mhz);

    void set_shutter_mode(const std::string &shutter_mode);
    void set_mirror(bool mirror);

    void set_flash_mode(const std::string &mode);
    std::string get_flash_mode();
    void set_strobe_auto(bool);
    void set_strobe_parameters(int, unsigned int);

    void set_log_mode(bool log_mode);
    int get_log_mode_gain();
    void set_log_mode_gain(const int gain);
    int get_log_mode_value();
    void set_log_mode_value(const int value);

    FramePtr grab();
    void start_capture();
private:
    bool _set_frame_rate(double fps) override;
    bool _init() override;

    /*******************************************************************************
     * FrameConsumer thread:
    *   Creates an EGLStream::FrameConsumer object to read frames from the stream
    *   and create NvBuffers (dmabufs) from acquired frames before providing the
    *   buffers to V4L2 for video conversion. The converter will feed the image to
    *   processing routine.
    ******************************************************************************/
    class ConsumerThread : public NvidiaUtils::Thread
    {
    public:
        explicit ConsumerThread(Argus::OutputStream* stream, std::shared_ptr<camera_context> ctx);
        ~ConsumerThread();

        bool isInError()
        {   
            return m_gotError;
        }
        void abort();

    private:
        /** @name Thread methods */
        /**@{*/ 
        virtual bool threadInitialize();
        virtual bool threadExecute();
        virtual bool threadShutdown();
        /**@}*/
    
        bool createImageConverter();
    
        static bool converterCapturePlaneDqCallback(
                struct v4l2_buffer *v4l2_buf,
                NvBuffer *buffer,
                NvBuffer *shared_buffer,
                void *arg);
        static bool converterOutputPlaneDqCallback(
                struct v4l2_buffer *v4l2_buf,
                NvBuffer *buffer,
                NvBuffer *shared_buffer,
                void *arg);
    
        void writeFrameToOpencvConsumer(
            std::shared_ptr<camera_context> p_ctx,
            NvBuffer *buffer);
    
        Argus::OutputStream* m_stream;
        Argus::UniqueObj<EGLStream::FrameConsumer> m_consumer;
        NvVideoConverter *m_ImageConverter;
        std::queue < NvBuffer * > *m_ConvOutputPlaneBufQueue;
        pthread_mutex_t m_queueLock;
        pthread_cond_t m_queueCond;
        int conv_buf_num;
        int m_numPendingFrames;
    
        std::shared_ptr<camera_context> m_pContext;

        bool m_gotError;
    };

    Argus::UniqueObj<Argus::CameraProvider>  cameraProvider;
    Argus::UniqueObj<Argus::OutputStream>    outputStream;
    Argus::UniqueObj<Argus::CaptureSession>  captureSession;
    Argus::UniqueObj<Argus::Request>         request;

    std::unique_ptr<ConsumerThread> frameConsumerThread;

    uint32_t requestId;
};

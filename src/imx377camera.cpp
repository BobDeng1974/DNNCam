#include "imx377camera.hpp"
#include "frame_processor.hpp"
#include <EGLStream/NV/ImageNativeBuffer.h>
#include "Timer.hpp"

#define DEFAULT_FPS             (30)
#define FRAME_CONVERTER_BUF_NUMBER (10)
#define MAX_PENDING_FRAMES      (3)

#define EXIT_IF_NULL(val,msg,ret)   \
        {if (!val) { std::cout << msg << std::endl; return ret;}}
#define EXIT_IF_NOT_OK(val,msg,ret) \
        {if (val!=Argus::STATUS_OK) { std::cout << msg << std::endl; return ret;}}
#define EXIT_IF_LESS_THAN_ZERO(val,msg,ret) \
        {if (val < 0) { std::cout << msg << std::endl; return ret;}}

Imx377Camera::Imx377Camera(std::shared_ptr<camera_context> ctx) : Camera(ctx), requestId(0)
{

}

Imx377Camera::~Imx377Camera()
{
    std::cout << "Destroying camera" << std::endl;
}

bool Imx377Camera::_init()
{
    // Create the CameraProvider object and get the core interface.
    cameraProvider.reset(Argus::CameraProvider::create());
    Argus::ICameraProvider *iCameraProvider = Argus::interface_cast<Argus::ICameraProvider>(cameraProvider);
    EXIT_IF_NULL(iCameraProvider, "Cannot get core camera provider interface", false);

    // Get the camera devices.
    std::vector<Argus::CameraDevice*> cameraDevices;
    iCameraProvider->getCameraDevices(&cameraDevices);
    EXIT_IF_NULL(cameraDevices.size(), "No camera devices available", false);

    // Create the capture session using the first device and get the core interface.
    captureSession.reset(iCameraProvider->createCaptureSession(cameraDevices[0]));
    Argus::ICaptureSession *iCaptureSession = Argus::interface_cast<Argus::ICaptureSession>(captureSession);
    EXIT_IF_NULL(iCaptureSession, "Cannot get Capture Session Interface", false);
    Argus::ICameraProperties *iCameraDevice =
        Argus::interface_cast<Argus::ICameraProperties>(cameraDevices[0]);
    EXIT_IF_NULL(iCameraDevice, "Couldn't get camera properties", false);

    std::vector<Argus::SensorMode*> sensorModes;
    iCameraDevice->getBasicSensorModes(&sensorModes);
    for(int i = 0; i < sensorModes.size(); ++i) {
        Argus::ISensorMode *mode = Argus::interface_cast<Argus::ISensorMode>(sensorModes[i]);
        auto resolution = mode->getResolution();
        std::cout << "Sensor mode: " << i << " width: " << resolution.width() << " height: " << resolution.height() << " mode type: " << mode->getSensorModeType().getName() << std::endl;
        Argus::Range<uint64_t> limitExposureTimeRange = mode->getExposureTimeRange();
        std::cout << "\tExposure min: " << limitExposureTimeRange.min() << " max: " << limitExposureTimeRange.max() << std::endl;
        Argus::Range<float> sensorModeAnalogGainRange = mode->getAnalogGainRange();
        std::cout << "\tGain min: " << sensorModeAnalogGainRange.min() << " max: " << sensorModeAnalogGainRange.max() << std::endl;
    }

    Argus::IEventProvider *iEventProvider = Argus::interface_cast<Argus::IEventProvider>(captureSession);

    // Used for capture events
    /*std::vector<Argus::EventType> eventTypes;
    eventTypes.push_back(Argus::EVENT_TYPE_CAPTURE_COMPLETE);
    Argus::UniqueObj<Argus::EventQueue> queue(iEventProvider->createEventQueue(eventTypes));
    Argus::IEventQueue *iQueue = Argus::interface_cast<Argus::IEventQueue>(queue);*/

    // Create the OutputStream.
    Argus::UniqueObj<Argus::OutputStreamSettings> streamSettings(iCaptureSession->createOutputStreamSettings());
    Argus::IOutputStreamSettings *iStreamSettings = Argus::interface_cast<Argus::IOutputStreamSettings>(streamSettings);
    EXIT_IF_NULL(iStreamSettings, "Cannot get OutputStreamSettings Interface", false);

    iStreamSettings->setPixelFormat(Argus::PIXEL_FMT_YCbCr_420_888);
    Argus::ISensorMode *mode = Argus::interface_cast<Argus::ISensorMode>(sensorModes[0]);
    //iStreamSettings->setResolution(mode->getResolution());
    iStreamSettings->setResolution(Argus::Size2D<uint32_t>(_ctx->width, _ctx->height));
    outputStream.reset(iCaptureSession->createOutputStream(streamSettings.get()));

    // Launch the FrameConsumer thread to consume frames from the OutputStream.
    std::cout << "Launching consumer thread" << std::endl;
    frameConsumerThread.reset(new ConsumerThread(outputStream.get(), _ctx));
    frameConsumerThread->initialize();
    // Wait until the consumer is connected to the stream.
    frameConsumerThread->waitRunning();

    // Create capture request and enable output stream.
    //request.reset(iCaptureSession->createRequest());
    request.reset(iCaptureSession->createRequest(Argus::CAPTURE_INTENT_MANUAL));
    Argus::IRequest *iRequest = Argus::interface_cast<Argus::IRequest>(request);
    EXIT_IF_NULL(iRequest, "Failed to get capture request interface", false);
    iRequest->enableOutputStream(outputStream.get());

    Argus::ISourceSettings *iSourceSettings = Argus::interface_cast<Argus::ISourceSettings>(iRequest->getSourceSettings());
    EXIT_IF_NULL(iSourceSettings, "Unable to get ISOurceSettings interface", false);
    Argus::ISensorMode *iSensorMode = Argus::interface_cast<Argus::ISensorMode>(iSourceSettings->getSensorMode());
    EXIT_IF_NULL(iSensorMode, "Unable to get ISensorMode", false);
    Argus::Range<uint64_t> limitExposureTimeRange = iSensorMode->getExposureTimeRange();
    //iSourceSettings->setFrameDurationRange(Argus::Range<uint64_t>(1e9/DEFAULT_FPS));
    const uint64_t THIRD_OF_A_SECOND = 33333333;
    iSourceSettings->setExposureTimeRange(Argus::Range<uint64_t>(THIRD_OF_A_SECOND));
    Argus::Range<float> sensorModeAnalogGainRange = iSensorMode->getAnalogGainRange();

    iSourceSettings->setGainRange(Argus::Range<float>(sensorModeAnalogGainRange.min()));

    // Submit capture requests.
    std::cout << "Starting repeat capture requests" << std::endl;
    if (iCaptureSession->repeat(request.get()) != Argus::STATUS_OK) {
        std::cout << "Unable to start repeating capture request" << std::endl;
        return false;
    }

    std::cout << "Initialized camera" << std::endl;

    /*for(int i = 0; i < 5; ++i) {
        const uint64_t ONE_SECOND = 1000000000;
        iEventProvider->waitForEvents(queue.get(), ONE_SECOND);

        const Argus::Event* event = iQueue->getEvent(iQueue->getSize() - 1);
        const Argus::IEventCaptureComplete *iEventCaptureComplete
            = Argus::interface_cast<const Argus::IEventCaptureComplete>(event);

        const Argus::CaptureMetadata *metaData = iEventCaptureComplete->getMetadata();
        const Argus::ICaptureMetadata* iMetadata = Argus::interface_cast<const Argus::ICaptureMetadata>(metaData);

        uint64_t frameExposureTime = iMetadata->getSensorExposureTime();
        float frameGain = iMetadata->getSensorAnalogGain();
        std::cout << "exposureTime: " << frameExposureTime << " gain: " << frameGain << std::endl;
    }*/

    return true;
}

void Imx377Camera::stop()
{
    if (true == isInit()) {
        if (captureSession) {
            Argus::ICaptureSession *iCaptureSession = Argus::interface_cast<Argus::ICaptureSession>(captureSession);
            iCaptureSession->stopRepeat();
            iCaptureSession->waitForIdle();
        }
        outputStream.reset();
        if (frameConsumerThread) {
            frameConsumerThread->abort();
            frameConsumerThread->shutdown();
        }
        cameraProvider.reset();
    }
}

void Imx377Camera::set_exposure(const float exposure_time_ms)
{
    Argus::IRequest *iRequest = Argus::interface_cast<Argus::IRequest>(request);
    Argus::ISourceSettings *iSourceSettings = Argus::interface_cast<Argus::ISourceSettings>(iRequest->getSourceSettings());
    iSourceSettings->setExposureTimeRange(Argus::Range<uint64_t>(exposure_time_ms));
    Argus::ICaptureSession *iCaptureSession = Argus::interface_cast<Argus::ICaptureSession>(captureSession);
    iCaptureSession->repeat(request.get());
}

void Imx377Camera::set_gain(const int gain)
{
    Argus::IRequest *iRequest = Argus::interface_cast<Argus::IRequest>(request);
    Argus::ISourceSettings *iSourceSettings = Argus::interface_cast<Argus::ISourceSettings>(iRequest->getSourceSettings());
    iSourceSettings->setGainRange(Argus::Range<float>((float)gain));
    Argus::ICaptureSession *iCaptureSession = Argus::interface_cast<Argus::ICaptureSession>(captureSession);
    iCaptureSession->repeat(request.get());
}

void Imx377Camera::set_gain_red(const int gain)
{

}

void Imx377Camera::set_gain_green(const int gain)
{

}

void Imx377Camera::set_gain_blue(const int gain)
{

}

void Imx377Camera::set_gain_boost(bool gain_boost)
{

}

void Imx377Camera::set_pixel_clock(unsigned int speed_mhz)
{

}

void Imx377Camera::set_shutter_mode(const std::string &shutter_mode)
{

}

void Imx377Camera::set_mirror(bool mirror)
{

}

void Imx377Camera::set_flash_mode(const std::string &mode)
{

}

std::string Imx377Camera::get_flash_mode()
{
    // We don't have a flash
    return "off";
}

void Imx377Camera::set_strobe_auto(bool)
{

}

void Imx377Camera::set_strobe_parameters(int, unsigned int)
{

}

void Imx377Camera::set_log_mode(bool log_mode)
{

}

int Imx377Camera::get_log_mode_gain()
{
    return 0;
}

void Imx377Camera::set_log_mode_gain(const int gain)
{

}

int Imx377Camera::get_log_mode_value()
{
    return 0;
}

void Imx377Camera::set_log_mode_value(const int value)
{

}

FramePtr Imx377Camera::grab()
{
}

void Imx377Camera::start_capture()
{

}

bool Imx377Camera::_set_frame_rate(double fps)
{
    bool ret = false;
    return ret;
}

Imx377Camera::ConsumerThread::ConsumerThread(Argus::OutputStream* stream, std::shared_ptr<camera_context> ctx) :
        m_pContext(ctx),
        m_stream(stream),
        m_ImageConverter(NULL),
        m_gotError(false)
{
    conv_buf_num = FRAME_CONVERTER_BUF_NUMBER;
    m_ConvOutputPlaneBufQueue = new std::queue < NvBuffer * >;
    pthread_mutex_init(&m_queueLock, NULL);
    pthread_cond_init(&m_queueCond, NULL);
    m_numPendingFrames = 0;
}

Imx377Camera::ConsumerThread::~ConsumerThread()
{
    delete m_ConvOutputPlaneBufQueue;
    if (m_ImageConverter)
    {
        //if (DO_STAT)
             m_ImageConverter->printProfilingStats(std::cout);
        delete m_ImageConverter;
    }
}

bool Imx377Camera::ConsumerThread::threadInitialize()
{
    // Create the FrameConsumer.
    m_consumer = Argus::UniqueObj<EGLStream::FrameConsumer>(EGLStream::FrameConsumer::create(m_stream));
    EXIT_IF_NULL(m_consumer, "Failed to create FrameConsumer", false);

    // Create Video converter
    if (!createImageConverter())
    {
        std::cout << "Failed to create video m_ImageConverter" << std::endl;
        return false;
    }

    return true;
}

bool Imx377Camera::ConsumerThread::threadExecute()
{
    Argus::IStream *iStream = Argus::interface_cast<Argus::IStream>(m_stream);
    EGLStream::IFrameConsumer *iFrameConsumer = Argus::interface_cast<EGLStream::IFrameConsumer>(m_consumer);
    Timer t1;

    // Wait until the producer has connected to the stream.
    std::cout << "Waiting until producer is connected..." << std::endl;
    if (iStream->waitUntilConnected() != Argus::STATUS_OK)
    {
        std::cout << "Stream failed to connect" << std::endl;
        return false;
    }
    std::cout << "Producer has connected; continuing" << std::endl;
	auto prev_t = t1.GetTickCount();

    // Keep acquire frames and queue into converter
    while (!m_gotError)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];

        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, MAX_PLANES * sizeof(struct v4l2_plane));

        v4l2_buf.m.planes = planes;
        pthread_mutex_lock(&m_queueLock);
        while (!m_gotError &&
            ((m_ConvOutputPlaneBufQueue->empty()) || (m_numPendingFrames >= MAX_PENDING_FRAMES)))
        {
            pthread_cond_wait(&m_queueCond, &m_queueLock);
        }

        if (m_gotError)
        {
            pthread_mutex_unlock(&m_queueLock);
            break;
        }

        NvBuffer *buffer = NULL;
        int fd = -1;

        buffer = m_ConvOutputPlaneBufQueue->front();
        m_ConvOutputPlaneBufQueue->pop();
        pthread_mutex_unlock(&m_queueLock);

        // Acquire a frame.
        Argus::UniqueObj<EGLStream::Frame> frame(iFrameConsumer->acquireFrame());
        EGLStream::IFrame *iFrame = Argus::interface_cast<EGLStream::IFrame>(frame);
        if (!iFrame)
            break;

        // Get the IImageNativeBuffer extension interface and create the fd.
        EGLStream::NV::IImageNativeBuffer *iNativeBuffer =
            Argus::interface_cast<EGLStream::NV::IImageNativeBuffer>(iFrame->getImage());
        EXIT_IF_NULL(iNativeBuffer, "IImageNativeBuffer not supported by Image", false);
        fd = iNativeBuffer->createNvBuffer(Argus::Size2D<uint32_t>(m_pContext->width, m_pContext->height),
                                           NvBufferColorFormat_YUV420,
                                           NvBufferLayout_BlockLinear);

        // Push the frame into V4L2.
        v4l2_buf.index = buffer->index;
        // Set the bytesused to some non-zero value so that
        // v4l2 convert processes the buffer
        buffer->planes[0].bytesused = 1;
        buffer->planes[0].fd = fd;
        buffer->planes[1].fd = fd;
        buffer->planes[2].fd = fd;
        pthread_mutex_lock(&m_queueLock);
        m_numPendingFrames++;
        pthread_mutex_unlock(&m_queueLock);

        int ret = m_ImageConverter->output_plane.qBuffer(v4l2_buf, buffer);
        if (ret < 0) {
            abort();
            std::cout << "Fail to qbuffer for conv output plane" << std::endl;
        }
	auto next_t = t1.GetTickCount();
//	std::cout << "time taken = " << next_t - prev_t << " ms."<< std::endl;
	std::cout << "fps = " << 1000.00f/(next_t - prev_t) << std::endl;
	prev_t = next_t;
    }

    // Wait till capture plane DQ Thread finishes
    // i.e. all the capture plane buffers are dequeued
    m_ImageConverter->capture_plane.waitForDQThread(2000);

    std::cout << "consumer thread done" << std::endl;

    requestShutdown();

    return true;
}

bool Imx377Camera::ConsumerThread::threadShutdown()
{
    return true;
}

void Imx377Camera::ConsumerThread::writeFrameToOpencvConsumer(
    std::shared_ptr<camera_context> p_ctx, NvBuffer *buffer)
{
	Timer t4;
    NvBuffer::NvBufferPlane *plane = &buffer->planes[0];
    uint8_t *pdata = (uint8_t *) plane->data;

    cv::Mat imgbuf = cv::Mat(p_ctx->height, p_ctx->width, CV_8UC4, pdata);
    cv::Mat display_img;

    auto start_t = t4.GetTickCount();
    cv::cvtColor(imgbuf, display_img, cv::COLOR_RGBA2BGR);
    auto end_t = t4.GetTickCount();
    std::cout << "Time for cvtcolor = " << end_t-start_t << " ms."<< std::endl;
    //cv::cvtColor(imgbuf, display_img, cv::COLOR_RGBA2GRAY);
    //cv::imshow("img", display_img);
    //cv::waitKey();

    p_ctx->frame_processor->process_frame(FramePtr(new Frame(display_img)));
    //p_ctx->frame_processor->process_frame(FramePtr(new Frame(imgbuf)));
}

bool Imx377Camera::ConsumerThread::converterCapturePlaneDqCallback(
    struct v4l2_buffer *v4l2_buf,
    NvBuffer * buffer,
    NvBuffer * shared_buffer,
    void *arg)
{
    ConsumerThread *thiz = (ConsumerThread*)arg;
    std::shared_ptr<camera_context> p_ctx = thiz->m_pContext;
    int e;

    if (!v4l2_buf)
    {
        std::cout << "Failed to dequeue buffer from conv capture plane" << std::endl;
        thiz->abort();
        return false;
    }

    if (v4l2_buf->m.planes[0].bytesused == 0)
    {
        return false;
    }

    thiz->writeFrameToOpencvConsumer(p_ctx, buffer);

    e = thiz->m_ImageConverter->capture_plane.qBuffer(*v4l2_buf, NULL);
    EXIT_IF_LESS_THAN_ZERO(e, "qBuffer failed", false);

    return true;
}

bool Imx377Camera::ConsumerThread::converterOutputPlaneDqCallback(
    struct v4l2_buffer *v4l2_buf,
    NvBuffer * buffer,
    NvBuffer * shared_buffer,
    void *arg)
{
    ConsumerThread *thiz = (ConsumerThread*)arg;

    if (!v4l2_buf)
    {
        std::cout << "Failed to dequeue buffer from conv capture plane" << std::endl;
        thiz->abort();
        return false;
    }

    if (v4l2_buf->m.planes[0].bytesused == 0)
    {
        return false;
    }
    NvBufferDestroy(shared_buffer->planes[0].fd);

    pthread_mutex_lock(&thiz->m_queueLock);
    thiz->m_numPendingFrames--;
    thiz->m_ConvOutputPlaneBufQueue->push(buffer);
    pthread_cond_broadcast(&thiz->m_queueCond);
    pthread_mutex_unlock(&thiz->m_queueLock);

    return true;
}
bool Imx377Camera::ConsumerThread::createImageConverter()
{
    int ret = 0;

    // YUV420 --> RGB32 converter
    m_ImageConverter = NvVideoConverter::createVideoConverter("conv");
    EXIT_IF_NULL(m_ImageConverter, "Could not create m_ImageConverter", false);

    //if (DO_STAT)
        m_ImageConverter->enableProfiling();

    m_ImageConverter->capture_plane.
        setDQThreadCallback(converterCapturePlaneDqCallback);
    m_ImageConverter->output_plane.
        setDQThreadCallback(converterOutputPlaneDqCallback);

    ret = m_ImageConverter->setOutputPlaneFormat(V4L2_PIX_FMT_YUV420M, m_pContext->width,
                                    m_pContext->height, V4L2_NV_BUFFER_LAYOUT_BLOCKLINEAR);
    EXIT_IF_LESS_THAN_ZERO(ret, "Could not set output plane format", false);

    ret = m_ImageConverter->setCapturePlaneFormat(V4L2_PIX_FMT_ABGR32, m_pContext->width,
                                    m_pContext->height, V4L2_NV_BUFFER_LAYOUT_PITCH);
    EXIT_IF_LESS_THAN_ZERO(ret, "Could not set capture plane format", false);

    // Query, Export and Map the output plane buffers so that we can read
    // raw data into the buffers
    ret = m_ImageConverter->output_plane.setupPlane(V4L2_MEMORY_DMABUF, conv_buf_num, false, false);
    EXIT_IF_LESS_THAN_ZERO(ret, "Could not setup output plane", false);

    // Query, Export and Map the output plane buffers so that we can write
    // m_ImageConverteroded data from the buffers
    ret = m_ImageConverter->capture_plane.setupPlane(V4L2_MEMORY_MMAP, conv_buf_num, true, false);
    EXIT_IF_LESS_THAN_ZERO(ret, "Could not setup capture plane", false);

    // Add all empty conv output plane buffers to m_ConvOutputPlaneBufQueue
    for (uint32_t i = 0; i < m_ImageConverter->output_plane.getNumBuffers(); i++)
    {
        m_ConvOutputPlaneBufQueue->push(
            m_ImageConverter->output_plane.getNthBuffer(i));
    }

    // conv output plane STREAMON
    ret = m_ImageConverter->output_plane.setStreamStatus(true);
    EXIT_IF_LESS_THAN_ZERO(ret, "fail to set conv output stream on", false);

    // conv capture plane STREAMON
    ret = m_ImageConverter->capture_plane.setStreamStatus(true);
    EXIT_IF_LESS_THAN_ZERO(ret, "fail to set conv capture stream on", false);

    // Start threads to dequeue buffers on conv capture plane,
    // conv output plane and capture plane
    m_ImageConverter->capture_plane.startDQThread(this);
    m_ImageConverter->output_plane.startDQThread(this);

   // Enqueue all empty conv capture plane buffers
    for (uint32_t i = 0; i < m_ImageConverter->capture_plane.getNumBuffers(); i++)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];

        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, MAX_PLANES * sizeof(struct v4l2_plane));

        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;

        ret = m_ImageConverter->capture_plane.qBuffer(v4l2_buf, NULL);
        if (ret < 0) {
            abort();
            std::cout << "Error queueing buffer at conv capture plane" << std::endl;
        }
    }

    std::cout << "created video converter" << std::endl;
    return true;
}

void Imx377Camera::ConsumerThread::abort()
{
    m_ImageConverter->abort();
    m_gotError = true;
}


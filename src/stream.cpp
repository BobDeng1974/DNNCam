
#ifdef WIN32
// The Windows header windef.h causes a name conflict with std::min.
// We must #define NOMINMAX before we we include the OpenCV headers,
// which use std::min, in order to prevent this conflict.
// See http://stackoverflow.com/questions/5004858/stdmin-gives-error
#define NOMINMAX
#endif

#include <sstream>
#include <stdexcept>

#include <opencv/highgui.h>

#include "stream.hpp"

std::mutex Stream::_context_mutex;

Stream::Stream(const int width, const int height, const std::string host, const int port) :
    _send_frames(false),
    _timestamp(0),
    _host(host), 
    _port(port)
{
    _streamContext.width = width;
    _streamContext.height = height;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    /* init GStreamer */
    int argc=0;
    gst_init(&argc, NULL);

    _loop = g_main_loop_new (NULL, FALSE); 


    /* create a server instance */
    server = gst_rtsp_server_new ();

    /* get the mount points for this server, every server has a default object
    * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points (server);

    factory = gst_rtsp_media_factory_new ();

    std::cout << "Stream image size: " << _streamContext.width << "x" << _streamContext.height << std::endl; 
    /*g_object_set(G_OBJECT (clockoverlay), 
            "halignment", 2, 
            "valignment", 1, 
            "color", 4278255359,
            NULL);
*/
    gst_rtsp_media_factory_set_launch (factory,
        "( appsrc name=mysrc ! clockoverlay halignment=2 valignment=1 ! omxh265enc ! rtph265pay name=pay0 config-interval=3 pt=96 )");
    g_signal_connect(factory, "media-configure", (GCallback) media_configure, (gpointer)(&_streamContext));
//gst_rtsp_media_factory_set_launch (factory,
//            "( videotestsrc horizontal-speed=5 is-live=1 ! clockoverlay halignment=0 valignment=2 ! omxh264enc ! rtph264pay name=pay0 pt=96 )");
//gst_rtsp_media_factory_set_shared (factory, TRUE);

    /* attach the factory to the /stream url */
    gst_rtsp_mount_points_add_factory (mounts, "/stream", factory);

    /* don't need the ref to the mapper anymore */
    g_object_unref (mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach (server, NULL);
}

void Stream::media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data)
{
    std::lock_guard<std::mutex> lg(_context_mutex);
    streamContext* sc = (streamContext*)(user_data);

    GstElement* element = gst_rtsp_media_get_element(media);
    GstElement* appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");
    g_object_set(G_OBJECT(appsrc),
            "caps", gst_caps_new_simple ("video/x-raw",
                "format", G_TYPE_STRING, "I420",
                "width", G_TYPE_INT, sc->width,
                "height", G_TYPE_INT, sc->height,
                "framerate", GST_TYPE_FRACTION, 0, 1,
                NULL),
            "stream-type", 0,
            "min-latency", gst_util_uint64_scale_int (1, GST_SECOND, 2),
            "is-live", TRUE,
            "format", GST_FORMAT_TIME, NULL);
    sc->elementMap.insert(std::make_pair(appsrc, element));
}

void Stream::start()
{
    if (_thread_ptr.get()) 
    {
        return; 
    }

    _thread_ptr.reset(new boost::thread(boost::bind(&g_main_loop_run, _loop)));
}

void Stream::stop()
{
    /* clean up */
    g_main_loop_quit(_loop);
    if (!_thread_ptr.get()) 
    {
        return; 
    }
    _thread_ptr->join();
    _thread_ptr.reset();
}

void Stream::push_frame(const cv::Mat frame)
{
    std::lock_guard<std::mutex> lg(_context_mutex);
    if(_streamContext.elementMap.empty()) return;
    guint size = frame.rows * frame.cols * 3/2;

    auto itr = _streamContext.elementMap.begin();
    while (_streamContext.elementMap.end() != itr) {
        GstBuffer *buffer;
        GstFlowReturn ret;
        buffer = gst_buffer_new_allocate (NULL, size, NULL);
        //gst_buffer_memset (buffer, 0, 0xff, size);
 
        //memcpy(GST_BUFFER_DATA(buffer), frame.data, GST_BUFFER_SIZE(buffer));
        /* I420 => Y frame followed by 2x2 subsampled U/V frame
        * set the U/V to 128 for black & white */
        GstMapInfo map;
        if (gst_buffer_map (buffer, &map, GST_MAP_READ)) {
            if (frame.channels() == 1)
            {
                memcpy(map.data, frame.data, frame.rows*frame.cols);
                memset(map.data+frame.rows*frame.cols, 128, frame.rows*frame.cols/2);
            }
            else if(frame.channels() == 4)
            {
                rgba_to_i420(frame.data, map.data, frame.cols, frame.rows);
            }
            else
            {
                rgb_to_i420(frame.data, map.data, frame.cols, frame.rows);
            }
            gst_buffer_unmap (buffer, &map);
        } else {
            std::cout << "gst_buffer_map error" << std::endl;
        }

        const GstClockTime now = gst_clock_get_time(GST_ELEMENT_CLOCK(itr->first)) - gst_element_get_base_time(itr->first);
        GST_BUFFER_PTS(buffer) = now;
 
        g_signal_emit_by_name (itr->first, "push-buffer", buffer, &ret);
        gst_buffer_unref(buffer);
 
        if (ret != GST_FLOW_OK) {
            std::cout << "Something went wrong while streaming(most likely client disconnected): " << ret << std::endl;
            gst_object_unref(itr->first);
            gst_object_unref(itr->second);
            auto erase = itr;
            ++itr;
            _streamContext.elementMap.erase(erase);
        } else {
            ++itr;
        }
    }
}

Stream::~Stream() 
{
    stop(); 
    auto itr = _streamContext.elementMap.begin();
    while (_streamContext.elementMap.end() != itr) {
        auto erase = itr;
        ++itr;
        gst_object_unref(erase->first);
        gst_object_unref(erase->second);
        _streamContext.elementMap.erase(erase);
    }
    g_main_loop_unref(_loop);
}

void Stream::rgb_to_yuv(unsigned char b, unsigned char g, unsigned char r, unsigned char & y, unsigned char & u, unsigned char & v)
{
    y = cv::saturate_cast<unsigned char>((( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16);
    u = cv::saturate_cast<unsigned char>(((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128); 
    v = cv::saturate_cast<unsigned char>(((112 * r -  94 * g -  18 * b + 128) >> 8) + 128);
}

/* http://code.opencv.org/attachments/1116/rgb_to_yuv420.cpp */
void Stream::rgb_to_i420(unsigned char *rgb, unsigned char *yuv420, int width, int height)
{
    unsigned char * y_pixel = yuv420;
    unsigned char * u_pixel = yuv420 + width * height;
    unsigned char * v_pixel = yuv420 + width * height + (width * height / 4);

    int index = 0;
    int uv_index=0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const unsigned char b = rgb[3 * (y * width + x) + 0]; 
            const unsigned char g = rgb[3 * (y * width + x) + 1]; 
            const unsigned char r = rgb[3 * (y * width + x) + 2]; 
            if ((x%2==0) && (y%2==0))
            {
                u_pixel[uv_index]=cv::saturate_cast<unsigned char>(((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128);
                v_pixel[uv_index++]=cv::saturate_cast<unsigned char>(((112 * r -  94 * g -  18 * b + 128) >> 8) + 128);
            }
            y_pixel[index++] = cv::saturate_cast<unsigned char>((( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16);
        }
    }
}

/* http://code.opencv.org/attachments/1116/rgb_to_yuv420.cpp modified for alpha */
void Stream::rgba_to_i420(unsigned char *rgb, unsigned char *yuv420, int width, int height)
{
    unsigned char * y_pixel = yuv420;
    unsigned char * u_pixel = yuv420 + width * height;
    unsigned char * v_pixel = yuv420 + width * height + (width * height / 4);

    int index = 0;
    int uv_index=0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const unsigned char b = rgb[4 * (y * width + x) + 0]; 
            const unsigned char g = rgb[4 * (y * width + x) + 1]; 
            const unsigned char r = rgb[4 * (y * width + x) + 2]; 
            if ((x%2==0) && (y%2==0))
            {
                u_pixel[uv_index]=cv::saturate_cast<unsigned char>(((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128);
                v_pixel[uv_index++]=cv::saturate_cast<unsigned char>(((112 * r -  94 * g -  18 * b + 128) >> 8) + 128);
            }
            y_pixel[index++] = cv::saturate_cast<unsigned char>((( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16);
        }
    }
}

// The following code came from the link listed above.  It's a conversion using float instead of int arithmetic.
// It works, but the int code would probably run faster.  I'm leaving it here and can easily be swapped out.
/*
void Stream::rgb_to_yuv(unsigned char   b, unsigned char   g, unsigned char   r,
                       unsigned char & y, unsigned char & u, unsigned char & v)
{
    float yf, uf, vf;
    //Y = R * 0.299 + G * 0.587 + B * 0.114;
    //U = R * -0.169 + G * -0.332 + B * 0.500 + 128.0;
    //V = R * 0.500 + G * -0.419 + B * -0.0813 + 128.0;

    yf =    0.299f * static_cast<float>(r) +
            0.587f * static_cast<float>(g) +
            0.114f * static_cast<float>(b);
    yf = (yf > 255.0f) ? 255.0f: yf;
    yf = (yf < 0.0f) ? 0.0f: yf;
    y = static_cast<unsigned char>(yf);
    uf =   -0.169f * static_cast<float>(r) -
            0.332f * static_cast<float>(g) +
            0.500f * static_cast<float>(b) + 128.0;
    uf = (uf > 255.0f) ? 255.0f: uf;
    uf = (uf < 0.0f) ? 0.0f: uf;
    u = static_cast<unsigned char>(uf);
    vf =    0.500f * static_cast<float>(r) -
            0.419f * static_cast<float>(g) -
            0.081f * static_cast<float>(b) + 128.0;
    vf = (vf > 255.0f) ? 255.0f: vf;
    vf = (vf < 0.0f) ? 0.0f: vf;
    v = static_cast<unsigned char>(vf);
}

void Stream::rgb_to_i420(unsigned char *rgb, unsigned char *yuv420, int width, int height)
{
    unsigned char * y_pixel = yuv420;
    unsigned char * u_pixel = yuv420 + width * height;
    unsigned char * v_pixel = yuv420 + width * height + (width * height / 4);
    unsigned char * U_tmp = new unsigned char [width * height];
    unsigned char * V_tmp = new unsigned char [width * height];
    int index = 0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            rgb_to_yuv(rgb[3 * (y * width + x) + 0], rgb[3 * (y * width + x) + 1], rgb[3 * (y * width + x) + 2], y_pixel[index], U_tmp[index], V_tmp[index]);
            index++;
        }
    }
    index = 0;
    for (int y = 0; y < height; y+=2)
    {
        for (int x = 0; x < width; x+=2)
        {
            u_pixel[index] = U_tmp[y * width + x];
            v_pixel[index] = V_tmp[y * width + x];
            index++;
        }
    }
    delete [] U_tmp;
    delete [] V_tmp;
}*/

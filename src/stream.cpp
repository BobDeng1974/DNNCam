
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
#include "log.hpp"

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

void Stream::push_frame(const FrameCollection frame_col)
{
    std::lock_guard<std::mutex> lg(_context_mutex);
    if(_streamContext.elementMap.empty()) return;

    cv::Mat frame_y = frame_col.frame_y->to_mat();
    cv::Mat frame_u = frame_col.frame_u->to_mat();
    cv::Mat frame_v = frame_col.frame_v->to_mat();
    const guint size = frame_y.rows * frame_y.cols * 3/2;
    
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
        if (gst_buffer_map (buffer, &map, GST_MAP_READ))
        {
            // the data produced by libargus has stride != width, so we must copy by row...
            for(int j = 0; j < frame_y.rows; j++)
            {
                memcpy(map.data + j * frame_y.cols, frame_y.row(j).data, frame_y.cols);
            }
            for(int j = 0; j < frame_u.rows; j++)
            {
                memcpy(map.data + frame_y.rows * frame_y.cols + j * frame_u.cols, frame_u.row(j).data, frame_u.cols);
            }
            for(int j = 0; j < frame_v.rows; j++)
            {
                memcpy(map.data + frame_y.rows * frame_y.cols + frame_u.rows * frame_u.cols + j * frame_v.cols, frame_v.row(j).data, frame_v.cols);
            }
            
            gst_buffer_unmap (buffer, &map);
        }
        else {
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

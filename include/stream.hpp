#pragma once

#include <string>
#include <mutex>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <unordered_map>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include "frame.hpp"

namespace pt = boost::posix_time;

namespace BoulderAI
{

class Stream
{
public:
    Stream(const int width, const int height, const std::string host = "0.0.0.0", const int port = 9090);
    virtual ~Stream();

    void start();
    void stop(); 
    void push_frame(const FrameCollection frame_col);

protected:

    using ElementMap = std::unordered_map<GstElement*, GstElement*>;
    struct streamContext {
        int width;
        int height;
        ElementMap elementMap;
    } _streamContext;
    // Protect the streamContext
    static std::mutex _context_mutex;

    static void media_configure(GstRTSPMediaFactory * factory, GstRTSPMedia * media, gpointer user_data);
    static void rgb_to_i420(unsigned char *rgb, unsigned char *yuv420, int width, int height); 
    static void rgba_to_i420(unsigned char *rgb, unsigned char *yuv420, int width, int height); 
    static inline void rgb_to_yuv(unsigned char b, unsigned char g, unsigned char r, unsigned char & y, unsigned char & u, unsigned char & v);

    const std::string _host;
    const int _port;

    boost::shared_ptr<boost::thread> _thread_ptr;

    GMainLoop* _loop;
    bool _send_frames; 
    GstClockTime _timestamp; 
};

typedef boost::shared_ptr < Stream > StreamPtr;

} // namespace BoulderAI

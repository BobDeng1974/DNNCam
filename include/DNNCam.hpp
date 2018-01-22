#pragma once

#include <boost/program_options.hpp>
#include <opencv2/opencv.hpp>

#include "Argus/Argus.h"
#include "EGLStream/EGLStream.h"

#include "frame.hpp"

namespace po = boost::program_options;

namespace BoulderAI
{

struct ArgusReleaseData;
    
class DNNCam
{
public:
    static const char *OPT_WIDTH;
    static const char *OPT_HEIGHT;
    static const char *OPT_EXPOSURE_TIME_MIN;
    static const char *OPT_EXPOSURE_TIME_MAX;
    static const char *OPT_GAIN_MIN;
    static const char *OPT_GAIN_MAX;
    static const char *OPT_AWB_MODE;
    static const char *OPT_WB_GAINS;
    static const char *OPT_FRAMERATE;
    static const char *OPT_TIMEOUT;
    static const char *OPT_EXPOSURE_COMPENSATION;
    static const double DEFAULT_EXPOSURE_TIME_MIN;
    static const double DEFAULT_EXPOSURE_TIME_MAX;
    static const float DEFAULT_GAIN_MIN;
    static const float DEFAULT_GAIN_MAX;
    static const Argus::AwbMode DEFAULT_AWB_MODE;
    static const std::vector<float> DEFAULT_WB_GAINS;
    static const double DEFAULT_FRAMERATE;
    static const double DEFAULT_TIMEOUT;
    static const float DEFAULT_EXPOSURE_COMPENSATION;

    static po::options_description GetOptions();

    DNNCam(const uint32_t roi_x, const uint32_t roi_y,
                const uint32_t roi_width, const uint32_t roi_height,
                const uint32_t output_width, const uint32_t output_height,
                const double exposure_time_min = DEFAULT_EXPOSURE_TIME_MIN,
                const double exposure_time_max = DEFAULT_EXPOSURE_TIME_MAX,
                const float gain_min = DEFAULT_GAIN_MIN,
                const float gain_max = DEFAULT_GAIN_MAX,
                const Argus::AwbMode awb_mode = DEFAULT_AWB_MODE,
                const std::vector<float>& wb_gains = DEFAULT_WB_GAINS,
                const double framerate = DEFAULT_FRAMERATE,
                const double timeout = DEFAULT_TIMEOUT,
                const float exposure_compensation = DEFAULT_EXPOSURE_COMPENSATION );
    DNNCam( const po::variables_map &vm );
    ~DNNCam();


    bool init(); // Must be called first
    bool is_initialized();
    
    // Grabs the RGB frame. This call must be made before any of the other grab_* calls
    FramePtr grab(bool &dropped_frame, // return parameter: true if a frame was dropped since the last call
                  uint64_t &frame_num); // return parameter: the frame number of the returned frame, according to libargus
    FramePtr grab_y(); // Grabs just the 'Y' plane of a YUV image. This is equivalent to greyscale.
    FramePtr grab_u(); // Grabs just the 'U' plane of a YUV image. NOTE: this is half the size of the full frame
    FramePtr grab_v(); // Grabs just the 'V' plane of a YUV image. NOTE: this is half the size of the full frame
    
    void set_auto_exposure(const bool enabled);
    Argus::Range < uint64_t > get_exposure_time();
    void set_exposure_time(const Argus::Range < uint64_t > exposure_range);
    void set_exposure_compensation(const float comp);
    float get_exposure_compensation();
    void set_frame_duration(const Argus::Range < uint64_t > frame_range);
    Argus::Range < uint64_t > get_frame_duration();
    
    void set_gain(const Argus::Range < float > gain_range);
    Argus::Range < float > get_gain();

    void set_awb_mode(const Argus::AwbMode mode);
    Argus::AwbMode get_awb_mode();
    void set_awb(const bool enabled);
    void set_awb_gains(const float wb_gains[Argus::BAYER_CHANNEL_COUNT]); // For use with AWB_MODE_MANUAL
    
    void set_denoise_mode(const Argus::DenoiseMode mode);
    Argus::DenoiseMode get_denoise_mode();
    void set_denoise_strength(const float strength);
    float get_denoise_strength();
    
private:
    ArgusReleaseData *request_frame(bool &dropped_frame, uint64_t &frame_num);
    bool check_bounds();

    bool _initialized;
    cv::Mat cv_frame_rgb;
    cv::Mat cv_frame_y;
    cv::Mat cv_frame_u;
    cv::Mat cv_frame_v;
    const uint32_t _roi_x, _roi_y;
    const uint32_t _roi_width, _roi_height;
    const uint32_t _sensor_width, _sensor_height;
    const uint32_t _output_width, _output_height;
    const double _exposure_time_min, _exposure_time_max;
    const float _gain_min, _gain_max;
    Argus::AwbMode _awb_mode;
    const std::vector<float> _wb_gains;
    const double _framerate;
    const double _timeout;
    const float _exposure_compensation;

    Argus::UniqueObj<Argus::CameraProvider>    _camera_provider_object;
    Argus::UniqueObj<Argus::CaptureSession>    _capture_session_object;
    Argus::UniqueObj<Argus::OutputStream>      _output_stream_object;
    Argus::UniqueObj<EGLStream::FrameConsumer> _frame_consumer_object;
    Argus::UniqueObj<Argus::Request>           _request_object;
    Argus::SensorMode *_sensor_mode_object = nullptr;

  //Argus::UniqueObj < Argus::ICaptureSession > *capture_session;
  EGLStream::IFrameConsumer *frame_consumer;
};

typedef std::shared_ptr < DNNCam > DNNCamPtr;

}

#pragma once

#include <boost/program_options.hpp>
#include <opencv2/opencv.hpp>

#include "Argus/Argus.h"
#include "EGLStream/EGLStream.h"

#include "motordriver.hpp"
#include "frame.hpp"

namespace po = boost::program_options;

namespace BoulderAI
{

struct ArgusReleaseData;

// sample log handler using cout
void cout_log_handler(std::string output);
    
class DNNCam
{
public:
    // options names
    static const char *OPT_ROI_X;
    static const char *OPT_ROI_Y;
    static const char *OPT_ROI_W;
    static const char *OPT_ROI_H;
    static const char *OPT_OUTPUT_W;
    static const char *OPT_OUTPUT_H;
    static const char *OPT_AUTO_EXP_LOCK;
    static const char *OPT_EXP_TIME_MIN;
    static const char *OPT_EXP_TIME_MAX;
    static const char *OPT_FRAME_DUR_MIN;
    static const char *OPT_FRAME_DUR_MAX;
    static const char *OPT_GAIN_MIN;
    static const char *OPT_GAIN_MAX;
    static const char *OPT_AWB;
    static const char *OPT_AWB_MODE;
    static const char *OPT_WB_GAINS;
    static const char *OPT_TIMEOUT;
    static const char *OPT_EXPOSURE_COMPENSATION;
    static const char *OPT_DENOISE_MODE;
    static const char *OPT_DENOISE_STRENGTH;

    // option defaults
    static const uint32_t DEFAULT_ROI_X;
    static const uint32_t DEFAULT_ROI_Y;
    static const uint32_t DEFAULT_ROI_W;
    static const uint32_t DEFAULT_ROI_H;
    static const uint32_t DEFAULT_OUTPUT_W;
    static const uint32_t DEFAULT_OUTPUT_H;
    static const bool DEFAULT_AUTO_EXP_LOCK;
    static const double DEFAULT_EXP_TIME_MIN;
    static const double DEFAULT_EXP_TIME_MAX;
    static const double DEFAULT_FRAME_DUR_MIN;
    static const double DEFAULT_FRAME_DUR_MAX;
    static const float DEFAULT_GAIN_MIN;
    static const float DEFAULT_GAIN_MAX;
    static const bool DEFAULT_AWB;
    static const char *DEFAULT_AWB_MODE;
    static const std::vector < float > DEFAULT_WB_GAINS;
    static const double DEFAULT_TIMEOUT;
    static const float DEFAULT_EXPOSURE_COMPENSATION;
    static const char *DEFAULT_DENOISE_MODE;
    static const float DEFAULT_DENOISE_STRENGTH;

    // option variables
    static uint32_t _roi_x;
    static uint32_t _roi_y;
    static uint32_t _roi_width;
    static uint32_t _roi_height;
    static uint32_t _output_width;
    static uint32_t _output_height;
    static bool _auto_exp_lock;
    static double _exp_time_min;
    static double _exp_time_max;
    static double _frame_dur_min;
    static double _frame_dur_max;
    static float _gain_min;
    static float _gain_max;
    static bool _awb;
    static std::string _awb_mode;
    static std::vector < float > _wb_gains;
    static double _timeout;
    static float _exposure_compensation;
    static std::string _denoise_mode;
    static float _denoise_strength;

    static po::options_description GetOptions();

    // use the parameters set by boost program options
    DNNCam(boost::function < void(std::string) > log_callback = cout_log_handler);

    DNNCam(const uint32_t roi_x,
           const uint32_t roi_y,
           const uint32_t roi_width = DEFAULT_ROI_W,
           const uint32_t roi_height = DEFAULT_ROI_H,
           const uint32_t output_width = DEFAULT_OUTPUT_W,  // NOTE: if the output resolution is different than the
           const uint32_t output_height = DEFAULT_OUTPUT_H, //       ROI resolution, Argus scales to the output res
           boost::function < void(std::string) > log_callback = cout_log_handler,
           const bool auto_exp_lock = DEFAULT_AUTO_EXP_LOCK,
           const double exp_time_min = DEFAULT_EXP_TIME_MIN,
           const double exp_time_max = DEFAULT_EXP_TIME_MAX,
           const double frame_dur_min = DEFAULT_FRAME_DUR_MIN,
           const double frame_dur_max = DEFAULT_FRAME_DUR_MAX,
           const float gain_min = DEFAULT_GAIN_MIN,
           const float gain_max = DEFAULT_GAIN_MAX,
           const bool awb = DEFAULT_AWB,
           const Argus::AwbMode awb_mode = string_to_awb_mode(DEFAULT_AWB_MODE),
           const std::vector < float >& wb_gains = DEFAULT_WB_GAINS,
           const double timeout = DEFAULT_TIMEOUT,
           const float exposure_compensation = DEFAULT_EXPOSURE_COMPENSATION,
           const Argus::DenoiseMode denoise_mode = string_to_denoise_mode(DEFAULT_DENOISE_MODE),
           const float denoise_strength = DEFAULT_DENOISE_STRENGTH);
    
    ~DNNCam();

    static std::string awb_mode_to_string(const Argus::AwbMode mode);
    static std::string denoise_mode_to_string(const Argus::DenoiseMode mode);

    static Argus::AwbMode string_to_awb_mode(const std::string mode);
    static Argus::DenoiseMode string_to_denoise_mode(const std::string mode);

    bool init(); // Must be called first
    bool is_initialized();

    uint32_t get_output_width();
    uint32_t get_output_height();
    
    FramePtr grab(bool &dropped_frame); // Grabs the RGB frame. This call must be made before any of the other grab_* calls
                                        // Return parameter 'dropped_frame' is set to true if a frame was missed being read from
                                        // libargus since the last grab call. Call get_dropped_frames() to see how many
                                        // frames were missed.
    FramePtr grab_y(); // Grabs just the 'Y' plane of a YUV image. This is equivalent to greyscale.
    FramePtr grab_u(); // Grabs just the 'U' plane of a YUV image. NOTE: this is half the size of the full frame
    FramePtr grab_v(); // Grabs just the 'V' plane of a YUV image. NOTE: this is half the size of the full frame
    
    void set_auto_exposure_lock(const bool enabled);
    bool get_auto_exposure_lock();
    void set_exposure_time(const Argus::Range < uint64_t > exposure_range);
    Argus::Range < uint64_t > get_exposure_time();
    void set_exposure_compensation(const float comp);
    float get_exposure_compensation();
    void set_frame_duration(const Argus::Range < uint64_t > frame_range);
    Argus::Range < uint64_t > get_frame_duration();
    
    void set_gain(const Argus::Range < float > gain_range);
    Argus::Range < float > get_gain();

    void set_awb(const bool enabled);
    bool get_awb();
    void set_awb_mode(const Argus::AwbMode mode);
    Argus::AwbMode get_awb_mode();
    void set_awb_gains(std::vector < float > gains); // For use with AWB_MODE_MANUAL
    std::vector < float > get_awb_gains();
    
    void set_denoise_mode(const Argus::DenoiseMode mode);
    Argus::DenoiseMode get_denoise_mode();
    void set_denoise_strength(const float strength);
    float get_denoise_strength();

    // if the camera is producing frames faster than we can read them, this counter will increase
    uint64_t get_dropped_frames();
    
    //
    // Lens control
    // NOTE: The *_absolute(), *_home(), and *_location() functions do not work on the current hardware rev
    //

    // zoom should be limited to 7000 steps
    bool zoom_relative(const int steps);
    bool zoom_absolute(const int pos);
    bool zoom_home();
    int get_zoom_location();

    // focus should be limited to 7000 steps
    bool focus_relative(const int steps);
    bool focus_absolute(const int pos);
    bool focus_home();
    int get_focus_location();

    // iris should be limited to 120 steps
    bool iris_relative(const int steps);
    bool iris_absolute(const int pos);
    bool iris_home();
    int get_iris_location();

    bool set_ir_cut(const bool enabled);
    
    boost::function < void(std::string) > _log_callback;

private:
    ArgusReleaseData *request_frame(bool &dropped_frame);
    bool check_bounds();

    bool _initialized;
    cv::Mat cv_frame_rgb;
    cv::Mat cv_frame_y;
    cv::Mat cv_frame_u;
    cv::Mat cv_frame_v;
    const uint32_t _sensor_width;
    const uint32_t _sensor_height;
    uint64_t _dropped_frames;

    MotorDriver _motor;

    Argus::UniqueObj<Argus::CameraProvider>    _camera_provider_object;
    Argus::UniqueObj<Argus::CaptureSession>    _capture_session_object;
    Argus::UniqueObj<Argus::OutputStream>      _output_stream_object;
    Argus::UniqueObj<EGLStream::FrameConsumer> _frame_consumer_object;
    Argus::UniqueObj<Argus::Request>           _request_object;
    Argus::SensorMode *_sensor_mode_object = nullptr;
};

typedef std::shared_ptr < DNNCam > DNNCamPtr;

}

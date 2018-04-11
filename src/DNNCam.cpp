#include <sstream>

#include "DNNCam.hpp"

#include "EGLStream/NV/ImageNativeBuffer.h"
#include "Argus/Ext/InternalFrameCount.h"

using namespace std;

namespace BoulderAI
{
const char *DNNCam::OPT_ROI_X = "roi-x";
const char *DNNCam::OPT_ROI_Y = "roi-y";
const char *DNNCam::OPT_ROI_W = "roi-w";
const char *DNNCam::OPT_ROI_H = "roi-h";
const char *DNNCam::OPT_OUTPUT_W = "output-w";
const char *DNNCam::OPT_OUTPUT_H = "output-h";
const char *DNNCam::OPT_AUTO_EXP_LOCK = "auto-exp-lock";
const char *DNNCam::OPT_EXP_TIME_MIN = "exp-time-min";
const char *DNNCam::OPT_EXP_TIME_MAX = "exp-time-max";
const char *DNNCam::OPT_FRAME_DUR_MIN = "frame-dur-min";
const char *DNNCam::OPT_FRAME_DUR_MAX = "frame-dur-max";
const char *DNNCam::OPT_GAIN_MIN = "gain-min";
const char *DNNCam::OPT_GAIN_MAX = "gain-max";
const char *DNNCam::OPT_AWB = "awb";
const char *DNNCam::OPT_AWB_MODE = "awb-mode";
const char *DNNCam::OPT_WB_GAINS = "wb-gains";
const char *DNNCam::OPT_TIMEOUT = "timeout";
const char *DNNCam::OPT_EXPOSURE_COMPENSATION = "exposure-compensation";
const char *DNNCam::OPT_DENOISE_MODE = "denoise-mode";
const char *DNNCam::OPT_DENOISE_STRENGTH = "denoise-strength";

const uint32_t DNNCam::DEFAULT_ROI_X = 0;
const uint32_t DNNCam::DEFAULT_ROI_Y = 0;
const uint32_t DNNCam::DEFAULT_ROI_W = 3864;
const uint32_t DNNCam::DEFAULT_ROI_H = 2196;
const uint32_t DNNCam::DEFAULT_OUTPUT_W = 3864;
const uint32_t DNNCam::DEFAULT_OUTPUT_H = 2196;
const bool DNNCam::DEFAULT_AUTO_EXP_LOCK = false;
const double DNNCam::DEFAULT_EXP_TIME_MIN = 16000;
const double DNNCam::DEFAULT_EXP_TIME_MAX = 165770000;
const double DNNCam::DEFAULT_FRAME_DUR_MIN = 16666667;
const double DNNCam::DEFAULT_FRAME_DUR_MAX = 1462525056;
const float DNNCam::DEFAULT_GAIN_MIN = 1;
const float DNNCam::DEFAULT_GAIN_MAX = 180;
const bool DNNCam::DEFAULT_AWB = false;
const char *DNNCam::DEFAULT_AWB_MODE = "Auto";
const std::vector < float > DNNCam::DEFAULT_WB_GAINS = {1, 1, 1, 1};
const double DNNCam::DEFAULT_TIMEOUT = -1;
const float DNNCam::DEFAULT_EXPOSURE_COMPENSATION = 0;
const char *DNNCam::DEFAULT_DENOISE_MODE = "High Quality";
const float DNNCam::DEFAULT_DENOISE_STRENGTH = -1;

uint32_t DNNCam::_roi_x = DEFAULT_ROI_X;
uint32_t DNNCam::_roi_y = DEFAULT_ROI_Y;
uint32_t DNNCam::_roi_width = DEFAULT_ROI_W;
uint32_t DNNCam::_roi_height = DEFAULT_ROI_H;
uint32_t DNNCam::_output_width = DEFAULT_OUTPUT_W;
uint32_t DNNCam::_output_height = DEFAULT_OUTPUT_H;
bool DNNCam::_auto_exp_lock = DEFAULT_AUTO_EXP_LOCK;
double DNNCam::_exp_time_min = DEFAULT_EXP_TIME_MIN;
double DNNCam::_exp_time_max = DEFAULT_EXP_TIME_MAX;
double DNNCam::_frame_dur_min = DEFAULT_FRAME_DUR_MIN;
double DNNCam::_frame_dur_max = DEFAULT_FRAME_DUR_MAX;
float DNNCam::_gain_min = DEFAULT_GAIN_MIN;
float DNNCam::_gain_max = DEFAULT_GAIN_MAX;
bool DNNCam::_awb = DEFAULT_AWB;
string DNNCam::_awb_mode = DEFAULT_AWB_MODE;
std::vector < float > DNNCam::_wb_gains = DEFAULT_WB_GAINS;
double DNNCam::_timeout = DEFAULT_TIMEOUT;
float DNNCam::_exposure_compensation = DEFAULT_EXPOSURE_COMPENSATION;
string DNNCam::_denoise_mode = DEFAULT_DENOISE_MODE;
float DNNCam::_denoise_strength = DEFAULT_DENOISE_STRENGTH;
    
// we need to track this stuff to avoid an extra copy here....
// this will be used to free the nvbuffer data when a user is done with a frame
struct ArgusReleaseData
{
    ArgusReleaseData(const int rgb_fd, void *rgb, const int yuv_fd, void *y, void *u, void *v)
        :
        _rgb_fd(rgb_fd),
        _yuv_fd(yuv_fd),
        _rgb(rgb),
        _y(y),
        _u(u),
        _v(v)
    {
    }

    int _rgb_fd;
    int _yuv_fd;
    void *_rgb;
    void *_y;
    void *_u;
    void *_v;
};

static void argus_release_helper(void *opaque)
{
    ArgusReleaseData *data = (ArgusReleaseData *)opaque;
    if(data)
    {
        NvBufferMemUnMap(data->_rgb_fd, 0, &data->_rgb);
        NvBufferDestroy(data->_rgb_fd);

        NvBufferMemUnMap(data->_yuv_fd, 0, &data->_y);
        NvBufferMemUnMap(data->_yuv_fd, 1, &data->_u);
        NvBufferMemUnMap(data->_yuv_fd, 2, &data->_v);
        NvBufferDestroy(data->_yuv_fd);
        delete data;
    }
}
    
void cout_log_handler(string output)
{
    cout << output << endl;
}
    
po::options_description DNNCam::GetOptions()
{
    po::options_description desc( "DNNCam Options" );
    desc.add_options()
        ( OPT_ROI_X, po::value<uint32_t>(&_roi_x)->default_value(DEFAULT_ROI_X), "X position of ROI" )
        ( OPT_ROI_Y, po::value<uint32_t>(&_roi_y)->default_value(DEFAULT_ROI_Y), "Y position of ROI" )
        ( OPT_ROI_W, po::value<uint32_t>(&_roi_width)->default_value(DEFAULT_ROI_W), "Width of ROI" )
        ( OPT_ROI_H, po::value<uint32_t>(&_roi_height)->default_value(DEFAULT_ROI_H), "Height of ROI" )
        ( OPT_OUTPUT_W, po::value<uint32_t>(&_output_width)->default_value(DEFAULT_OUTPUT_W), "Width of output. Argus will scale the ROI to this width." )
        ( OPT_OUTPUT_H, po::value<uint32_t>(&_output_height)->default_value(DEFAULT_OUTPUT_H), "Height of output. Argus will scale the ROI to this height." )
        ( OPT_AUTO_EXP_LOCK, po::value < bool >(&_auto_exp_lock)->default_value(DEFAULT_AUTO_EXP_LOCK), "Auto exposure lock." )
        ( OPT_EXP_TIME_MIN, po::value<double>(&_exp_time_min)->default_value( DEFAULT_EXP_TIME_MIN ),
          "Minimum exposure time (in nS) The AE algorithm will strive to "
          "keep expsore time within this range. If negative, the default "
          "minimum exposure time of the sensor will be used. Determines max frame rate." )
        ( OPT_EXP_TIME_MAX, po::value<double>(&_exp_time_max)->default_value( DEFAULT_EXP_TIME_MAX ),
          "Maximum exposure time (in nS) The AE algorithm will strive to "
          "keep expsure time within this range. If negative, the default "
          "maximum exposure time of the sensor will be used. Determines max frame rate." )
        ( OPT_FRAME_DUR_MIN, po::value<double>(&_frame_dur_min)->default_value( DEFAULT_FRAME_DUR_MAX ),
          "Minimum frame duration (in nS), determines max frame rate" )
        ( OPT_FRAME_DUR_MAX, po::value<double>(&_frame_dur_max)->default_value( DEFAULT_FRAME_DUR_MAX ),
          "Maximum frame duration (in nS), determines max frame rate" )
        ( OPT_GAIN_MIN, po::value<float>(&_gain_min)->default_value( DEFAULT_GAIN_MIN ),
          "Minimum gain value to be used by the AE algorithm. If negative, the "
          "default minimum gain value for the sensor will be used" )
        ( OPT_GAIN_MAX, po::value<float>(&_gain_max)->default_value( DEFAULT_GAIN_MAX ),
          "Maximum gain value to be used by the AE algorithm. If negative, the "
          "default maximum gain value for the sensor will be used" )
        ( OPT_AWB, po::value < bool >(&_awb)->default_value(DEFAULT_AWB), "Auto white balance." )
        ( OPT_AWB_MODE, po::value<std::string>(&_awb_mode)->default_value(DEFAULT_AWB_MODE),
          "Auto white balance mode.")
        ( OPT_WB_GAINS, po::value<std::vector < float > >(&_wb_gains)->multitoken()->default_value( DEFAULT_WB_GAINS, "1 1 1 1" ),
          ("Vector of manual white balance gains [R, G_EVEN, G_ODD, B]. Only used if --" +
           std::string(OPT_AWB_MODE) + " is set to 'Manual'.").c_str() )
        ( OPT_TIMEOUT, po::value<double>(&_timeout)->default_value( DEFAULT_TIMEOUT ),
          "Timeout for acquiring frames. If this is negative, the timeout will be infinite." )
        ( OPT_EXPOSURE_COMPENSATION, po::value<float>(&_exposure_compensation)->default_value( DEFAULT_EXPOSURE_COMPENSATION ),
          "Exposure compensation, in (EV) stops." )
        ( OPT_DENOISE_MODE, po::value < string >(&_denoise_mode)->default_value(DEFAULT_DENOISE_MODE), "Denoise mode.")
        ( OPT_DENOISE_STRENGTH, po::value < float >(&_denoise_strength)->default_value(DEFAULT_DENOISE_STRENGTH), "Denoise strength.")
        ;
    return desc;
}

DNNCam::DNNCam(boost::function < void(std::string) > log_callback)
    :
    _initialized(false),
    _sensor_width(3864),
    _sensor_height(2196),
    _log_callback(log_callback),
    _dropped_frames(0),
    _motor(true, log_callback)
{
    if(!check_bounds())
    {
        ostringstream oss;
        oss << "Bounds checking failed... SensorW " << _sensor_width << " SensorH " << _sensor_height << " roix " << _roi_x << " roiy " << _roi_y
            << " roiwidth " << _roi_width << " roiheight " << _roi_height;
        _log_callback(oss.str());
        throw runtime_error(oss.str());
    }

    if(_wb_gains.size() != Argus::BAYER_CHANNEL_COUNT)
    {
        ostringstream oss;
        oss << "Requires 4 WB Gains, only got " << _wb_gains.size();
        throw runtime_error(oss.str());
    }
}
    
DNNCam::DNNCam(const uint32_t roi_x, const uint32_t roi_y,
               const uint32_t roi_width, const uint32_t roi_height,
               const uint32_t output_width, const uint32_t output_height,
               boost::function < void(std::string) > log_callback,
               const bool auto_exp_lock,
               const double exposure_time_min,
               const double exposure_time_max,
               const double frame_dur_min,
               const double frame_dur_max,
               const float gain_min,
               const float gain_max,
               const bool awb,
               const Argus::AwbMode awb_mode,
               const std::vector < float >& wb_gains,
               const double timeout,
               const float exposure_compensation,
               const Argus::DenoiseMode denoise_mode,
               const float denoise_strength)
:
    _initialized(false),
    _sensor_width(3864),
    _sensor_height(2196),
    _log_callback(log_callback),
    _dropped_frames(0),
    _motor(true, log_callback)
{
    _roi_x = roi_x;
    _roi_y = roi_y;
    _roi_width = roi_width;
    _roi_height = roi_height;
    _output_width = output_width;
    _output_height = output_height;
    _auto_exp_lock = auto_exp_lock;
    _exp_time_min = exposure_time_min;
    _exp_time_max = exposure_time_max;
    _frame_dur_min = frame_dur_min;
    _frame_dur_max = frame_dur_max;
    _gain_min = gain_min;
    _gain_max = gain_max;
    _awb = awb;
    _awb_mode = awb_mode_to_string(awb_mode);
    _wb_gains = wb_gains;
    _timeout = timeout;
    _exposure_compensation = exposure_compensation;
    _denoise_mode = denoise_mode_to_string(denoise_mode);
    _denoise_strength = denoise_strength;
    
    if(!check_bounds())
    {
        ostringstream oss;
        oss << "Bounds checking failed... SensorW " << _sensor_width << " SensorH " << _sensor_height << " roix " << _roi_x << " roiy " << _roi_y
            << " roiwidth " << _roi_width << " roiheight " << _roi_height;
        _log_callback(oss.str());
        throw runtime_error(oss.str());
    }

    if(wb_gains.size() != Argus::BAYER_CHANNEL_COUNT)
    {
        ostringstream oss;
        oss << "Requires 4 WB Gains, only got " << wb_gains.size();
        throw runtime_error(oss.str());
    }
}
    
DNNCam::~DNNCam()
{
    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->stopRepeat();
    capture_session->waitForIdle();
}

std::string DNNCam::awb_mode_to_string(const Argus::AwbMode mode)
{
    if(mode == Argus::AWB_MODE_OFF)
        return "Off";
    else if(mode == Argus::AWB_MODE_AUTO)
        return "Auto";
    else if(mode == Argus::AWB_MODE_INCANDESCENT)
        return "Incandescent";
    else if(mode == Argus::AWB_MODE_FLUORESCENT)
        return "Fluorescent";
    else if(mode == Argus::AWB_MODE_WARM_FLUORESCENT)
        return "Warm Fluorescent";
    else if(mode == Argus::AWB_MODE_DAYLIGHT)
        return "Daylight";
    else if(mode == Argus::AWB_MODE_CLOUDY_DAYLIGHT)
        return "Cloudy Daylight";
    else if(mode == Argus::AWB_MODE_TWILIGHT)
        return "Twilight";
    else if(mode == Argus::AWB_MODE_SHADE)
        return "Shade";
    else if(mode == Argus::AWB_MODE_MANUAL)
        return "Manual";
    else
        return "Unknown AWB Mode";
}
    
std::string DNNCam::denoise_mode_to_string(const Argus::DenoiseMode mode)
{
    if(mode == Argus::DENOISE_MODE_OFF)
        return "Off";
    else if(mode == Argus::DENOISE_MODE_FAST)
        return "Fast";
    else if(mode == Argus::DENOISE_MODE_HIGH_QUALITY)
        return "High Quality";
    else
        return "Unknown Denoise Mode";
}    

Argus::AwbMode DNNCam::string_to_awb_mode(const std::string mode)
{
    if(mode == "Off")
        return Argus::AWB_MODE_OFF;
    else if(mode == "Auto")
        return Argus::AWB_MODE_AUTO;
    else if(mode == "Incandescent")
        return Argus::AWB_MODE_INCANDESCENT;
    else if(mode == "Fluorescent")
        return Argus::AWB_MODE_FLUORESCENT;
    else if(mode == "Warm Fluorescent" || mode == "Warm%20Fluorescent")
        return Argus::AWB_MODE_WARM_FLUORESCENT;
    else if(mode == "Daylight")
        return Argus::AWB_MODE_DAYLIGHT;
    else if(mode == "Cloudy Daylight" || mode == "Cloudy%20Daylight")
        return Argus::AWB_MODE_CLOUDY_DAYLIGHT;
    else if(mode == "Twilight")
        return Argus::AWB_MODE_TWILIGHT;
    else if(mode == "Shade")
        return Argus::AWB_MODE_SHADE;
    else if(mode == "Manual")
        return Argus::AWB_MODE_MANUAL;

    // nothing good to use here...
    return Argus::AWB_MODE_OFF;
}

Argus::DenoiseMode DNNCam::string_to_denoise_mode(const std::string mode)
{
    if(mode == "Off")
        return Argus::DENOISE_MODE_OFF;
    else if(mode == "Fast")
        return Argus::DENOISE_MODE_FAST;
    else if(mode == "High Quality" || mode == "High%20Quality")
        return Argus::DENOISE_MODE_HIGH_QUALITY;

    // nothing good to use here...
    return Argus::DENOISE_MODE_OFF;
}

bool DNNCam::check_bounds()
{
    if((_roi_x + _roi_width <= _sensor_width) && (_roi_y + _roi_height <= _sensor_height))
        return true;
    return false;
}

void DNNCam::set_auto_exposure_lock(const bool auto_exp)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());        
        return;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto_control_settings->setAeLock(auto_exp);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->repeat(_request_object.get());
}

bool DNNCam::get_auto_exposure_lock()
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());        
        return false;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return false;
    }
    
    return auto_control_settings->getAeLock();
}

Argus::Range < uint64_t > DNNCam::get_exposure_time()
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return -1;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(request->getSourceSettings());
    if ( source_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return -1;
    }

    Argus::Range < uint64_t > exposure_range;
    exposure_range = source_settings->getExposureTimeRange();
    
    return exposure_range;
}

void DNNCam::set_exposure_time(const Argus::Range < uint64_t > exposure_range)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(request->getSourceSettings());
    if ( source_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return;
    }

    source_settings->setExposureTimeRange(exposure_range);

    Argus::Range < uint64_t > frame_range, exposure_range2;
    frame_range = source_settings->getFrameDurationRange();
    exposure_range2 = source_settings->getExposureTimeRange();
    
    cout << "exp " << exposure_range2.min() << " " << exposure_range2.max() << endl;
    cout << "frame " << frame_range.min() << " " << frame_range.max() << endl;

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->repeat(_request_object.get());
}

void DNNCam::set_exposure_compensation(const float comp)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto_control_settings->setExposureCompensation(comp);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->repeat(_request_object.get());
}

float DNNCam::get_exposure_compensation()
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return -9999;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return -9999;
    }
    
    return auto_control_settings->getExposureCompensation();
}

void DNNCam::set_frame_duration(const Argus::Range < uint64_t > frame_range)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(request->getSourceSettings());
    if ( source_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return;
    }

    source_settings->setFrameDurationRange(frame_range);
    

    Argus::Range < uint64_t > exposure_range, frame_range2;
    frame_range2 = source_settings->getFrameDurationRange();
    exposure_range = source_settings->getExposureTimeRange();
    
    cout << "exp " << exposure_range.min() << " " << exposure_range.max() << endl;
    cout << "frame " << frame_range2.min() << " " << frame_range2.max() << endl;

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->repeat(_request_object.get());
}

Argus::Range < uint64_t > DNNCam::get_frame_duration()
{
    Argus::Range < uint64_t > error(-1, -1);
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return error;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(request->getSourceSettings());
    if ( source_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return error;
    }

    return source_settings->getFrameDurationRange();
}

void DNNCam::set_gain(const Argus::Range < float > gain_range)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(request->getSourceSettings());
    if ( source_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return;
    }

    source_settings->setGainRange(gain_range);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->repeat(_request_object.get());
}

Argus::Range < float > DNNCam::get_gain()
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return -1;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(request->getSourceSettings());
    if ( source_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return -1;
    }

    Argus::Range < float > gain_range;
    gain_range = source_settings->getGainRange();

    return gain_range;
}

void DNNCam::set_awb_mode(const Argus::AwbMode mode)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return;
    }

    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return;
    }

    auto_control_settings->setAwbMode(mode);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->repeat(_request_object.get());
}

Argus::AwbMode DNNCam::get_awb_mode()
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return Argus::AWB_MODE_OFF;
    }

    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return Argus::AWB_MODE_OFF;
    }

    return auto_control_settings->getAwbMode();
}

void DNNCam::set_awb(const bool enabled)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto_control_settings->setAwbLock(enabled);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->repeat(_request_object.get());
}

bool DNNCam::get_awb()
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return false;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return false;
    }
    
    return auto_control_settings->getAwbLock();
}
    
void DNNCam::set_awb_gains(vector < float > gains)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return;
    }

    Argus::BayerTuple < float > bayer_gains(gains[0], gains[1], gains[2], gains[3]);
    auto_control_settings->setWbGains(bayer_gains);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }

    capture_session->repeat(_request_object.get());
}
    
vector < float > DNNCam::get_awb_gains()
{
    vector < float > ret;
    ret.resize(4);
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return ret;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return ret;
    }

    Argus::BayerTuple < float > bayer_gains;
    bayer_gains = auto_control_settings->getWbGains();
    ret = {bayer_gains[0], bayer_gains[1], bayer_gains[2], bayer_gains[3]};
    
    return ret;
}
    
void DNNCam::set_denoise_mode(const Argus::DenoiseMode mode)
{
    auto *denoise_settings = Argus::interface_cast<Argus::IDenoiseSettings>(_request_object);
    if (denoise_settings == nullptr)
    {
        ostringstream oss;
        oss << "Interface cast to IDenoiseSettings failed.";
        _log_callback(oss.str());
        return;
    }

    denoise_settings->setDenoiseMode(mode);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }
    capture_session->repeat(_request_object.get());
}

Argus::DenoiseMode DNNCam::get_denoise_mode()
{
    auto *denoise_settings = Argus::interface_cast<Argus::IDenoiseSettings>(_request_object);
    if (denoise_settings == nullptr)
    {
        ostringstream oss;
        oss << "Interface cast to IDenoiseSettings failed.";
        _log_callback(oss.str());
        return Argus::DENOISE_MODE_OFF;
    }

    return denoise_settings->getDenoiseMode();
}

void DNNCam::set_denoise_strength(const float strength)
{
    auto *denoise_settings = Argus::interface_cast<Argus::IDenoiseSettings>(_request_object);
    if (denoise_settings == nullptr)
    {
        ostringstream oss;
        oss << "Interface cast to IDenoiseSettings failed.";
        _log_callback(oss.str());
        return;
    }

    denoise_settings->setDenoiseStrength(strength);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return;
    }
    capture_session->repeat(_request_object.get());
}

float DNNCam::get_denoise_strength()
{
    auto *denoise_settings = Argus::interface_cast<Argus::IDenoiseSettings>(_request_object);
    if (denoise_settings == nullptr)
    {
        ostringstream oss;
        oss << "Interface cast to IDenoiseSettings failed.";
        _log_callback(oss.str());
        return -9999;
    }

    return denoise_settings->getDenoiseStrength();
}

bool DNNCam::is_initialized()
{
    return _initialized;
}

bool DNNCam::init()
{
    if ( is_initialized() ) {
        return true;
    }

    {
        ostringstream oss;
        _log_callback("Initializing DNNCam with the following parameters:");
        oss << OPT_ROI_X << ": " << _roi_x; _log_callback(oss.str()); oss.str("");
        oss << OPT_ROI_Y << ": " << _roi_y; _log_callback(oss.str()); oss.str("");
        oss << OPT_ROI_W << ": " << _roi_width; _log_callback(oss.str()); oss.str("");
        oss << OPT_ROI_H << ": " << _roi_height; _log_callback(oss.str()); oss.str("");
        oss << OPT_OUTPUT_W << ": " << _output_width; _log_callback(oss.str()); oss.str("");
        oss << OPT_OUTPUT_H << ": " << _output_height; _log_callback(oss.str()); oss.str("");
        oss << OPT_AUTO_EXP_LOCK << ": " << _auto_exp_lock; _log_callback(oss.str()); oss.str("");
        oss << OPT_EXP_TIME_MIN << ": " << _exp_time_min; _log_callback(oss.str()); oss.str("");
        oss << OPT_EXP_TIME_MAX << ": " << _exp_time_max; _log_callback(oss.str()); oss.str("");
        oss << OPT_FRAME_DUR_MIN << ": " << _frame_dur_min; _log_callback(oss.str()); oss.str("");
        oss << OPT_FRAME_DUR_MAX << ": " << _frame_dur_max; _log_callback(oss.str()); oss.str("");
        oss << OPT_GAIN_MIN << ": " << _gain_min; _log_callback(oss.str()); oss.str("");
        oss << OPT_GAIN_MAX << ": " << _gain_max; _log_callback(oss.str()); oss.str("");
        oss << OPT_AWB << ": " << _awb; _log_callback(oss.str()); oss.str("");
        oss << OPT_AWB_MODE << ": " << _awb_mode; _log_callback(oss.str()); oss.str("");
        oss << OPT_WB_GAINS << ": " << _wb_gains[0] << "," << _wb_gains[1] << "," << _wb_gains[2] << "," << _wb_gains[3]; _log_callback(oss.str()); oss.str("");
        oss << OPT_TIMEOUT << ": " << _timeout; _log_callback(oss.str()); oss.str("");
        oss << OPT_EXPOSURE_COMPENSATION << ": " << _exposure_compensation; _log_callback(oss.str()); oss.str("");
        oss << OPT_DENOISE_MODE << ": " << _denoise_mode; _log_callback(oss.str()); oss.str("");
        oss << OPT_DENOISE_STRENGTH << ": " << _denoise_strength; _log_callback(oss.str()); oss.str("");
    }
    
    Argus::Status status;

    // Get camera provider
    _camera_provider_object.reset( Argus::CameraProvider::create( &status ) );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to create camera provider. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    auto *camera_provider = Argus::interface_cast<Argus::ICameraProvider>(_camera_provider_object);
    if ( camera_provider == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICameraProvider failed.";
        _log_callback(oss.str());
        return false;
    }

    // Enumerate available devices
    std::vector<Argus::CameraDevice *> devices;
    status = camera_provider->getCameraDevices( &devices );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Could not get available cameras. Status: " << status;
        _log_callback(oss.str());
        return false;
    }
    if ( devices.empty() ) {
        ostringstream oss;
        oss <<  "No devices available.";
        _log_callback(oss.str());
        return false;
    }

    // Use the first reported device
    // TODO: For multiple camera support, this will need to change
    Argus::CameraDevice *device = devices[0];

    // Create a capture session
    _capture_session_object.reset( camera_provider->createCaptureSession(device, &status));
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to create a capture session. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return false;
    }

    // Get camera properties
    auto *camera_properties = Argus::interface_cast<Argus::ICameraProperties>(device);
    if ( camera_properties == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICameraProperties failed.";
        _log_callback(oss.str());
        return false;
    }

    // Get sensor modes to use
    std::vector<Argus::SensorMode *> sensor_mode_objects;
    status = camera_properties->getBasicSensorModes( &sensor_mode_objects );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Could not get available sensor modes. Status: " << status;
        _log_callback(oss.str());
        return false;
    }
    if ( sensor_mode_objects.empty()) {
        ostringstream oss;
        oss << "Camera reports no sensor modes.";
        _log_callback(oss.str());
        return false;
    }

    // TODO: Choose sensor mode based on closest resolution
    _sensor_mode_object = sensor_mode_objects[0];
    auto sensor_mode = Argus::interface_cast<Argus::ISensorMode>(_sensor_mode_object);
    if ( sensor_mode == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISensorMode failed.";
        _log_callback(oss.str());
        return false;
    }

    // Create settings object for the output stream
    Argus::UniqueObj<Argus::OutputStreamSettings> output_stream_settings_object(capture_session->createOutputStreamSettings( &status ));
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to create output stream settings. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    auto *output_stream_settings = Argus::interface_cast<Argus::IOutputStreamSettings>(output_stream_settings_object);
    if ( output_stream_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IOutputStreamSettings failed.";
        _log_callback(oss.str());
        return false;
    }

    // Configure stream settings
    status = output_stream_settings->setPixelFormat( Argus::PIXEL_FMT_YCbCr_420_888 );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set the pixel format. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    status = output_stream_settings->setResolution( Argus::Size2D<uint32_t>( _roi_width,
                                                                             _roi_height ) );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set the output resolution. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    status = output_stream_settings->setMetadataEnable( true );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't enable metadata output. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    status = output_stream_settings->setMode( Argus::STREAM_MODE_MAILBOX );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't make the stream operate in mailbox mode. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Create output stream
    _output_stream_object.reset( capture_session->createOutputStream(
                                     output_stream_settings_object.get(), &status ) );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to create output stream. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    auto *output_stream = Argus::interface_cast<Argus::IStream>(_output_stream_object);
    if ( output_stream == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IStream failed.";
        _log_callback(oss.str());
        return false;
    }

    // Create frame consumer
    _frame_consumer_object.reset(EGLStream::FrameConsumer::create(_output_stream_object.get(), 1, &status));
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to create frame consumer. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    auto *frame_consumer = Argus::interface_cast<EGLStream::IFrameConsumer>(_frame_consumer_object);
    if ( frame_consumer == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IFrameConsumer failed.";
        _log_callback(oss.str());
        return false;
    }

    // Construct capture request
    _request_object.reset(capture_session->createRequest(Argus::CAPTURE_INTENT_STILL_CAPTURE, &status));
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to create capture request. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IRequest failed.";
        _log_callback(oss.str());
        return false;
    }

    status = request->enableOutputStream( _output_stream_object.get() );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss <<"Couldn't set the output stream. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    auto stream_settings = Argus::interface_cast < Argus::IStreamSettings > (request->getStreamSettings(_output_stream_object.get()));
    if(stream_settings == nullptr)
    {
        ostringstream oss;
        oss << "Interface cast to IStreamSettings failed.";
        _log_callback(oss.str());
        return false;
    }

    Argus::Rectangle < float > rect((float)_roi_x / _sensor_width, (float)_roi_y / _sensor_height, (float)(_roi_x + _roi_width) / _sensor_width, (float)(_roi_y + _roi_height) / _sensor_height);
    status = stream_settings->setSourceClipRect(rect);
    if(status != Argus::STATUS_OK)
    {
        ostringstream oss;
        oss << "Couldn't set the clipping rect, status: " << status;
        _log_callback(oss.str());
        return false;
    }
  
    // Set capture settings
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(request->getSourceSettings());
    if ( source_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return false;
    }

    status = source_settings->setSensorMode( _sensor_mode_object );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set the sensor mode. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Set exposure time
    status = source_settings->setExposureTimeRange(Argus::Range<uint64_t>( _exp_time_min, _exp_time_max) );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set exposure time range. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Set frame duration
    status = source_settings->setFrameDurationRange(Argus::Range<uint64_t>( _frame_dur_min, _frame_dur_max ));
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set frame duration range. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Set gain range
    status = source_settings->setGainRange( Argus::Range<float>(_gain_min, _gain_max) );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set gain range. Status: "  << status;
        _log_callback(oss.str());
        return false;
    }

    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(request->getAutoControlSettings());
    if ( auto_control_settings == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ISourceSettings failed.";
        _log_callback(oss.str());
        return false;
    }

    // Set Auto Exposure lock
    status = auto_control_settings->setAeLock(_auto_exp_lock);
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set auto exposure lock. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Set awb lock
    status = auto_control_settings->setAwbLock(_awb);
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set awb lock. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Set auto white balance mode
    status = auto_control_settings->setAwbMode(string_to_awb_mode(_awb_mode));
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't auto white balance mode. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Set white balance gains if white balance mode is manual
    if ( string_to_awb_mode(_awb_mode) == Argus::AWB_MODE_MANUAL ) {
        status = auto_control_settings->setWbGains(
            Argus::BayerTuple<float>( _wb_gains[0],
                                      _wb_gains[1],
                                      _wb_gains[2],
                                      _wb_gains[3] )
            );
        if ( status != Argus::STATUS_OK ) {
            ostringstream oss;
            oss << "Couldn't set white balance gains. Status: " << status;
            _log_callback(oss.str());
            return false;
        }
    }

    // Set exposure compensation
    status = auto_control_settings->setExposureCompensation( _exposure_compensation );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set exposure compensation. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Set denoise mode
    auto *denoise_settings = Argus::interface_cast<Argus::IDenoiseSettings>(_request_object);
    if (denoise_settings == nullptr)
    {
        ostringstream oss;
        oss << "Interface cast to IDenoiseSettings failed.";
        _log_callback(oss.str());
        return false;
    }
    status = denoise_settings->setDenoiseMode(string_to_denoise_mode(_denoise_mode));
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set denoise mode. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Set denoise strength
    status = denoise_settings->setDenoiseStrength(_denoise_strength);
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Couldn't set denoise strength. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    // Submit capture request on repeat
    status = capture_session->repeat( _request_object.get() );
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to submit repeating capture request. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    status = output_stream->waitUntilConnected();
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to connect output stream. Status: " << status;
        _log_callback(oss.str());
        return false;
    }

    return _initialized = true;
}

uint32_t DNNCam::get_output_width()
{
    return _output_width;
}
    
uint32_t DNNCam::get_output_height()
{
    return _output_height;
}
    
ArgusReleaseData *DNNCam::request_frame(bool &dropped_frame)
{
    if ( !is_initialized()) {
        ostringstream oss;
        oss << "Camera is not initialized.";
        _log_callback(oss.str());
        return NULL;
    }

    // Obtain capture session
    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(_capture_session_object);
    if ( capture_session == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to ICaptureSession failed.";
        _log_callback(oss.str());
        return NULL;
    }

    // Obtain frame consumer
    auto *frame_consumer = Argus::interface_cast<EGLStream::IFrameConsumer>(_frame_consumer_object);
    if ( frame_consumer == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IFrameConsumer failed.";
        _log_callback(oss.str());
        return NULL;
    }

    // Acquire frame from frame consumer
    Argus::Status status;
    uint64_t timeout = Argus::TIMEOUT_INFINITE;
    if ( _timeout > 0 ) {
        timeout = static_cast<uint64_t>( _timeout * pow( 10, 9 ) );
    }

    Argus::UniqueObj<EGLStream::Frame> frame_object( frame_consumer->acquireFrame(timeout, &status));
    if ( status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to acquire frame. Status: " << status;
        _log_callback(oss.str());
        return NULL;
    }

    auto frame = Argus::interface_cast<EGLStream::IFrame>( frame_object );
    if ( frame == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IFrame failed.";
        _log_callback(oss.str());
        return NULL;
    }

    EGLStream::IArgusCaptureMetadata *iArgusCaptureMetadata = Argus::interface_cast<EGLStream::IArgusCaptureMetadata>(frame_object);
    if(!iArgusCaptureMetadata)
    {
        _log_callback("Interface cast to Iarguscapturemetadata failed.");
        return NULL;
    }
    Argus::CaptureMetadata *metadata = iArgusCaptureMetadata->getMetadata();
    Argus::ICaptureMetadata *iMetadata = Argus::interface_cast<Argus::ICaptureMetadata>(metadata);
    
    auto *frame_count = Argus::interface_cast < Argus::Ext::IInternalFrameCount >(metadata);
    if(frame_count == nullptr)
    {
        _log_callback("Interface cast to IInternalFrameCount failed.");
        return NULL;
    }
    static uint64_t last_frame_num = 0;
    uint64_t this_frame_num = frame_count->getInternalFrameCount();
    dropped_frame = false;
    if((this_frame_num != last_frame_num + 1) && (last_frame_num != 0))
    {
        ostringstream oss;
        _dropped_frames += this_frame_num - last_frame_num - 1;
        oss << "Missed frame! last " << last_frame_num << " this " << this_frame_num << " dropped " << _dropped_frames;
        _log_callback(oss.str());
        dropped_frame = true;
    }
    last_frame_num = this_frame_num;
  
    // Get image from frame
    EGLStream::Image *image_object = frame->getImage();
    auto *image_native_buffer =
        Argus::interface_cast<EGLStream::NV::IImageNativeBuffer>( image_object );
    if ( image_native_buffer == nullptr ) {
        ostringstream oss;
        oss << "Interface cast to IImageNativeBuffer failed.";
        _log_callback(oss.str());
        return NULL;
    }

    Argus::Size2D<uint32_t> image_size( _output_width, _output_height );
    int fd_yuv = image_native_buffer->createNvBuffer( image_size,
                                                      NvBufferColorFormat_YUV420,
                                                      NvBufferLayout_Pitch,
                                                      &status );
    if ( fd_yuv == -1 || status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to create NvBuffer! Status: " << status;
        _log_callback(oss.str());
        return NULL;
    }

    int fd_rgb = image_native_buffer->createNvBuffer( image_size,
                                                      NvBufferColorFormat_ARGB32,
                                                      NvBufferLayout_Pitch,
                                                      &status );
    if ( fd_rgb == -1 || status != Argus::STATUS_OK ) {
        ostringstream oss;
        oss << "Failed to create NvBuffer! Status: " << status;
        _log_callback(oss.str());
        return NULL;
    }

    NvBufferParams params_yuv;
    NvBufferGetParams( fd_yuv, &params_yuv );
    NvBufferParams params_rgb;
    NvBufferGetParams( fd_rgb, &params_rgb );

    if (params_yuv.num_planes != 3) {
        ostringstream oss;
        oss << "Buffer doesn't have the correct number of planes. (" << params_yuv.num_planes << " != 3)";
        _log_callback(oss.str());
        return NULL;
    }
    if (params_rgb.num_planes != 1) {
        ostringstream oss;
        oss << "Buffer doesn't have the correct number of planes. (" << params_rgb.num_planes << " != 1)";
        _log_callback(oss.str());
        return NULL;
    }
  
    void *plane_buffer_y;
    void *plane_buffer_u;
    void *plane_buffer_v;
    NvBufferMemMap(fd_yuv, 0, NvBufferMem_Read, &plane_buffer_y);
    NvBufferMemMap(fd_yuv, 1, NvBufferMem_Read, &plane_buffer_u);
    NvBufferMemMap(fd_yuv, 2, NvBufferMem_Read, &plane_buffer_v);
    NvBufferMemSyncForCpu(fd_yuv, 0, &plane_buffer_y);
    NvBufferMemSyncForCpu(fd_yuv, 1, &plane_buffer_u);
    NvBufferMemSyncForCpu(fd_yuv, 2, &plane_buffer_v);
    cv_frame_y = cv::Mat(params_yuv.height[0], params_yuv.width[0],
                         CV_8U, plane_buffer_y, params_yuv.pitch[0]);
    cv_frame_u = cv::Mat(params_yuv.height[1], params_yuv.width[1],
                         CV_8U, plane_buffer_u, params_yuv.pitch[1]);
    cv_frame_v = cv::Mat(params_yuv.height[2], params_yuv.width[2],
                         CV_8U, plane_buffer_v, params_yuv.pitch[2]);
  
    void *plane_buffer_rgb;
    NvBufferMemMap(fd_rgb, 0, NvBufferMem_Read, &plane_buffer_rgb);
    NvBufferMemSyncForCpu(fd_rgb, 0, &plane_buffer_rgb);
    cv_frame_rgb = cv::Mat(params_rgb.height[0], params_rgb.width[0],
                           CV_8UC4, plane_buffer_rgb, params_rgb.pitch[0]);

    // NOTE:
    // In order to avoid copying all the data here, the calls to NvBufferMemUnMap()
    // and NvBufferDestroy() are deferred until the user is finished with the data.
    // The provided 'Frame' class will call these as part of it's dtor through the
    // release_callback.

    return new ArgusReleaseData(fd_rgb, plane_buffer_rgb, fd_yuv, plane_buffer_y, plane_buffer_u, plane_buffer_v);
}

uint64_t DNNCam::get_dropped_frames()
{
    return _dropped_frames;
}
    
FramePtr DNNCam::grab(bool &dropped_frame)
{
    ArgusReleaseData *data = request_frame(dropped_frame);
    return FramePtr(new Frame(cv_frame_rgb, data, argus_release_helper));
}

FramePtr DNNCam::grab_y()
{
    return FramePtr(new Frame(cv_frame_y, NULL, argus_release_helper));
}
FramePtr DNNCam::grab_y_copy()
{
    cv::Mat newframe = cv_frame_y.clone();
    return FramePtr(new Frame(newframe, NULL, argus_release_helper));
}
FramePtr DNNCam::grab_u_copy()
{
    cv::Mat newframe = cv_frame_u.clone();
    return FramePtr(new Frame(newframe, NULL, argus_release_helper));
}
FramePtr DNNCam::grab_v_copy()
{
    cv::Mat newframe = cv_frame_v.clone();
    return FramePtr(new Frame(newframe, NULL, argus_release_helper));
}

FramePtr DNNCam::grab_u()
{
    return FramePtr(new Frame(cv_frame_u, NULL, argus_release_helper));
}

FramePtr DNNCam::grab_v()
{
    return FramePtr(new Frame(cv_frame_v, NULL, argus_release_helper));
}

bool DNNCam::zoom_relative(const int steps)
{
    return _motor.zoomRelative(steps);
}
    
bool DNNCam::zoom_absolute(const int pos)
{
    return _motor.zoomAbsolute(pos);
}

bool DNNCam::zoom_home()
{
    return _motor.zoomHome();
}

int DNNCam::get_zoom_location()
{
    return _motor.zoomAbsoluteLocation();
}

bool DNNCam::focus_relative(const int steps)
{
    return _motor.focusRelative(steps);
}

bool DNNCam::focus_absolute(const int pos)
{
    return _motor.focusAbsolute(pos);
}

bool DNNCam::focus_home()
{
    return _motor.focusHome();
}

int DNNCam::get_focus_location()
{
    return _motor.focusAbsoluteLocation();
}    

bool DNNCam::iris_relative(const int steps)
{
    return _motor.irisRelative(steps);
}
    
bool DNNCam::iris_absolute(const int pos)
{
    return _motor.irisAbsolute(pos);
}

bool DNNCam::iris_home()
{
    return _motor.irisHome();
}

int DNNCam::get_iris_location()
{
    return _motor.irisAbsoluteLocation();
}

bool DNNCam::set_ir_cut(const bool enabled)
{
    if(enabled)
        return _motor.ircutOn();
    else
        return _motor.ircutOff();
}
    
} // namespace BoulderAI

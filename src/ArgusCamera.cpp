#include <sstream>

#include "ArgusCamera.hpp"
#include "log.hpp"
#include "CameraFrame.hpp"

#include "EGLStream/NV/ImageNativeBuffer.h"

//#include "amp_camera/argus_utils.hpp"

//namespace amp {
//namespace camera {

const char *ArgusCamera::OPT_WIDTH = "argus-camera-width";
const char *ArgusCamera::OPT_HEIGHT = "argus-camera-height";
const char *ArgusCamera::OPT_EXPOSURE_TIME_MIN = "argus-camera-exposure-time-min";
const char *ArgusCamera::OPT_EXPOSURE_TIME_MAX = "argus-camera-exposure-time-max";
const char *ArgusCamera::OPT_GAIN_MIN = "argus-camera-gain-min";
const char *ArgusCamera::OPT_GAIN_MAX = "argus-camera-gain-max";
const char *ArgusCamera::OPT_AWB_MODE = "argus-camera-awb-mode";
const char *ArgusCamera::OPT_WB_GAINS = "argus-camera-wb-gains";
const char *ArgusCamera::OPT_FRAMERATE = "argus-camera-framerate";
const char *ArgusCamera::OPT_TIMEOUT = "argus-camera-timeout";
const char *ArgusCamera::OPT_EXPOSURE_COMPENSATION = "argus-camera-exposure-compensation";

const double ArgusCamera::DEFAULT_EXPOSURE_TIME_MIN = -1;
const double ArgusCamera::DEFAULT_EXPOSURE_TIME_MAX = -1;
const float ArgusCamera::DEFAULT_GAIN_MIN = -1;
const float ArgusCamera::DEFAULT_GAIN_MAX = -1;
const AutoWhiteBalanceMode ArgusCamera::DEFAULT_AWB_MODE = AutoWhiteBalanceMode::AUTO;
const std::vector<float> ArgusCamera::DEFAULT_WB_GAINS = { 1, 1, 1, 1 };
const double ArgusCamera::DEFAULT_FRAMERATE = 30;
const double ArgusCamera::DEFAULT_TIMEOUT = -1;
const float ArgusCamera::DEFAULT_EXPOSURE_COMPENSATION = 0;

using namespace std;

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

po::options_description ArgusCamera::GetOptions()
{
    po::options_description desc( "ArgusCamera Options" );
    desc.add_options()
        ( OPT_WIDTH, po::value<uint32_t>(), "Pixel width of output video" )
        ( OPT_HEIGHT, po::value<uint32_t>(), "Pixel height of output video" )
        ( OPT_EXPOSURE_TIME_MIN,
          po::value<double>()->default_value( DEFAULT_EXPOSURE_TIME_MIN ),
          "Minimum exposure time (in seconds.) The AE algorithm will strive to "
          "keep expsore time within this range. If negative, the default "
          "minimum exposure time of the sensor will be used" )
        ( OPT_EXPOSURE_TIME_MAX,
          po::value<double>()->default_value( DEFAULT_EXPOSURE_TIME_MAX ),
          "Maximum exposure time (in seconds.) The AE algorithm will strive to "
          "keep expsure time within this range. If negative, the default "
          "maximum exposure time of the sensor will be used" )
        ( OPT_GAIN_MIN, po::value<float>()->default_value( DEFAULT_GAIN_MIN ),
          "Minimum gain value to be used by the AE algorithm. If negative, the "
          "default minimum gain value for the sensor will be used" )
        ( OPT_GAIN_MAX, po::value<float>()->default_value( DEFAULT_GAIN_MAX ),
          "Maximum gain value to be used by the AE algorithm. If negative, the "
          "default maximum gain value for the sensor will be used" )
        ( OPT_AWB_MODE, po::value<std::string>()->default_value(
            AutoWhiteBalanceModeToString( DEFAULT_AWB_MODE ) ),
          // "Auto"),
          "Auto white balance mode. Options: off, auto, incandescent, florescent,"
          "warm_fluorescent, daylight, cloudy_daylight, twilight, shade, or"
          "manual." )
        ( OPT_WB_GAINS, po::value<std::vector<float>>()->multitoken()
          ->default_value( DEFAULT_WB_GAINS, "1 1 1 1" ), ("Vector of manual"
                                                           " white balance gains [R, G_EVEN, G_ODD, B]. Only used if --" +
                                                           std::string(OPT_AWB_MODE) + " is set to 'manual'.").c_str() )
        ( OPT_FRAMERATE, po::value<double>()->default_value( DEFAULT_FRAMERATE ),
          "Framerate at which to have the sensor capture images. If this is "
          "negative, the fastest framerate for the sensor is used. This will "
          "not effect the runrate/framerate of this process. This value should"
          " be higher than the process's configured runrate/framerate to avoid"
          " obtaining duplicate frames.")
        ( OPT_TIMEOUT, po::value<double>()->default_value( DEFAULT_TIMEOUT ),
          "Timeout for acquiring frames. If this is negative, the timeout will be"
          " infinite." )
        ( OPT_EXPOSURE_COMPENSATION, po::value<float>()
          ->default_value( DEFAULT_EXPOSURE_COMPENSATION ),
          "Exposure compensation, in (EV) stops." )
        ;
    return desc;
}

ArgusCamera::ArgusCamera(const uint32_t roi_x, const uint32_t roi_y,
                         const uint32_t roi_width, const uint32_t roi_height,
                         const uint32_t output_width, const uint32_t output_height,
                         const double exposure_time_min,
                         const double exposure_time_max,
                         const float gain_min,
                         const float gain_max,
                         const AutoWhiteBalanceMode awb_mode,
                         const std::vector<float>& wb_gains,
                         const double framerate,
                         const double timeout,
                         const float exposure_compensation )
:
    _initialized(false),
    _roi_x(roi_x),
    _roi_y(roi_y),
    _roi_width(roi_width),
    _roi_height(roi_height),
    _sensor_width(3864),
    _sensor_height(2196),
    _output_width(output_width),
    _output_height(output_height),
    _exposure_time_min( exposure_time_min ),
    _exposure_time_max( exposure_time_max ),
    _gain_min( gain_min ),
    _gain_max( gain_max ),
    _awb_mode( awb_mode ),
    _wb_gains( wb_gains ),
    _framerate( framerate ),
    _timeout( timeout ),
    _exposure_compensation( exposure_compensation )
{
    if(!check_bounds())
    {
        ostringstream oss;
        oss << "Bounds checking failed... SensorW " << _sensor_width << " SensorH " << _sensor_height << " roix " << _roi_x << " roiy " << _roi_y
            << " roiwidth " << _roi_width << " roiheight " << _roi_height;
        bl_log_error(oss.str());
        throw runtime_error(oss.str());
    }
}

ArgusCamera::ArgusCamera( const po::variables_map &vm )
    :
    _initialized(false),
    _roi_x(0),
    _roi_y(0),
    _roi_width( vm.count( OPT_WIDTH ) ?
                vm[OPT_WIDTH].as<uint32_t>() :
                throw po::error(
                    std::string( OPT_WIDTH ) + " is required." ) ),
    _roi_height( vm.count( OPT_HEIGHT ) ?
                 vm[OPT_HEIGHT].as<uint32_t>() :
                 throw po::error(
                     std::string( OPT_HEIGHT ) + " is required." ) ),
    _sensor_width(3864),
    _sensor_height(2196),
    _output_width(_sensor_width),
    _output_height(_sensor_height),
    _exposure_time_min( vm[OPT_EXPOSURE_TIME_MIN].as<double>() ),
    _exposure_time_max( vm[OPT_EXPOSURE_TIME_MAX].as<double>() ),
    _gain_min( vm[OPT_GAIN_MIN].as<float>() ),
    _gain_max( vm[OPT_GAIN_MAX].as<float>() ),
    _wb_gains( vm[OPT_WB_GAINS].as<std::vector<float>>() ),
    _framerate( vm[OPT_FRAMERATE].as<double>() ),
    _timeout( vm[OPT_TIMEOUT].as<double>() ),
    _exposure_compensation( vm[OPT_EXPOSURE_COMPENSATION].as<float>() )
{

    bl_log_info("Hello from ArgusCamera");
    
    if ( !AutoWhiteBalanceModeFromString( _awb_mode,
                                          vm[OPT_AWB_MODE].as<std::string>() ) ) {
        throw po::error( vm[OPT_AWB_MODE].as<std::string>() +
                         " is not a valid value for " + std::string( OPT_AWB_MODE ) );
    }

    if ( _wb_gains.size() != 4 ) {
        throw po::error( std::string( OPT_WB_GAINS ) + " requires exactly 4 values." );
    }

    if(!check_bounds())
    {
        ostringstream oss;
        oss << "Bounds checking failed... SensorW " << _sensor_width << " SensorH " << _sensor_height << " roix " << _roi_x << " roiy " << _roi_y
            << " roiwidth " << _roi_width << " roiheight " << _roi_height;
        bl_log_error(oss.str());
        throw runtime_error(oss.str());      
    }
}

bool ArgusCamera::check_bounds()
{
    if((_roi_x + _roi_width <= _sensor_width) && (_roi_y + _roi_height <= _sensor_height))
        return true;
    return false;
}

void ArgusCamera::set_auto_exposure(const bool auto_exp)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        bl_log_error( "Interface cast to IRequest failed." );
        return;
    }
    
    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(
        request->getAutoControlSettings() );
    if ( auto_control_settings == nullptr ) {
        bl_log_error( "Interface cast to ISourceSettings failed." );
        return;
    }
    
    auto_control_settings->setAeLock(auto_exp);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(
        _capture_session_object );
    if ( capture_session == nullptr ) {
        bl_log_error( "Interface cast to ICaptureSession failed." );
//    onError( FrameError::UNKNOWN );
        return;
    }

    capture_session->repeat(_request_object.get());
}

uint64_t ArgusCamera::get_exposure_time()
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        bl_log_error( "Interface cast to IRequest failed." );
        return -1;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(
        request->getSourceSettings() );
    if ( source_settings == nullptr ) {
        bl_log_error( "Interface cast to ISourceSettings failed." );
        return -1;
    }

    Argus::Range < uint64_t > exposure_range;
    exposure_range = source_settings->getExposureTimeRange();
    Argus::Range < uint64_t > frame_range;
    frame_range = source_settings->getFrameDurationRange();
    
    bl_log_info("exp " << exposure_range.min() << " " << exposure_range.max());
    bl_log_info("frame " << frame_range.min() << " " << frame_range.max());
    
    return exposure_range.min();
}

void ArgusCamera::set_exposure_time(const float exp_time)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        bl_log_error( "Interface cast to IRequest failed." );
        return;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(
        request->getSourceSettings() );
    if ( source_settings == nullptr ) {
        bl_log_error( "Interface cast to ISourceSettings failed." );
        return;
    }

    Argus::Range < uint64_t > exposure_range(exp_time, exp_time);
    source_settings->setExposureTimeRange(exposure_range);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(
        _capture_session_object );
    if ( capture_session == nullptr ) {
        bl_log_error( "Interface cast to ICaptureSession failed." );
//    onError( FrameError::UNKNOWN );
        return;
    }

    capture_session->repeat(_request_object.get());
    
}

float ArgusCamera::get_gain()
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        bl_log_error( "Interface cast to IRequest failed." );
        return -1;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(
        request->getSourceSettings() );
    if ( source_settings == nullptr ) {
        bl_log_error( "Interface cast to ISourceSettings failed." );
        return -1;
    }

    Argus::Range < float > gain_range;
    gain_range = source_settings->getGainRange();

    bl_log_info("gain " << gain_range.min() << " " << gain_range.max());

    return gain_range.min();
}

void ArgusCamera::set_gain(const int gain)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        bl_log_error( "Interface cast to IRequest failed." );
        return;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(
        request->getSourceSettings() );
    if ( source_settings == nullptr ) {
        bl_log_error( "Interface cast to ISourceSettings failed." );
        return;
    }

    Argus::Range < float > gain_range((float)gain / 10.0, (float)gain / 10.0);
    source_settings->setGainRange(gain_range);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(
        _capture_session_object );
    if ( capture_session == nullptr ) {
        bl_log_error( "Interface cast to ICaptureSession failed." );
//    onError( FrameError::UNKNOWN );
        return;
    }

    capture_session->repeat(_request_object.get());
}

void ArgusCamera::set_frame_duration(const uint64_t frame_duration)
{
    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        bl_log_error( "Interface cast to IRequest failed." );
        return;
    }
    
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(
        request->getSourceSettings() );
    if ( source_settings == nullptr ) {
        bl_log_error( "Interface cast to ISourceSettings failed." );
        return;
    }

    Argus::Range < uint64_t > frame_dur(frame_duration, frame_duration);
    source_settings->setFrameDurationRange(frame_dur);

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(
        _capture_session_object );
    if ( capture_session == nullptr ) {
        bl_log_error( "Interface cast to ICaptureSession failed." );
//    onError( FrameError::UNKNOWN );
        return;
    }

    capture_session->repeat(_request_object.get());
}

bool ArgusCamera::is_initialized()
{
    return _initialized;
}

bool ArgusCamera::init()
{
    if ( is_initialized() ) {
        return true;
    }

    Argus::Status status;

    // Get camera provider
    _camera_provider_object.reset( Argus::CameraProvider::create( &status ) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to create camera provider. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    auto *camera_provider = Argus::interface_cast<Argus::ICameraProvider>(
        _camera_provider_object );
    if ( camera_provider == nullptr ) {
        bl_log_error( "Interface cast to ICameraProvider failed." );
        return false;
    }

    // Enumerate available devices
    std::vector<Argus::CameraDevice *> devices;
    status = camera_provider->getCameraDevices( &devices );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Could not get available cameras. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }
    if ( devices.empty() ) {
        bl_log_error( "No devices available." );
        return false;
    }

    // Use the first reported device
    // TODO(cooper): Investigate if we need to make this configurable
    Argus::CameraDevice *device = devices[0];

    // Create a capture session
    _capture_session_object.reset( camera_provider->createCaptureSession( device,
                                                                          &status ) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to create a capture session. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(
        _capture_session_object );
    if ( capture_session == nullptr ) {
        bl_log_error( "Interface cast to ICaptureSession failed." );
        return false;
    }

    // Get camera properties
    auto *camera_properties = Argus::interface_cast<Argus::ICameraProperties>(
        device );
    if ( camera_properties == nullptr ) {
        bl_log_error( "Interface cast to ICameraProperties failed." );
        return false;
    }

    // Get sensor modes to use
    std::vector<Argus::SensorMode *> sensor_mode_objects;
    status = camera_properties->getBasicSensorModes( &sensor_mode_objects );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Could not get available sensor modes. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }
    if ( sensor_mode_objects.empty()) {
        bl_log_error( "Camera reports no sensor modes." );
        return false;
    }

    // TODO(#1436): Choose sensor mode based on closest resolution
    _sensor_mode_object = sensor_mode_objects[0];
    auto sensor_mode = Argus::interface_cast<Argus::ISensorMode>(
        _sensor_mode_object );
    if ( sensor_mode == nullptr ) {
        bl_log_error( "Interface cast to ISensorMode failed." );
        return false;
    }

    // Create settings object for the output stream
    Argus::UniqueObj<Argus::OutputStreamSettings> output_stream_settings_object(
        capture_session->createOutputStreamSettings( &status ) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to create output stream settings. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    auto *output_stream_settings = Argus::interface_cast<Argus::IOutputStreamSettings>(
        output_stream_settings_object );
    if ( output_stream_settings == nullptr ) {
        bl_log_error( "Interface cast to IOutputStreamSettings failed." );
        return false;
    }

    // Configure stream settings
    status = output_stream_settings->setPixelFormat( Argus::PIXEL_FMT_YCbCr_420_888 );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't set the pixel format. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    status = output_stream_settings->setResolution( Argus::Size2D<uint32_t>( _roi_width,
                                                                             _roi_height ) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't set the output resolution. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    status = output_stream_settings->setMetadataEnable( true );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't enable metadata output. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    status = output_stream_settings->setMode( Argus::STREAM_MODE_MAILBOX );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't make the stream operate in mailbox mode. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    // Create output stream
    _output_stream_object.reset( capture_session->createOutputStream(
                                     output_stream_settings_object.get(), &status ) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to create output stream. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    auto *output_stream = Argus::interface_cast<Argus::IStream>(
        _output_stream_object );
    if ( output_stream == nullptr ) {
        bl_log_error( "Interface cast to IStream failed." );
        return false;
    }

    // Create frame consumer
    _frame_consumer_object.reset( EGLStream::FrameConsumer::create(
                                      _output_stream_object.get(), 1, &status ) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to create frame consumer. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    auto *frame_consumer = Argus::interface_cast<EGLStream::IFrameConsumer>(
        _frame_consumer_object );
    if ( frame_consumer == nullptr ) {
        bl_log_error( "Interface cast to IFrameConsumer failed." );
        return false;
    }

    // Construct capture request
    _request_object.reset( capture_session->createRequest(
                               Argus::CAPTURE_INTENT_STILL_CAPTURE, &status ) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to create capture request. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    auto *request = Argus::interface_cast<Argus::IRequest>( _request_object );
    if ( request == nullptr ) {
        bl_log_error( "Interface cast to IRequest failed." );
        return false;
    }

    status = request->enableOutputStream( _output_stream_object.get() );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't set the output stream. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    auto stream_settings = Argus::interface_cast < Argus::IStreamSettings > (request->getStreamSettings(_output_stream_object.get()));
    if(stream_settings == nullptr)
    {
        bl_log_error("Interface cast to IStreamSettings failed.");
        return false;
    }

    bl_log_info("Post processing: " << stream_settings->getPostProcessingEnable());

    //Argus::Rectangle < float > rect((float)_roi_x / _sensor_width, (float)_roi_y / _sensor_height, (float)(_roi_x + _roi_width) / _sensor_width, (float)(_roi_y + _roi_height) / _sensor_height);
    Argus::Rectangle < float > rect(0, 0, 1, 1);
    status = stream_settings->setSourceClipRect(rect);
    if(status != Argus::STATUS_OK)
    {
        bl_log_error("Couldn't set the clipping rect, status: " << status);
        return false;
    }
  
    // Set capture settings
    auto source_settings = Argus::interface_cast<Argus::ISourceSettings>(
        request->getSourceSettings() );
    if ( source_settings == nullptr ) {
        bl_log_error( "Interface cast to ISourceSettings failed." );
        return false;
    }

    status = source_settings->setSensorMode( _sensor_mode_object );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't set the sensor mode. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    // Set exposure time range
    Argus::Range<uint64_t> sensor_exposure_time = sensor_mode->getExposureTimeRange();
    uint64_t exposure_time_min = sensor_exposure_time.min();
    uint64_t exposure_time_max = sensor_exposure_time.max();

    if ( _exposure_time_min >= 0 ) {
        double exposure_time_min_ns = _exposure_time_max * pow( 10 , 9 );

        if ( sensor_exposure_time.max() < exposure_time_min_ns ||
             sensor_exposure_time.min() > exposure_time_min_ns ) {
            bl_log_error( "Minimum exposure time value (" << _exposure_time_min << ") is"
                          " outside the range supported by the sensor. Allowed range is ["
                          << sensor_exposure_time.min() * pow( 10 , -9 ) << ", "
                          << sensor_exposure_time.max() * pow( 10 , -9 ) << "].");
            return false;
        }

        exposure_time_min = static_cast<uint64_t>( _exposure_time_min * pow( 10, 9 ) );
    }

    if ( _exposure_time_max >= 0) {
        double exposure_time_max_ns = _exposure_time_max * pow( 10 , 9 );

        if ( sensor_exposure_time.max() < exposure_time_max_ns ||
             sensor_exposure_time.min() > exposure_time_max_ns ) {
            bl_log_error( "Maximum exposure time value (" << _exposure_time_max << ") is"
                          " outside the range supported by the sensor. Allowed range is ["
                          << sensor_exposure_time.min() * pow( 10 , -9 ) << ", "
                          << sensor_exposure_time.max() * pow( 10 , -9 ) << "].");
            return false;
        }

        if ( exposure_time_max_ns < exposure_time_min ) {
            bl_log_error( "Maximum exposure time value (" << _exposure_time_max
                          << ") cannot be less than the minimum value ("
                          << exposure_time_min * pow( 10 , -9 ) << ")." );
            return false;
        }

        exposure_time_max = static_cast<uint64_t>( _exposure_time_max * pow( 10, 9 ) );
    }

    status = source_settings->setExposureTimeRange(
        Argus::Range<uint64_t>( exposure_time_min, exposure_time_max) );
    //Argus::Range<uint64_t>( 20000, 20000) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't set exposure time range. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    // Set gain range
    Argus::Range<float> sensor_gain = sensor_mode->getAnalogGainRange();
    float gain_min = sensor_gain.min();
    float gain_max = sensor_gain.max();

    if ( _gain_min >= 0 ) {
        if ( sensor_gain.max() < _gain_min || sensor_gain.min() > _gain_min ) {
            bl_log_error( "Minimum gain value (" << _gain_min
                          << ") is outside the range supported by the sensor. "
                          << "Allowed range is [" << sensor_gain.min() << ", "
                          << sensor_gain.max() << "]." );
            return false;
        }

        gain_min = _gain_min;
    }

    if ( _gain_max >= 0 ) {
        if ( sensor_gain.max() < _gain_max || sensor_gain.min() > _gain_max ) {
            bl_log_error( "Maximum gain value (" << _gain_max
                          << ") is outside the range supported by the sensor. "
                          << "Allowed range is [" << sensor_gain.min() << ", "
                          << sensor_gain.max() << "].");
            return false;
        }

        if ( _gain_max < gain_min ) {
            bl_log_error( "Maximum gain value (" << _gain_max
                          << ") cannot be less than the minimum value ("
                          << gain_min << ")." );
            return false;
        }

        gain_max = _gain_max;
    }

    status = source_settings->setGainRange( Argus::Range<float>(
                                                gain_min, gain_max) );
    //1, 1) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't set gain range. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    // Set frame duration
    Argus::Range<uint64_t> sensor_frame_duration = sensor_mode->getFrameDurationRange();
    uint64_t frame_duration = sensor_frame_duration.min();

    if ( _framerate > 0 ) {
        double requested_frame_duration = ( 1 / _framerate ) * pow( 10, 9 );

        if ( sensor_frame_duration.max() < requested_frame_duration ||
             sensor_frame_duration.min() > requested_frame_duration ) {
            bl_log_error( "Framerate value (" << _framerate
                          << ") is outside the range supported by the sensor. "
                          << "Allowed range is ["
                          << 1 / ( sensor_frame_duration.max() * pow( 10, -9 ) ) << ", "
                          << 1 / ( sensor_frame_duration.min() * pow( 10, -9 ) ) << "].");
            return false;
        }

        frame_duration = static_cast<uint64_t>(requested_frame_duration);
    }

    status = source_settings->setFrameDurationRange(
        //Argus::Range<uint64_t>( frame_duration, frame_duration )
        //Argus::Range<uint64_t>( 20000, 20000)
        Argus::Range<uint64_t>(33333333, 33333333)
        );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't set frame duration range. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    auto auto_control_settings = Argus::interface_cast<Argus::IAutoControlSettings>(
        request->getAutoControlSettings() );
    if ( auto_control_settings == nullptr ) {
        bl_log_error( "Interface cast to ISourceSettings failed." );
        return false;
    }
    //FIXME
#if 1
    // Set auto white balance mode
    status = auto_control_settings->setAwbMode(
        AutoWhiteBalanceModeToAwbMode( _awb_mode ) );
    //        <Argus::AwbMode> _awb_mode);
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't auto white balance mode. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }
#endif

    // Set white balance gains if white balance mode is manual
    if ( _awb_mode == AutoWhiteBalanceMode::MANUAL ) {
        status = auto_control_settings->setWbGains(
            Argus::BayerTuple<float>( _wb_gains[0],
                                      _wb_gains[1],
                                      _wb_gains[2],
                                      _wb_gains[3] )
            );
        if ( status != Argus::STATUS_OK ) {
            bl_log_error( "Couldn't set white balance gains. Status: ");
//                     << ArgusStatusToString( status ) );
            return false;
        }
    }

    // Set exposure compensation
    status = auto_control_settings->setExposureCompensation( _exposure_compensation );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Couldn't set exposure compensation. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    // Submit capture request on repeat
    status = capture_session->repeat( _request_object.get() );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to submit repeating capture request. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    status = output_stream->waitUntilConnected();
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to connect output stream. Status: ");
//                   << ArgusStatusToString( status ) );
        return false;
    }

    return _initialized = true;
}

ArgusReleaseData *ArgusCamera::requestFrame(
//    pubsub::Callback<void, CameraFrame::Shared> onSuccess,
//    pubsub::Callback<void, FrameError> onError
    )
{
    if ( !is_initialized()) {
        bl_log_error( "Camera is not initialized." );
//    onError( FrameError::NOT_INITIALIZED );
        return NULL;
    }
#if 0
    if ( !isConnected()) {
        bl_log_error( "Camera is not connected." );
//    onError( FrameError::NOT_CONNECTED );
        return NULL;
    }
#endif

    // Obtain capture session
    auto *capture_session = Argus::interface_cast<Argus::ICaptureSession>(
        _capture_session_object );
    if ( capture_session == nullptr ) {
        bl_log_error( "Interface cast to ICaptureSession failed." );
//    onError( FrameError::UNKNOWN );
        return NULL;
    }

    // Obtain frame consumer
    auto *frame_consumer = Argus::interface_cast<EGLStream::IFrameConsumer>(
        _frame_consumer_object );
    if ( frame_consumer == nullptr ) {
        bl_log_error( "Interface cast to IFrameConsumer failed." );
//    onError( FrameError::UNKNOWN );
        return NULL;
    }

    // Acquire frame from frame consumer
    Argus::Status status;
    uint64_t timeout = Argus::TIMEOUT_INFINITE;
    if ( _timeout > 0 ) {
        timeout = static_cast<uint64_t>( _timeout * pow( 10, 9 ) );
    }

    Argus::UniqueObj<EGLStream::Frame> frame_object( frame_consumer->acquireFrame(
                                                         timeout, &status ) );
    if ( status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to acquire frame. Status: ");
//                   << ArgusStatusToString( status ) );
//    onError( ArgusStatusToFrameError( status ) );
        return NULL;
    }

    auto frame = Argus::interface_cast<EGLStream::IFrame>( frame_object );
    if ( frame == nullptr ) {
        bl_log_error( "Interface cast to IFrame failed." );
//    onError( FrameError::UNKNOWN );
        return NULL;
    }

    static uint64_t last_frame_num = 0;
    uint64_t this_frame_num = frame->getNumber();
    if((this_frame_num != last_frame_num + 1) &&
       (last_frame_num != 0))
    {
        bl_log_error("Missed frame! last " << last_frame_num << " this " << this_frame_num);
    }
    //bl_log_info("Got frame " << this_frame_num);
    last_frame_num = this_frame_num;
  
    // Get image from frame
    EGLStream::Image *image_object = frame->getImage();
    auto *image_native_buffer =
        Argus::interface_cast<EGLStream::NV::IImageNativeBuffer>( image_object );
    if ( image_native_buffer == nullptr ) {
        bl_log_error( "Interface cast to IImageNativeBuffer failed." );
//    onError( FrameError::UNKNOWN );
        return NULL;
    }

    Argus::Size2D<uint32_t> image_size( _output_width, _output_height );
    int fd_yuv = image_native_buffer->createNvBuffer( image_size,
                                                      NvBufferColorFormat_YUV420,
                                                      //NvBufferColorFormat_NV12,
                                                      NvBufferLayout_Pitch,
                                                      &status );
    if ( fd_yuv == -1 || status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to create NvBuffer! Status: " << status);
        return NULL;
    }

    int fd_rgb = image_native_buffer->createNvBuffer( image_size,
                                                      NvBufferColorFormat_ARGB32,
                                                      NvBufferLayout_Pitch,
                                                      &status );
    if ( fd_rgb == -1 || status != Argus::STATUS_OK ) {
        bl_log_error( "Failed to create NvBuffer! Status: " << status);
        return NULL;
    }

    // Read image buffer into the final camera frame object
//  CameraFrame::Shared result( std::make_shared<CameraFrame>() );

    NvBufferParams params_yuv;
    NvBufferGetParams( fd_yuv, &params_yuv );
    NvBufferParams params_rgb;
    NvBufferGetParams( fd_rgb, &params_rgb );

    //bl_log_info("height0 " << params_yuv.height[0] << " width0 " << params_yuv.width[0] << " pitch0 " << params_yuv.pitch[0]);
    //bl_log_info("height1 " << params_yuv.height[1] << " width1 " << params_yuv.width[1] << " pitch1 " << params_yuv.pitch[1]);
    //bl_log_info("height2 " << params_yuv.height[2] << " width2 " << params_yuv.width[2] << " pitch2 " << params_yuv.pitch[2]);

    if (params_yuv.num_planes != 3) {
        bl_log_error( "Buffer doesn't have the correct number of planes. (" << params_yuv.num_planes << " != 3)" );
        return NULL;
    }
    if (params_rgb.num_planes != 1) {
        bl_log_error( "Buffer doesn't have the correct number of planes. (" << params_rgb.num_planes << " != 1)" );
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
  
    //cv::cvtColor(plane, cv_frame, cv::COLOR_BGRA2BGR);
    //plane.copyTo( cv_frame );

    // Convert capture time in microseconds since epoch to seconds since epoch
//  result->time = frame->getTime() * pow( 10, -6 );
//  result->encoding = ImageEncoding::BGRA8;
//  onSuccess( result );
    return new ArgusReleaseData(fd_rgb, plane_buffer_rgb, fd_yuv, plane_buffer_y, plane_buffer_u, plane_buffer_v);
}

FramePtr ArgusCamera::grab()
{
    ArgusReleaseData *data = requestFrame();
    return FramePtr(new Frame(cv_frame_rgb, data, argus_release_helper));
}

FramePtr ArgusCamera::grab_y()
{
    //ArgusReleaseData *data = requestFrame();
    return FramePtr(new Frame(cv_frame_y, NULL, argus_release_helper));
}

FramePtr ArgusCamera::grab_u()
{
    //ArgusReleaseData *data = requestFrame();
    return FramePtr(new Frame(cv_frame_u, NULL, argus_release_helper));
}

FramePtr ArgusCamera::grab_v()
{
    //ArgusReleaseData *data = requestFrame();
    return FramePtr(new Frame(cv_frame_v, NULL, argus_release_helper));
}

//}
//}

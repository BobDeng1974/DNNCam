#ifndef __ARGUS_CAMERA__
#define __ARGUS_CAMERA__

#include <boost/program_options.hpp>

#include "Argus/Argus.h"
#include "EGLStream/EGLStream.h"

//#include "amp_camera/Camera.hpp"
#include "include/AutoWhiteBalanceMode.hpp"
#include "opencv2/opencv.hpp"

namespace po = boost::program_options;

//namespace amp {
//namespace camera {

class ArgusCamera
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
  static const AutoWhiteBalanceMode DEFAULT_AWB_MODE;
  static const std::vector<float> DEFAULT_WB_GAINS;
  static const double DEFAULT_FRAMERATE;
  static const double DEFAULT_TIMEOUT;
  static const float DEFAULT_EXPOSURE_COMPENSATION;

  static po::options_description GetOptions();

  ArgusCamera( const uint32_t width, const uint32_t height,
               const double exposure_time_min = DEFAULT_EXPOSURE_TIME_MIN,
               const double exposure_time_max = DEFAULT_EXPOSURE_TIME_MAX,
               const float gain_min = DEFAULT_GAIN_MIN,
               const float gain_max = DEFAULT_GAIN_MAX,
               const AutoWhiteBalanceMode awb_mode = DEFAULT_AWB_MODE,
               const std::vector<float>& wb_gains = DEFAULT_WB_GAINS,
               const double framerate = DEFAULT_FRAMERATE,
               const double timeout = DEFAULT_TIMEOUT,
               const float exposure_compensation = DEFAULT_EXPOSURE_COMPENSATION );
  ArgusCamera( const po::variables_map &vm );
  cv::Mat cv_frame;

  bool connected = false;
  bool initialized = false;

  // Initable implementation
  virtual bool init();

  // Connectable implementation
  virtual bool connect()
  {
    return connected = true;
  }
  virtual bool isInitialized()
  {
    return initialized;
  }

  virtual bool isConnected()
  {
    return isInitialized() && connected;
  }

  virtual void disconnect()
  {
    connected = false;
  }

  // Camera implementation
  virtual void requestFrame(
//      pubsub::Callback<void, CameraFrame::Shared> onSuccess,
//      pubsub::Callback<void, FrameError> onError
  );

 private:
  const uint32_t _width, _height;
  const double _exposure_time_min, _exposure_time_max;
  const float _gain_min, _gain_max;
  AutoWhiteBalanceMode _awb_mode;
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
};

//}
//}

#endif //__ARGUS_CAMERA__

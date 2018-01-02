#ifndef __CAMERA__
#define __CAMERA__

//#include "amp_common/Initable.hpp"
//#include "amp_common/Connectable.hpp"
//#include "amp_common/ptr_utils.hpp"
//#include "amp_logging/Loggable.hpp"
//#include "amp_pubsub/Callback.hpp"

//#include "amp_camera/CameraFrame.hpp"
//#include "amp_camera/FrameError.hpp"

//namespace amp {
//namespace camera {

/**
 * Abstract interface for requesting frames from a video source of some kind.
 */
class Camera : public Initable, public Connectable, public logging::Loggable
{
 public:
//  AMP_PTR_TYPEDEFS(Camera)

  virtual ~Camera()
  {}

  /**
   * Asynchronous call that requests a frame from the video source.
   *
   * @param onSuccess Callback to handle the returned frame
   * @param onError Callback for handling an error
   */
  virtual void requestFrame(
      pubsub::Callback<void, CameraFrame::Shared> onSuccess,
      pubsub::Callback<void, FrameError> onError
  ) = 0;
};

//}
//}

#endif //__CAMERA__

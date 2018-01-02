#ifndef __AMP_CAMERA_FRAME_ERROR__
#define __AMP_CAMERA_FRAME_ERROR__

#include <ostream>
#include <string>

namespace amp {
namespace camera {

enum class FrameError {
  UNKNOWN,         //!< The frame request failed for an unknown reason
  NOT_INITIALIZED, //!< The camera was not initialized before requesting a frame
  NOT_CONNECTED,   //!< The camera was not connected before requesting a frame
  TIMEOUT,         //!< The frame request timed out
  FRAME_PENDING,   //!< The request was rejected because another is pending
  NOT_READY        //!< The camera is not ready for a new frame request
};

//! Returns a string version of the given FrameError.
std::string FrameErrorToString( FrameError error );

inline std::ostream& operator<<(
    std::ostream& os, const FrameError error )
{
  return os << FrameErrorToString( error );
}

}
}

#endif // __AMP_CAMERA_FRAME_ERROR__

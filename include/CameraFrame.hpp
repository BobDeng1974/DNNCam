#ifndef __CAMERA_FRAME__
#define __CAMERA_FRAME__

#include <opencv2/opencv.hpp>

#include "include/ImageEncoding.hpp"
//#include "amp_common/ptr_utils.hpp"

//namespace amp {
//namespace camera {

//! An image with a timestamp and an encoding type.
struct CameraFrame {
//  AMP_PTR_TYPEDEFS(CameraFrame)

  CameraFrame()
   : encoding(ImageEncoding::UNKNOWN),
     time(-1)
  {}

  ImageEncoding encoding;
  cv::Mat image;
  double time;
};

//}
//}

#endif //__CAMERA_FRAME__

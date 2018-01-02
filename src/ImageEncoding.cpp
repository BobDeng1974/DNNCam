#include "include/ImageEncoding.hpp"

#include <boost/algorithm/string.hpp>

//namespace amp {
//namespace camera {

const char* IMAGE_ENCODING_RGB8    = "rgb8";
const char* IMAGE_ENCODING_RGBA8   = "rgba8";
const char* IMAGE_ENCODING_RGB16   = "rgb16";
const char* IMAGE_ENCODING_RGBA16  = "rgba16";
const char* IMAGE_ENCODING_BGR8    = "bgr8";
const char* IMAGE_ENCODING_BGRA8   = "bgra8";
const char* IMAGE_ENCODING_BGR16   = "bgr16";
const char* IMAGE_ENCODING_BGRA16  = "bgra16";
const char* IMAGE_ENCODING_MONO8   = "mono8";
const char* IMAGE_ENCODING_MONO16  = "mono16";
const char* IMAGE_ENCODING_MONOF32 = "32FC1";

std::string ImageEncodingToString( ImageEncoding image_encoding )
{
  switch (image_encoding)
  {
    case ImageEncoding::RGB8:    return IMAGE_ENCODING_RGB8;
    case ImageEncoding::RGBA8:   return IMAGE_ENCODING_RGBA8;
    case ImageEncoding::RGB16:   return IMAGE_ENCODING_RGB16;
    case ImageEncoding::RGBA16:  return IMAGE_ENCODING_RGBA16;
    case ImageEncoding::BGR8:    return IMAGE_ENCODING_BGR8;
    case ImageEncoding::BGRA8:   return IMAGE_ENCODING_BGRA8;
    case ImageEncoding::BGR16:   return IMAGE_ENCODING_BGR16;
    case ImageEncoding::BGRA16:  return IMAGE_ENCODING_BGRA16;
    case ImageEncoding::MONO8:   return IMAGE_ENCODING_MONO8;
    case ImageEncoding::MONO16:  return IMAGE_ENCODING_MONO16;
    case ImageEncoding::MONOF32: return IMAGE_ENCODING_MONOF32;
    default:                     return "UNKNOWN";
  }
}

ImageEncoding StringToImageEncoding( const std::string image_encoding )
{
  if (boost::iequals(image_encoding, IMAGE_ENCODING_RGB8)) {
    return ImageEncoding::RGB8;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_RGBA8)) {
    return ImageEncoding::RGBA8;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_RGB16)) {
    return ImageEncoding::RGB16;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_RGBA16)) {
    return ImageEncoding::RGBA16;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_BGR8)) {
    return ImageEncoding::BGR8;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_BGRA8)) {
    return ImageEncoding::BGRA8;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_BGR16)) {
    return ImageEncoding::BGR16;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_BGRA16)) {
    return ImageEncoding::BGRA16;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_MONO8)) {
    return ImageEncoding::MONO8;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_MONO16)) {
    return ImageEncoding::MONO16;
  }
  else if (boost::iequals(image_encoding, IMAGE_ENCODING_MONOF32)) {
    return ImageEncoding::MONOF32;
  }
  else {
    return ImageEncoding::UNKNOWN;
  }
}

//}
//}

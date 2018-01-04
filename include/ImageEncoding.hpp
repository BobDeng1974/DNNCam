#ifndef __IMAGE_ENCODING__
#define __IMAGE_ENCODING__

#include <ostream>
#include <string>

//namespace amp {
//namespace camera {

extern const char* IMAGE_ENCODING_RGB8;
extern const char* IMAGE_ENCODING_RGBA8;
extern const char* IMAGE_ENCODING_RGB16;
extern const char* IMAGE_ENCODING_RGBA16;
extern const char* IMAGE_ENCODING_BGR8;
extern const char* IMAGE_ENCODING_BGRA8;
extern const char* IMAGE_ENCODING_BGR16;
extern const char* IMAGE_ENCODING_BGRA16;
extern const char* IMAGE_ENCODING_MONO8;
extern const char* IMAGE_ENCODING_MONO16;
extern const char* IMAGE_ENCODING_MONOF32;

//! Encoding formats for CameraFrames.
enum class ImageEncoding
{
  UNKNOWN = 0, //!< Unknown Encoding.
  RGB8,        //!< 8-Bit unsigned int 3-channel (Red-Green-Blue).
  RGBA8,       //!< 8-Bit unsigned int 4-channel (Red-Green-Blue-Alpha).
  RGB16,       //!< 16-Bit unsigned int 3-channel (Red-Green-Blue).
  RGBA16,      //!< 16-Bit unsigned int 4-channel (Red-Green-Blue-Alpha).
  BGR8,        //!< 8-Bit unsigned int 3-channel (Blue-Green-Red).
  BGRA8,       //!< 8-Bit unsigned int 4-channel (Blue-Green-Red-Alpha).
  BGR16,       //!< 16-Bit unsigned int 3-channel (Blue-Green-Red).
  BGRA16,      //!< 16-Bit unsigned int 4-channel (Blue-Green-Red-Alpha).
  MONO8,       //!< 8-Bit unsigned int 1-channel (mono).
  MONO16,      //!< 16-Bit unsigned int 1-channel (mono).
  MONOF32      //!< 32-Bit float 1-channel (mono).
};

//! Returns a string version of the given ImageEncoding.
std::string ImageEncodingToString( ImageEncoding image_encoding );

//! Returns an ImageEncoding from a given string.
ImageEncoding StringToImageEncoding( const std::string image_encoding );

inline std::ostream& operator<<( std::ostream& os,
                                 const ImageEncoding image_encoding )
{
  return os << ImageEncodingToString( image_encoding );
}

//}
//}

#endif

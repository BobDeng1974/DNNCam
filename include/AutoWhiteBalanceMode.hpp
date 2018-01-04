#ifndef __AUTO_WHITE_BALANCE_MODE__
#define __AUTO_WHITE_BALANCE_MODE__

#include <string>
#include <ostream>

//namespace amp {
//namespace camera {

enum class AutoWhiteBalanceMode {
  OFF,
  AUTO,
  INCANDESCENT,
  FLUORESCENT,
  WARM_FLUORESCENT,
  DAYLIGHT,
  CLOUDY_DAYLIGHT,
  TWILIGHT,
  SHADE,
  MANUAL
};

//! Returns a string version of the given AutoWhiteBalanceMode.
std::string AutoWhiteBalanceModeToString( AutoWhiteBalanceMode mode );
Argus::AwbMode AutoWhiteBalanceModeToAwbMode( AutoWhiteBalanceMode mode );

/**
 * Converts a string to a AutoWhiteBalanceMode
 * @param mode_str string to convert
 * @param mode Return value for the mode
 * @return True if valid string, false otherwise
 */
bool AutoWhiteBalanceModeFromString( AutoWhiteBalanceMode& mode,
                                     const std::string& mode_str );

inline std::ostream& operator<<( std::ostream& os,
                                 const AutoWhiteBalanceMode mode )
{
  return os << AutoWhiteBalanceModeToString( mode );
}

//}
//}

#endif //__AUTO_WHITE_BALANCE_MODE__

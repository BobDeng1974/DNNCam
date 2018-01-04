
#include "Argus/Argus.h"

#include "AutoWhiteBalanceMode.hpp"

#include <boost/algorithm/string.hpp>

//namespace amp {
//namespace camera {

static const char* AWB_MODE_OFF = "OFF";
static const char* AWB_MODE_AUTO = "AUTO";
static const char* AWB_MODE_INCANDESCENT = "INCANDESCENT";
static const char* AWB_MODE_FLUORESCENT = "FLUORESCENT";
static const char* AWB_MODE_WARM_FLUORESCENT = "WARM_FLUORESCENT";
static const char* AWB_MODE_DAYLIGHT = "DAYLIGHT";
static const char* AWB_MODE_CLOUDY_DAYLIGHT = "CLOUDY_DAYLIGHT";
static const char* AWB_MODE_TWILIGHT = "TWILIGHT";
static const char* AWB_MODE_SHADE = "SHADE";
static const char* AWB_MODE_MANUAL = "MANUAL";

std::string AutoWhiteBalanceModeToString( AutoWhiteBalanceMode mode )
{
  switch ( mode ) {
    case AutoWhiteBalanceMode::OFF: return AWB_MODE_OFF;
    case AutoWhiteBalanceMode::AUTO: return AWB_MODE_AUTO;
    case AutoWhiteBalanceMode::INCANDESCENT: return AWB_MODE_INCANDESCENT;
    case AutoWhiteBalanceMode::FLUORESCENT: return AWB_MODE_FLUORESCENT;
    case AutoWhiteBalanceMode::WARM_FLUORESCENT: return AWB_MODE_WARM_FLUORESCENT;
    case AutoWhiteBalanceMode::DAYLIGHT: return AWB_MODE_DAYLIGHT;
    case AutoWhiteBalanceMode::CLOUDY_DAYLIGHT: return AWB_MODE_CLOUDY_DAYLIGHT;
    case AutoWhiteBalanceMode::TWILIGHT: return AWB_MODE_TWILIGHT;
    case AutoWhiteBalanceMode::SHADE: return AWB_MODE_SHADE;
    case AutoWhiteBalanceMode::MANUAL: return AWB_MODE_MANUAL;
  }
}

Argus::AwbMode AutoWhiteBalanceModeToAwbMode( AutoWhiteBalanceMode mode )
{
  switch ( mode ) {
	  case AutoWhiteBalanceMode::OFF: return Argus::AWB_MODE_OFF;
	  case AutoWhiteBalanceMode::AUTO: return Argus::AWB_MODE_AUTO;
	  case AutoWhiteBalanceMode::INCANDESCENT: return Argus::AWB_MODE_INCANDESCENT;
	  case AutoWhiteBalanceMode::FLUORESCENT: return Argus::AWB_MODE_FLUORESCENT;
	  case AutoWhiteBalanceMode::WARM_FLUORESCENT: return Argus::AWB_MODE_WARM_FLUORESCENT;
	  case AutoWhiteBalanceMode::DAYLIGHT: return Argus::AWB_MODE_DAYLIGHT;
	  case AutoWhiteBalanceMode::CLOUDY_DAYLIGHT: return Argus::AWB_MODE_CLOUDY_DAYLIGHT;
	  case AutoWhiteBalanceMode::TWILIGHT: return Argus::AWB_MODE_TWILIGHT;
	  case AutoWhiteBalanceMode::SHADE: return Argus::AWB_MODE_SHADE;
	  case AutoWhiteBalanceMode::MANUAL: return Argus::AWB_MODE_MANUAL;
  }
}

bool AutoWhiteBalanceModeFromString( AutoWhiteBalanceMode& mode,
                                     const std::string& mode_str )
{
  if ( boost::iequals( mode_str, AWB_MODE_OFF ) ) {
    mode = AutoWhiteBalanceMode::OFF;
  } else if ( boost::iequals( mode_str, AWB_MODE_AUTO ) ) {
    mode = AutoWhiteBalanceMode::AUTO;
  } else if ( boost::iequals( mode_str, AWB_MODE_INCANDESCENT ) ) {
    mode = AutoWhiteBalanceMode::INCANDESCENT;
  } else if ( boost::iequals( mode_str, AWB_MODE_FLUORESCENT ) ) {
    mode = AutoWhiteBalanceMode::FLUORESCENT;
  } else if ( boost::iequals( mode_str, AWB_MODE_WARM_FLUORESCENT ) ) {
    mode = AutoWhiteBalanceMode::WARM_FLUORESCENT;
  } else if ( boost::iequals( mode_str, AWB_MODE_DAYLIGHT ) ) {
    mode = AutoWhiteBalanceMode::DAYLIGHT;
  } else if ( boost::iequals( mode_str, AWB_MODE_CLOUDY_DAYLIGHT ) ) {
    mode = AutoWhiteBalanceMode::CLOUDY_DAYLIGHT;
  } else if ( boost::iequals( mode_str, AWB_MODE_TWILIGHT ) ) {
    mode = AutoWhiteBalanceMode::INCANDESCENT;
  } else if ( boost::iequals( mode_str, AWB_MODE_SHADE ) ) {
    mode = AutoWhiteBalanceMode::SHADE;
  } else if ( boost::iequals( mode_str, AWB_MODE_MANUAL ) ) {
    mode = AutoWhiteBalanceMode::MANUAL;
  } else {
    return false;
  }

  return true;
}

//}
//}

#pragma once

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

#include "motordriver.hpp"
#include "DNNCam.hpp"
#include "configuration.hpp"

#include "log.hpp"

namespace BoulderAI
{

class FocusHome : public xmlrpc_c::method {
public:
    FocusHome(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:";
        this->_help = "Sets the focus to the home location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: FocusHome");
        if (false == _dnncam->focus_home()) {
            ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class FocusAbsolute : public xmlrpc_c::method {
public:
    FocusAbsolute(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:i";
        this->_help = "Sets focus to an absolute value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: FocusAbsolute");
        if (false == _dnncam->focus_absolute(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class FocusRelative : public xmlrpc_c::method {
public:
    FocusRelative(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:i";
        this->_help = "Sets focus to a relative value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: FocusRelative");
        if (false == _dnncam->focus_relative(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class FocusGetLocation : public xmlrpc_c::method {
public:
    FocusGetLocation(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:";
        this->_help = "Gets the focus location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: FocusGetLocation");
        ret = _dnncam->get_focus_location();
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class ZoomHome : public xmlrpc_c::method {
public:
    ZoomHome(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:";
        this->_help = "Sets the zoom to the home location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: ZoomHome");
        if (false == _dnncam->zoom_home()) {
            ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class ZoomAbsolute : public xmlrpc_c::method {
public:
    ZoomAbsolute(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:i";
        this->_help = "Sets zoom to an absolute value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: ZoomAbsolute");
        if (false == _dnncam->zoom_absolute(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class ZoomRelative : public xmlrpc_c::method {
public:
    ZoomRelative(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:i";
        this->_help = "Sets zoom to a relative value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: ZoomRelative");
        if (false == _dnncam->zoom_relative(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class ZoomGetLocation : public xmlrpc_c::method {
public:
    ZoomGetLocation(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:";
        this->_help = "Gets the zoom location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: ZoomGetLocation");
        ret = _dnncam->get_zoom_location();
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class IrisHome : public xmlrpc_c::method {
public:
    IrisHome(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:";
        this->_help = "Sets the iris to the home location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: IrisHome");
        if (false == _dnncam->iris_home()) {
            ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class IrisAbsolute : public xmlrpc_c::method {
public:
    IrisAbsolute(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:i";
        this->_help = "Sets the iris to an absolute value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: IrisAbsolute");
        if (false == _dnncam->iris_absolute(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class IrisRelative : public xmlrpc_c::method {
public:
    IrisRelative(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:i";
        this->_help = "Sets the iris to a relative value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: IrisRelative");
        if (false == _dnncam->iris_relative(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class IrisGetLocation : public xmlrpc_c::method {
public:
    IrisGetLocation(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:";
        this->_help = "Gets the iris location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: IrisGetLocation");
        ret = _dnncam->get_iris_location();
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class IRCut : public xmlrpc_c::method {
public:
    IRCut(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "i:b";
        this->_help = "Sets the IR cut filter.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const bool value(paramList.getBoolean(0));
        bl_log_info("XMLRPC: IRCut");
        ret = _dnncam->set_ir_cut(value);
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class SetAutoExposure : public xmlrpc_c::method {
public:
    SetAutoExposure(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "n:b";
        this->_help = "Sets auto exposure.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        const bool value(paramList.getBoolean(0));
        bl_log_info("XMLRPC: SetAutoExposure");
        _dnncam->set_auto_exposure(value);
    }

protected:
    DNNCamPtr _dnncam;
};

class GetExposureTime : public xmlrpc_c::method {
public:
    GetExposureTime(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "A:";
        this->_help = "Gets the exposure time range.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        bl_log_info("XMLRPC: GetExposureTime");
        Argus::Range < uint64_t > ret;
        ret = _dnncam->get_exposure_time();
        xmlrpc_c::carray ret_array;
        ret_array.push_back(xmlrpc_c::value_i8(ret.min()));
        ret_array.push_back(xmlrpc_c::value_i8(ret.max()));
        *retvalP = xmlrpc_c::value_array(ret_array);
    }

protected:
    DNNCamPtr _dnncam;
};

class SetExposureTime : public xmlrpc_c::method {
public:
    SetExposureTime(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "n:i8i8";
        this->_help = "Sets exposure time range.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        const uint64_t value_min(paramList.getI8(0));
        const uint64_t value_max(paramList.getI8(1));
        bl_log_info("XMLRPC: SetExposureTime");
        Argus::Range < uint64_t > param(value_min, value_max);
        _dnncam->set_exposure_time(param);
    }

protected:
    DNNCamPtr _dnncam;
};

class GetExposureCompensation : public xmlrpc_c::method {
public:
    GetExposureCompensation(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "d:";
        this->_help = "Gets the iris location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        bl_log_info("XMLRPC: GetExposureCompensation");
        const float ret = _dnncam->get_exposure_compensation();
        *retvalP = xmlrpc_c::value_double(ret);
    }

protected:
    DNNCamPtr _dnncam;
};

class SetExposureCompensation : public xmlrpc_c::method {
public:
    SetExposureCompensation(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "n:d";
        this->_help = "Sets exposure compensation.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        const bool param(paramList.getDouble(0));
        bl_log_info("XMLRPC: SetExposureCompensation");
        _dnncam->set_exposure_compensation(param);
    }

protected:
    DNNCamPtr _dnncam;
};

class GetFrameDuration : public xmlrpc_c::method {
public:
    GetFrameDuration(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "A:";
        this->_help = "Gets frame duration range.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        bl_log_info("XMLRPC: GetFrameDuration");
        Argus::Range < uint64_t > ret;
        ret = _dnncam->get_frame_duration();
        xmlrpc_c::carray ret_array;
        ret_array.push_back(xmlrpc_c::value_i8(ret.min()));
        ret_array.push_back(xmlrpc_c::value_i8(ret.max()));
        *retvalP = xmlrpc_c::value_array(ret_array);
    }

protected:
    DNNCamPtr _dnncam;
};

class SetFrameDuration : public xmlrpc_c::method {
public:
    SetFrameDuration(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "n:i8i8";
        this->_help = "Sets frame druation range.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        const uint64_t value_min(paramList.getI8(0));
        const uint64_t value_max(paramList.getI8(1));
        bl_log_info("XMLRPC: SetFrameDuration");
        Argus::Range < uint64_t > param(value_min, value_max);
        _dnncam->set_frame_duration(param);
    }

protected:
    DNNCamPtr _dnncam;
};

class GetGain : public xmlrpc_c::method {
public:
    GetGain(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "A:";
        this->_help = "Gets gain range.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        bl_log_info("XMLRPC: GetGain");
        Argus::Range < float > ret;
        ret = _dnncam->get_gain();
        xmlrpc_c::carray ret_array;
        ret_array.push_back(xmlrpc_c::value_double(ret.min()));
        ret_array.push_back(xmlrpc_c::value_double(ret.max()));
        *retvalP = xmlrpc_c::value_array(ret_array);
    }

protected:
    DNNCamPtr _dnncam;
};

class SetGain : public xmlrpc_c::method {
public:
    SetGain(DNNCamPtr dnncam) : _dnncam(dnncam)
    {
        this->_signature = "n:dd";
        this->_help = "Sets gain range.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        const float value_min(paramList.getI8(0));
        const float value_max(paramList.getI8(1));
        bl_log_info("XMLRPC: SetGain");
        Argus::Range < float > param(value_min, value_max);
        _dnncam->set_gain(param);
    }

protected:
    DNNCamPtr _dnncam;
};
    
class DNNCamServer
{
public:
    /**
     * @param md Smart pointer to the motordriver.
     */
    DNNCamServer(DNNCamPtr dnncam) :
        _done(false)
    {
        // lens methods
        xmlrpc_c::methodPtr const focusHome(new FocusHome(dnncam));
        _registry.addMethod("focus_home", focusHome);

        xmlrpc_c::methodPtr const focusAbsolute(new FocusAbsolute(dnncam));
        _registry.addMethod("focus_absolute", focusAbsolute);

        xmlrpc_c::methodPtr const focusRelative(new FocusRelative(dnncam));
        _registry.addMethod("focus_relative", focusRelative);

        xmlrpc_c::methodPtr const focusGetLocation(new FocusGetLocation(dnncam));
        _registry.addMethod("focus_get_location", focusGetLocation);

        xmlrpc_c::methodPtr const zoomHome(new ZoomHome(dnncam));
        _registry.addMethod("zoom_home", zoomHome);

        xmlrpc_c::methodPtr const zoomAbsolute(new ZoomAbsolute(dnncam));
        _registry.addMethod("zoom_absolute", zoomAbsolute);

        xmlrpc_c::methodPtr const zoomRelative(new ZoomRelative(dnncam));
        _registry.addMethod("zoom_relative", zoomRelative);

        xmlrpc_c::methodPtr const zoomGetLocation(new ZoomGetLocation(dnncam));
        _registry.addMethod("zoom_get_location", zoomGetLocation);

        xmlrpc_c::methodPtr const irisHome(new IrisHome(dnncam));
        _registry.addMethod("iris_home", irisHome);

        xmlrpc_c::methodPtr const irisAbsolute(new IrisAbsolute(dnncam));
        _registry.addMethod("iris_absolute", irisAbsolute);

        xmlrpc_c::methodPtr const irisRelative(new IrisRelative(dnncam));
        _registry.addMethod("iris_relative", irisRelative);

        xmlrpc_c::methodPtr const irisGetLocation(new IrisGetLocation(dnncam));
        _registry.addMethod("iris_get_location", irisGetLocation);

        xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("ir_cut", irCut);

        // camera methods
        xmlrpc_c::methodPtr const setAutoExposure(new SetAutoExposure(dnncam));
        _registry.addMethod("set_auto_exposure", setAutoExposure);
        
        xmlrpc_c::methodPtr const getExposureTime(new GetExposureTime(dnncam));
        _registry.addMethod("get_exposure_time", getExposureTime);
        
        xmlrpc_c::methodPtr const setExposureTime(new SetExposureTime(dnncam));
        _registry.addMethod("set_exposure_time", setExposureTime);
        
        xmlrpc_c::methodPtr const setExposureCompensation(new SetExposureCompensation(dnncam));
        _registry.addMethod("set_exposure_compensation", setExposureCompensation);
        
        xmlrpc_c::methodPtr const getExposureCompensation(new GetExposureCompensation(dnncam));
        _registry.addMethod("get_exposure_compensation", getExposureCompensation);
        
        xmlrpc_c::methodPtr const setFrameDuration(new SetFrameDuration(dnncam));
        _registry.addMethod("set_frame_duration", setFrameDuration);
        
        xmlrpc_c::methodPtr const getFrameDuration(new GetFrameDuration(dnncam));
        _registry.addMethod("get_frame_duration", getFrameDuration);
        
        xmlrpc_c::methodPtr const setGain(new SetGain(dnncam));
        _registry.addMethod("set_gain", setGain);
        
        xmlrpc_c::methodPtr const getGain(new GetGain(dnncam));
        _registry.addMethod("get_gain", getGain);
        
        /*xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("set_awb_mode", setAWBMode);
        
        xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("get_awb_mode", getAWBMode);
        
        xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("set_awb", setAWB);
        
        xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("set_awb_gains", setAWBGains);
        
        xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("set_denoise_mode", setDenoiseMode);
        
        xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("get_denoise_mode", getDenoiseMode);
        
        xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("set_denoise_strength", setDenoiseStrength);
        
        xmlrpc_c::methodPtr const irCut(new IRCut(dnncam));
        _registry.addMethod("get_denoise_strength", getDenoiseStrength);*/

        _server = new xmlrpc_c::serverAbyss(xmlrpc_c::serverAbyss::constrOpt()
                        .registryP(&_registry)
                        .portNumber(7000)
                        .logFileName("/tmp/xmlrpc_log"));

    }

    virtual ~DNNCamServer()
    {
        stop();
        delete _server;
    }

    void run()
    {
        std::cout << "HELLO FROM SERVER" << std::endl;
        while (!_done)
        {
            // Loop over runOnce to run serially. Note that this is a blocking call;
            // it only returns once a request arrives or if _server->terminate() is called.
            _server->runOnce();
        }
    }

    void stop()
    {
        // First, mark the flag so that the while-loop in run() will terminate.
        _done = true;

        // Now tell the server to exit immediately, which will cause the server's
        // runOnce() call (a blocking method) to return, which will allow our thread
        // to exit and the program to exit cleanly.
        _server->terminate();
    }

protected:
    xmlrpc_c::registry _registry;
    xmlrpc_c::serverAbyss* _server;
    bool _done;

};

typedef std::shared_ptr<DNNCamServer> DNNCamServerPtr;

}

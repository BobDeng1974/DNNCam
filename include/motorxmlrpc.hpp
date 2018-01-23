#pragma once

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include "motordriver.hpp"
#include "log.hpp"
#include "configuration.hpp"

namespace BoulderAI
{

class FocusHome : public xmlrpc_c::method {
public:
    FocusHome(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method homes the focus.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: FocusHome");
        if (false == _md->focusHome()) {
            ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class FocusAbsolute : public xmlrpc_c::method {
public:
    FocusAbsolute(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method sets focus to an absolute value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: FocusAbsolute");
        if (false == _md->focusAbsolute(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class FocusRelative : public xmlrpc_c::method {
public:
    FocusRelative(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method sets focus to a relative value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: FocusRelative");
        if (false == _md->focusRelative(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class FocusGetLocation : public xmlrpc_c::method {
public:
    FocusGetLocation(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method gets the focus location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: FocusGetLocation");
        ret = _md->focusAbsoluteLocation();
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class ZoomHome : public xmlrpc_c::method {
public:
    ZoomHome(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method homes the zoom.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: ZoomHome");
        if (false == _md->zoomHome()) {
            ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class ZoomAbsolute : public xmlrpc_c::method {
public:
    ZoomAbsolute(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method sets zoom to an absolute value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: ZoomAbsolute");
        if (false == _md->zoomAbsolute(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class ZoomRelative : public xmlrpc_c::method {
public:
    ZoomRelative(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method sets zoom to a relative value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: ZoomRelative");
        if (false == _md->zoomRelative(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class ZoomGetLocation : public xmlrpc_c::method {
public:
    ZoomGetLocation(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method gets the zoom location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: ZoomGetLocation");
        ret = _md->zoomAbsoluteLocation();
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class IrisHome : public xmlrpc_c::method {
public:
    IrisHome(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method homes the iris.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: IrisHome");
        if (false == _md->irisHome()) {
            ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class IrisAbsolute : public xmlrpc_c::method {
public:
    IrisAbsolute(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method sets zoom to an absolute value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: IrisAbsolute");
        if (false == _md->irisAbsolute(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class IrisRelative : public xmlrpc_c::method {
public:
    IrisRelative(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method sets iris to a relative value.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        const int value(paramList.getInt(0));
        bl_log_info("XMLRPC: IrisRelative");
        if (false == _md->irisRelative(value)) {
           ret = -1;
        }
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class IrisGetLocation : public xmlrpc_c::method {
public:
    IrisGetLocation(MotorDriverPtr md) : _md(md)
    {
        this->_signature = "i:";
        this->_help = "This method gets the iris location.";
    }

    void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP)
    {
        int ret = 0;
        bl_log_info("XMLRPC: IrisGetLocation");
        ret = _md->irisAbsoluteLocation();
        *retvalP = xmlrpc_c::value_int(ret);
    }

protected:
    MotorDriverPtr _md;
};

class XMLRPCServer
{
public:
    /**
     * @param md Smart pointer to the motordriver.
     */
    XMLRPCServer(MotorDriverPtr md) :
        _done(false)
    {
        xmlrpc_c::methodPtr const focusHome(new FocusHome(md));
        _registry.addMethod("focus_home", focusHome);

        xmlrpc_c::methodPtr const focusAbsolute(new FocusAbsolute(md));
        _registry.addMethod("focus_absolute", focusAbsolute);

        xmlrpc_c::methodPtr const focusRelative(new FocusRelative(md));
        _registry.addMethod("focus_relative", focusRelative);

        xmlrpc_c::methodPtr const focusGetLocation(new FocusGetLocation(md));
        _registry.addMethod("focus_get_location", focusGetLocation);

        xmlrpc_c::methodPtr const zoomHome(new ZoomHome(md));
        _registry.addMethod("zoom_home", zoomHome);

        xmlrpc_c::methodPtr const zoomAbsolute(new ZoomAbsolute(md));
        _registry.addMethod("zoom_absolute", zoomAbsolute);

        xmlrpc_c::methodPtr const zoomRelative(new ZoomRelative(md));
        _registry.addMethod("zoom_relative", zoomRelative);

        xmlrpc_c::methodPtr const zoomGetLocation(new ZoomGetLocation(md));
        _registry.addMethod("zoom_get_location", zoomGetLocation);

        xmlrpc_c::methodPtr const irisHome(new IrisHome(md));
        _registry.addMethod("iris_home", irisHome);

        xmlrpc_c::methodPtr const irisAbsolute(new IrisAbsolute(md));
        _registry.addMethod("iris_absolute", irisAbsolute);

        xmlrpc_c::methodPtr const irisRelative(new IrisRelative(md));
        _registry.addMethod("iris_relative", irisRelative);

        xmlrpc_c::methodPtr const irisGetLocation(new IrisGetLocation(md));
        _registry.addMethod("iris_get_location", irisGetLocation);

        _server = new xmlrpc_c::serverAbyss(xmlrpc_c::serverAbyss::constrOpt()
                        .registryP(&_registry)
                        .portNumber(7000)
                        .logFileName("/tmp/xmlrpc_log"));

    }

    virtual ~XMLRPCServer()
    {
        stop();
        delete _server;
    }

    void run()
    {
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

typedef std::shared_ptr<XMLRPCServer> XMLRPCServerPtr;

}

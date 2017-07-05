#pragma once

#include "camera.hpp"
#include "datasource.hpp"
#include <string>
#include <vector>

class DataSourceCamera : public DataSourceBase
{
public:
    DataSourceCamera(CameraPtr camera,
                     const bool verbose,
                     const FrameProcessorPtr frame_proc);
    virtual ~DataSourceCamera();

protected:
    virtual bool ProcessImpl(void);
private:
    CameraPtr _camera;
};


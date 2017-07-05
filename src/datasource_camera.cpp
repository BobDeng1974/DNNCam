#include <stdexcept>  // std::runtime_error

#include "datasource_camera.hpp"
#include "frame_processor.hpp"

DataSourceCamera::DataSourceCamera(CameraPtr camera,
                                   const bool verbose,
                                   const FrameProcessorPtr frame_proc)
    :
    DataSourceBase(verbose, frame_proc, 0, -1, camera->usesCallback()),
    _camera(camera)
{
    if (!_camera)
    {
        throw std::runtime_error("Camera pointer is empty!");
    }
}

DataSourceCamera::~DataSourceCamera()
{

}


bool DataSourceCamera::ProcessImpl(void)
{
    FramePtr frame = _camera->grab();  // This is a blocking call.
    _frame_proc->process_frame(frame);
    return true;
}

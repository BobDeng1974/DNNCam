
#include "datasource.hpp"
#include "frame_processor.hpp"
#include "frame.hpp"

DataSourceBase::DataSourceBase(const bool verbose, const FrameProcessorPtr frame_proc,
                               const int start_frame, const int end_frame, const bool usesCallback)
    :
    _frame_proc(frame_proc),
    _done(false),
    _frame_num(start_frame),
    _start_frame(start_frame),
    _end_frame(end_frame),
    _verbose(verbose),
    _usesCallback(usesCallback)
{
}

DataSourceBase::~DataSourceBase()
{
}

bool DataSourceBase::Process(void)
{
    while(!_done)
    {
        if(_verbose)
           std::cout << "Processing frame: " << _frame_num << std::endl;

        if(_usesCallback == false) {
            if(ProcessImpl() == false) {
                std::cout << "Error processing frame: " << _frame_num << std::endl;
            }
        } else {
            sleep(1);
        }
        
        _frame_num++;
    }
	return true;
}

size_t DataSourceBase::GetFrameNum(void)
{
    return _frame_num;
}

int DataSourceBase::GetStartFrame(void)
{
    return _start_frame;
}

int DataSourceBase::GetEndFrame(void)
{
    return _end_frame;
}

bool DataSourceBase::usesCallback()
{
    return _usesCallback;
}

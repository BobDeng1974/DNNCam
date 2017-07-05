
#ifndef DATASOURCE_HPP
#define DATASOURCE_HPP

#include "frame_processor.hpp"

class DataSourceBase
{
public:
    DataSourceBase(const bool verbose, const FrameProcessorPtr frame_proc,
                   const int start_frame = 0, const int end_frame = -1, const bool usesCallback = false);
    virtual ~DataSourceBase();
    
    bool Process(void);
    size_t GetFrameNum(void);
    int GetStartFrame(void);
    int GetEndFrame(void);
    bool usesCallback();

protected:
    virtual bool ProcessImpl(void) = 0;

    FrameProcessorPtr _frame_proc;
    bool _done;
    size_t _frame_num;
    int _start_frame;
    int _end_frame;
    bool _verbose;
    bool _usesCallback;
};

typedef boost::shared_ptr < DataSourceBase > DataSourceBasePtr;

#endif

#include <boost/bind.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "frame_processor.hpp"
#include "log.hpp"

#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>

namespace pt=boost::posix_time;

static const size_t PREBUFFERED_FRAMES = 20; // number of frames to 'prebuffer' -- the prebuffer is
                                             // a small queue of frames that we can go back to after
                                             // we notice a new object for tracking, and dynamically turn
                                             // tracking on. this allows us to get a few frames before
                                             // the tracker notices an object, in case a detail was missed.
static const size_t PREBUFFER_POST_FRAMES = 20;

std::map<std::string, boost::shared_ptr<boost::mutex> > SequentialSection::_mutex;
std::map<std::string, boost::shared_ptr<boost::condition_variable> > SequentialSection::_condition;
std::map<std::string, int> SequentialSection::_frame_num;

FrameProcessor::FrameProcessor(const int w, const int h)
    :
    _stream_state(StreamState::OFF),
    _created_window(false),
    _frame_width(w),
    _frame_height(h),
    _frame_num(0),
    _previous_frame_num(-1),
    _queue_size(0),
    _dropped_frames(0),
    _prebuffer_post_frames(0),
    _worker(3, 512, "Frame Processor Worker"),
    _gui_worker(1, "GUI Worker")
{
    // Set up the masking block.
    /* Initialize sequential sections for later use */
    {
        SequentialSection ss1("stream");
    }

    _streamer.reset(new Stream(_frame_width, _frame_height));
    _streamer->start();
}

FrameProcessor::~FrameProcessor()
{
    _worker.wait();
    _gui_worker.wait();
}

// Creates the ObjectFinder and starts the workers.
// Call this once, after constructing frame processor, but before calling DataSource::Process()
// It's stuff we weren't able to do in the constructor (namely calling shared_from_this()).
void FrameProcessor::start_workers(void)
{
    _worker.start();
    _gui_worker.start();
}

void FrameProcessor::wait_for_queued_images()
{
    _worker.wait();
}

void FrameProcessor::set_stream_state(const std::string &state)
{
}


void FrameProcessor::set_stream_state(const StreamState state)
{
}

std::string FrameProcessor::get_stream_state()
{
}

void FrameProcessor::process_frame(FramePtr frame, const bool block)
{
    /* When processing captures / movies, block rather than filling the queue as fast
     * as io will allow, so we don't get overruns */
    while (block && get_queue_size() > 100)
    {
        usleep(50000);
    }

    if (!frame.get())
    {
        ScopedLock lock(_mtex);
        increment_dropped_frames();
        _frame_num++;
        return;
    }

    //cv::Mat m = frame->to_mat();
    //bl_log_info("In process frame: " << m.cols << "x" << m.rows);
    
    {
        ScopedLock lock(_prebuffer_mtex);
        if (_prebuffer.size() == PREBUFFERED_FRAMES)  // Buffer size limit
        {
            _prebuffer.pop_back();
        }
        /* store a copy in the prebuffer, for when we turn on capture */
        _prebuffer.push_front(frame);
    }

    {
        ScopedLock lock(_mtex);
        if ( (_queue_size = _worker.add_job( boost::bind(&FrameProcessor::process_frame_task, this, frame, _frame_num, _frame_num-_previous_frame_num-1)) ) == -1 )
        {
            std::cout << "The _worker queue is full!" << std::endl;
            increment_dropped_frames();
        }
        else
        {
            _previous_frame_num=_frame_num;
        }
        _frame_num++;
    }
}

std::string FrameProcessor::get_timing_string(void)
{
    std::ostringstream ret;
    return ret.str();
}

void FrameProcessor::do_stream(cv::Mat frame, const int frame_num, const int n_dropped_before)
{
    /* TODO: Potential optimization - don't for sequential section here if we're not going
     * to stream the frame. Requires some logic for figuring out dropped frames */

    cv::Mat scaled_frame; 
    //if (frame_num % 2 == 0)
    {
        cv::Mat stream_frame; 
        stream_frame = frame;

        if (frame.depth() == CV_16U)
        {
            stream_frame.convertTo(scaled_frame, CV_8U, 1/16.0);
        }
        else
        {
            scaled_frame = stream_frame;
        }
    }

    if (!SequentialSection::too_late("stream", frame_num, n_dropped_before))
    {
        SequentialSection ss("stream", frame_num, n_dropped_before);
        //if (frame_num % 2 == 0)
        {
           _streamer->push_frame(frame);
        }
    }
}

void FrameProcessor::process_frame_task(FramePtr src, const int frame_num, const int n_dropped_before)
{
    const int queue_size = get_queue_size();
    const bool skip_processing = (queue_size > 350) && frame_num % 2 == 1;
    if (skip_processing)
    {
        std::cout << "Skipping frame " << frame_num << " because frame queue contains " << queue_size << " frames.";
    }

    cv::Mat source_frame; 
    source_frame = src->to_mat();
    do_stream(source_frame, frame_num, n_dropped_before);
}

void touch(const std::string& pathname)
{
    int fd = open(pathname.c_str(), O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK, 0666);
    if (fd<0) // Couldn't open that path.
    {
        return;
    }
    int rc = utimensat(AT_FDCWD, pathname.c_str(), nullptr, 0);
    if (rc)
    {
        return;
    }
}

int FrameProcessor::get_queue_size(void)
{
    return _worker.getNumJobsRunning();
}

int FrameProcessor::get_dropped_frames(void)
{
    return _dropped_frames;
}

void FrameProcessor::increment_dropped_frames(void)
{
    _dropped_frames++;
}

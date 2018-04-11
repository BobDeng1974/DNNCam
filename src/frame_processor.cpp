#include <boost/bind.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "frame_processor.hpp"
#include "configuration.hpp"

#include <iostream>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <wrnch/returnCodes.h>

namespace pt=boost::posix_time;

namespace BoulderAI
{

static const size_t PREBUFFERED_FRAMES = 20; // number of frames to 'prebuffer' -- the prebuffer is
                                             // a small queue of frames that we can go back to after
                                             // we notice a new object for tracking, and dynamically turn
                                             // tracking on. this allows us to get a few frames before
                                             // the tracker notices an object, in case a detail was missed.
static const size_t PREBUFFER_POST_FRAMES = 20;

static const int NUM_COLORS = 6;

std::map<std::string, boost::shared_ptr<boost::mutex> > SequentialSection::_mutex;
std::map<std::string, boost::shared_ptr<boost::condition_variable> > SequentialSection::_condition;
std::map<std::string, int> SequentialSection::_frame_num;

FrameProcessor::FrameProcessor(boost::shared_ptr<wrnch::PoseEstimator> pe, boost::shared_ptr<wrnch::PoseEstimatorOptions> po, const int w, const int h)
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
    _worker(3, 1, "Frame Processor Worker"),
    _gui_worker(1, "GUI Worker"),
    poseEstimator(pe),
    poseOptions(po)
{
    // Set up the masking block.
    /* Initialize sequential sections for later use */
    {
        SequentialSection ss1("stream");
    }
    jointsDefinition = poseEstimator->GetHumanOutputFormat();
    bonePairs.resize(jointsDefinition.GetNumBones() * 2);
    jointsDefinition.GetBonePairs(bonePairs.data());

    y_color.resize(NUM_COLORS);
    u_color.resize(NUM_COLORS);
    v_color.resize(NUM_COLORS);
    y_color[0] = 76;
    u_color[0] = 84;
    v_color[0] = 255;
    y_color[1] = 126;
    u_color[1] = 56;
    v_color[1] = 219;
    y_color[2] = 181;
    u_color[2] = 25;
    v_color[2] = 171;
    y_color[3] = 71;
    u_color[3] = 87;
    v_color[3] = 82;
    y_color[4] = 41;
    u_color[4] = 197;
    v_color[4] = 98;
    y_color[5] = 62;
    u_color[5] = 173;
    v_color[5] = 193;


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

void FrameProcessor::process_frame(FrameCollection frame_col, const bool block)
{
    /* When processing captures / movies, block rather than filling the queue as fast
     * as io will allow, so we don't get overruns */
    while (block && get_queue_size() > 100)
    {
        usleep(50000);
    }

    if (!frame_col.frame_rgb.get())
    {
        ScopedLock lock(_mtex);
        increment_dropped_frames();
        _frame_num++;
        return;
    }

    //cv::Mat m = frame->to_mat();
    //cout << "In process frame: " << m.cols << "x" << m.rows << endl;
    cv::Mat bgr = frame_col.frame_rgb->to_mat().clone();
    cv::cvtColor(bgr, bgr, cv::COLOR_BGR2RGB);
    wrReturnCode ret = poseEstimator->ProcessFrame(
                bgr.data, bgr.cols, bgr.rows, *poseOptions);
    //              frame_col.frame_rgb->to_mat().data, frame_col.frame_rgb->to_mat().cols, frame_col.frame_rgb->to_mat().rows, *poseOptions);
    if ( ret != wrReturnCode_OK ) {
        std::cout << "Error in ProcessFrame: " << ret << std::endl;
    }

    unsigned int numPersons2D = poseEstimator->GetNumHumans2D();
    
    std::ifstream f("/home/nvidia/options/greenscreen");
    if (f.good()) {
        frame_col.frame_y_copy->to_mat().setTo(cv::Scalar(133));
        frame_col.frame_u_copy->to_mat().setTo(cv::Scalar(52));
        frame_col.frame_v_copy->to_mat().setTo(cv::Scalar(82));
    }
    std::ifstream f2("/home/nvidia/options/color");
    bool useColor = false;
    if (f2.good()) {
        useColor = true;
    }
    if (numPersons2D) {
        //std::cout << "Inference done! Found "
        //    << numPersons2D << " 2d person(s)\n";
        //const unsigned int maxPersons = 32;
        //std::vector<wrnch::Pose2d> poses(maxPersons);
        unsigned int counter = 0;
	for (wrnch::PoseEstimator::Humans2dIter it = poseEstimator->Humans2dBegin();
                 it < poseEstimator->Humans2dEnd();
                 it++)
            {
		int colorIdx = counter % NUM_COLORS;
                int y_val = 255;
                int u_val = 128;
                int v_val = 128;
                if (useColor) {
                    y_val = y_color[colorIdx];
                    u_val = u_color[colorIdx];
                    v_val = v_color[colorIdx];
                }
                wrnch::Pose2dView pose = *it;
                DrawPoints(frame_col.frame_y_copy->to_mat(), pose.GetJoints(), pose.GetNumOutputJoints(), cv::Scalar(y_val), 8.f);
                DrawLines(
                    frame_col.frame_y_copy->to_mat(), pose.GetJoints(), bonePairs.data(), jointsDefinition.GetNumBones(), cv::Scalar(y_val), 8.f);
                DrawPoints(frame_col.frame_u_copy->to_mat(), pose.GetJoints(), pose.GetNumOutputJoints(), cv::Scalar(u_val), 8.f);
                DrawLines(
                    frame_col.frame_u_copy->to_mat(), pose.GetJoints(), bonePairs.data(), jointsDefinition.GetNumBones(), cv::Scalar(u_val), 8.f);
                DrawPoints(frame_col.frame_v_copy->to_mat(), pose.GetJoints(), pose.GetNumOutputJoints(), cv::Scalar(v_val), 8.f);
                DrawLines(
                    frame_col.frame_v_copy->to_mat(), pose.GetJoints(), bonePairs.data(), jointsDefinition.GetNumBones(), cv::Scalar(v_val), 8.f);
		++counter;
            }

    }
    
    {
        ScopedLock lock(_prebuffer_mtex);
        if (_prebuffer.size() == PREBUFFERED_FRAMES)  // Buffer size limit
        {
            _prebuffer.pop_back();
        }
        /* store a copy in the prebuffer, for when we turn on capture */
        _prebuffer.push_front(frame_col);
    }

    {
        ScopedLock lock(_mtex);
        if ( (_queue_size = _worker.add_job( boost::bind(&FrameProcessor::process_frame_task, this, frame_col, _frame_num, _frame_num-_previous_frame_num-1)) ) == -1 )
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

inline void FrameProcessor::DrawPoints(cv::Mat frame, const float* points,
                                   unsigned int numPoints,
                                   const cv::Scalar& color,
                                   float pointSize)
{
    const int w = frame.cols;
    const int h = frame.rows;

    for (int i = 0; i < numPoints; i++)
    {
        int x = points[2 * i] * w;
        int y = points[2 * i + 1] * h;
        if (x >= 0 && y >= 0)
        {
            cv::circle(
                frame, cv::Point(x, y), static_cast<int>(pointSize), color, cv::FILLED, cv::LINE_AA);
        }
    }
}

inline void FrameProcessor::DrawLines(cv::Mat frame, const float* points,
                                  const unsigned int* pairs,
                                  unsigned int numPairs,
                                  const cv::Scalar& color,
                                  float thickness)
{
    const int w = frame.cols;
    const int h = frame.rows;

    for (unsigned int i = 0; i < numPairs; i++)
    {
        const int& idx1 = pairs[2 * i];
        const int& idx2 = pairs[2 * i + 1];
        if (idx1 >= 0 && idx2 >= 0)
        {
            int x1 = points[idx1 * 2] * w;
            int y1 = points[idx1 * 2 + 1] * h;
            int x2 = points[idx2 * 2] * w;
            int y2 = points[idx2 * 2 + 1] * h;

            if (x1 >= 0 && x2 >= 0)
            {
                cv::line(frame,
                         cv::Point(x1, y1),
                         cv::Point(x2, y2),
                         color,
                         static_cast<int>(thickness),
                         cv::LINE_AA);
            }
        }
    }
}



void FrameProcessor::DrawPoints3D(cv::Mat frame, const float* points,
                                     unsigned int numPoints,
                                     const cv::Scalar& color,
                                     float pointSize)
{
    const int w = frame.cols;
    const int h = frame.rows;

    for (int i = 0; i < numPoints; i++)
    {
        int x = points[3 * i] * w;
        int y = points[3 * i + 1] * h;
        // auto z = points[3 * i + 2] Depth is stored here

        if (x >= 0 && y >= 0)
        {
            cv::circle(
                frame, cv::Point(x, y), static_cast<int>(pointSize), color, cv::FILLED, cv::LINE_AA);
        }
    }
}


std::string FrameProcessor::get_timing_string(void)
{
    std::ostringstream ret;
    return ret.str();
}

void FrameProcessor::do_stream(FrameCollection frame_col, const int frame_num, const int n_dropped_before)
{
    /* TODO: Potential optimization - don't for sequential section here if we're not going
     * to stream the frame. Requires some logic for figuring out dropped frames */

    // This depth scaling will need to be updated if we can ever get > 8 bit data out of libargus...
    //cv::Mat scaled_frame; 
    // if (frame_num % 2 == 0)
    // {
    //     cv::Mat stream_frame; 
    //     stream_frame = frame;

    //     if (frame.depth() == CV_16U)
    //     {
    //         stream_frame.convertTo(scaled_frame, CV_8U, 1/16.0);
    //     }
    //     else
    //     {
    //         scaled_frame = stream_frame;
    //     }
    // }

    if (!SequentialSection::too_late("stream", frame_num, n_dropped_before))
    {
        SequentialSection ss("stream", frame_num, n_dropped_before);
        //if (frame_num % 2 == 0)
        {
           _streamer->push_frame(frame_col);
        }
    }
}

void FrameProcessor::process_frame_task(FrameCollection frame_col, const int frame_num, const int n_dropped_before)
{
    const int queue_size = get_queue_size();
    const bool skip_processing = (queue_size > 350) && frame_num % 2 == 1;
    if (skip_processing)
    {
        std::cout << "Skipping frame " << frame_num << " because frame queue contains " << queue_size << " frames." << std::endl;
    }

    do_stream(frame_col, frame_num, n_dropped_before);
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

} // namespace BoulderAI

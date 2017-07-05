#pragma once

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "frame.hpp"
#include "new_worker.hpp"
#include "stream.hpp"

typedef bl::NewWorker < bl::Logexc_policy > BoundedWorkerBase;
class BoundedWorker : public BoundedWorkerBase
{
public:
    typedef boost::recursive_mutex::scoped_lock ScopedLock;
    
    BoundedWorker(const size_t threads, const int max_queue_size, const std::string name = "Unnamed Bounded")
        :
        BoundedWorkerBase(threads, name),
        _max_queue_size(max_queue_size)
    {

    }

    virtual ~BoundedWorker()
    {
        
    }

    int add_job(const bl::WorkerFunction &f)
    {
        ScopedLock lock(mtex);
        if(queue_size() == _max_queue_size)
        {
            return -1;
        }

        return BoundedWorkerBase::add_job(f);
    }
    
    size_t queue_size(void)
    {
        ScopedLock lock(mtex);
        return this->queue.size();
    }
    
protected:
    const int _max_queue_size;


};

namespace CaptureState
{
    enum CaptureState
    {
        AUTO, // an intermediate state where the user specified auto mode but
              // we haven't detected a state yet
        OFF,
        AUTO_OFF,
        ON,
        AUTO_ON,
    };
}

enum class StreamState
{
    OFF,
    RAW,
    HDR,
    WATER_LEVEL,
    BACKGROUND,
    TRACKS
} ;

typedef std::deque < FramePtr > FrameQueue;
typedef FrameQueue::iterator FrameQueueIter;
typedef FrameQueue::reverse_iterator FrameQueueRIter;

class SequentialSection
{
public:
    /* Do the initialization first, using this constructor in a singlethreaded
     * manner. Then, later, use the other constructor for scoped, sequential access */
    SequentialSection(const std::string &name)
    {
        _mutex[name].reset(new boost::mutex());
        _condition[name].reset(new boost::condition_variable());
        _frame_num[name] = -1;
        _name = name;

    }

    SequentialSection(const std::string &name, const int frame_num, const int n_dropped_before) :
        _lock(*_mutex[name]),
        _name(name)
    {
        std::ostringstream oss; 
        oss << name << " wait"; 

        if (frame_num < _frame_num[name]+n_dropped_before) 
        {
            throw std::runtime_error("Frame numbers must always increase in a sequential section."); 
        }

        /* Only log a wait event if we actually wait */
        if (frame_num != _frame_num[name]+n_dropped_before)
        {
            while(frame_num != _frame_num[name]+n_dropped_before)
            {
                _condition[name]->wait(_lock);
            }
        }
        _frame_num[_name] += n_dropped_before;
    }

    ~SequentialSection()
    {
        _frame_num[_name] += 1; 
        _condition[_name]->notify_all();
    }

    /**
     * If we have a task that can start or stop during the processing, we 
     * don't want to force things to run seqentially when the task is stopped. 
     * If we just pick the current frame number when start is clicked, there may
     * be some frames that were already in flight when start was clicked. Those frames
     * are "too late". 
     */ 
    static bool too_late(const std::string &name, int frame_num, int n_dropped_before)
    {
        boost::mutex::scoped_lock lock(*_mutex[name]);
        return (frame_num < _frame_num[name]+n_dropped_before);
    }

    static void set_next_frame(const std::string &name, const int &frame_num)
    {
        boost::mutex::scoped_lock lock(*_mutex[name]);
        _frame_num[name]=frame_num;
    }
    
private: 
    std::string _name;
    boost::unique_lock<boost::mutex> _lock;
    static std::map<std::string, boost::shared_ptr<boost::mutex> > _mutex;
    static std::map<std::string, boost::shared_ptr<boost::condition_variable> > _condition; 
    static std::map<std::string, int> _frame_num; 
} ; 

class FrameProcessor : public boost::enable_shared_from_this<FrameProcessor>
{
public:
    FrameProcessor(const int w, const int h);
    virtual ~FrameProcessor();

    void start_workers(void);
    
    void process_frame(FramePtr frame, const bool block=false);
    void wait_for_queued_images(); 

    void set_stream_state(const StreamState state);
    void set_stream_state(const std::string &state);
    std::string get_stream_state(); 

    boost::posix_time::time_duration get_mean_tracking_time()
    {
        return boost::posix_time::time_duration(0,0,0); // No longer used. so just return 0 to prevent division by zero errors.
        //if (_frame_num == 0)
        //return _tracking_time;  // Dan: Should make this return zero though.
        //return _tracking_time/_frame_num;
    }

    int get_queue_size();
    int get_dropped_frames(void);

    void increment_dropped_frames(void);
    
    std::string get_timing_string(void);

protected:
    typedef boost::mutex::scoped_lock ScopedLock;

    /**
     * Note that we pass the cv::Mat by value, NOT by reference. Passing by reference WON'T work
     * because the data will cease to exist as soon as the previous thread is done with it.
     * There is a tiny performance penalty in passing by value because the header information
     * needs to be copied, but the data does not; a cv::Mat is essentially a smart pointer
     * to a data buffer. So this is plenty efficient.
     */
    void process_frame_task(FramePtr frame, const int frame_num, const int n_dropped_before);

    void do_stream(cv::Mat frame, const int frame_num, const int n_dropped_before);

    bool _created_window;
    int _frame_width;
    int _frame_height;
    int _frame_num;
    int _previous_frame_num;
    int _queue_size;
    int _dropped_frames;
    int _prebuffer_post_frames;
    StreamState _stream_state;
    StreamPtr _streamer;
    FrameQueue _prebuffer;

    boost::mutex _mtex;
    boost::mutex _prebuffer_mtex;

    BoundedWorker _worker;
    bl::NewWorker < bl::Logexc_policy > _gui_worker;
    
    boost::posix_time::time_duration _tracking_time;
} ;

typedef boost::shared_ptr < FrameProcessor > FrameProcessorPtr;

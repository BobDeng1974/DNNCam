#ifndef NEWWORKER_HPP
#define NEWWORKER_HPP
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <deque>
#include <vector>

// Preprocessor macro to accommodate name change for Boost condition variable.
#if (BOOST_VERSION < 104700)
#define BOOST_CONDITION_VARIABLE boost::condition
#else
#define BOOST_CONDITION_VARIABLE boost::condition_variable_any
#endif

namespace bl 
{

    typedef boost::function<void(void)> WorkerFunction;
    typedef boost::recursive_mutex::scoped_lock ScopedLock;

    struct WorkerFunctionWithTag
    {
        WorkerFunction function;
        std::string tag;
    };

    // won't catch exceptions raised by worker function
    class Uncaught_policy {
    protected:
        void call_func(const WorkerFunction &f)
        {
            f();
        }
    };

    // logs exceptions raised by worker function
    class Logexc_policy {
    protected:
        void call_func(const WorkerFunction &f)
        {
            try {
                f();
            }
            catch (std::exception &e)
            {
                std::cout << "Caught exception in worker thread: " << std::endl;
            }
        }
    };
        
    
    template<typename exception_policy=Uncaught_policy>
    class NewWorker : public exception_policy
    {
        using exception_policy::call_func;
        
    public:
        NewWorker(const size_t threads, const std::string name = "Unnamed")
            :
            num_threads(threads),
            jobs_running(0),
            running(false),
            _name(name),
            threadgroupPtr(new boost::thread_group)
        {
            assert(num_threads > 0);
        }

        virtual ~NewWorker(void)
        {
            stop();
        }

        // call start to begin the thread pool
        void start()
        {
            ScopedLock lock(mtex);
            if(!running)
            {
                this->running = true;
                for(size_t i = 0; i < num_threads; ++i)
                {
                    boost::thread* p = new boost::thread(boost::bind(&bl::NewWorker<exception_policy>::runLoop, this));
                    this->threadgroupPtr->add_thread(p);
                }
            }
        }

        // call this to queue up a new job
        int add_job(const WorkerFunction &f)
        {
            ScopedLock lock(mtex);
            WorkerFunctionWithTag wft;
            wft.tag = "";
            wft.function = f;
            this->queue.push_back(wft);
            this->condition.notify_one();
            return this->queue.size();
        }

        // call this to queue up a new job with a user-defined tag
        int add_job(const WorkerFunctionWithTag &wft)
        {
            ScopedLock lock(mtex);
            this->queue.push_back(wft);
            this->condition.notify_one();
            return this->queue.size();
        }

        // removes all jobs in queue
        void clear_jobs()
        {
            ScopedLock lock(mtex);
            this->queue.clear();
        }

        /**
         * Immediately forces the worker threads to stop and joins on them,
         * regardless of whether there is work remaining in the queue.
         * Does NOT guarantee completion of all work in the queue;
         * any remaining work will simply be left in the queue.
         */
        void stop()
        {
            ScopedLock lock(mtex);
            if(running)
            {
                running = false;
                //notify waiters to go
                this->condition.notify_all();
                lock.unlock();
                threadgroupPtr->join_all();
                lock.lock();
                threadgroupPtr.reset(new boost::thread_group);
            }
        }

        /**
         * Block and wait for the worker thread to finish its jobs.
         * Does nothing if worker thread is not running
         * returns 1 if successful
         */
        int wait()
        {
            ScopedLock lock(mtex);
            while (running && (jobs_running > 0 || queue.size() > 0))
            {
                wait_condition.wait(lock);
            }
            return 1;
        }

        /**
         * Return the number of jobs presently in the queue
         */
        size_t getNumJobsRunning()
        {
            ScopedLock lock(mtex);
            return queue.size();
        }

        size_t deleteAllJobsWithTag(std::string tag)
        {
            ScopedLock lock(mtex);
            WorkerQueue::iterator it = queue.begin();
            size_t count = 0;

            while(it != queue.end())
            {
                if (it->tag == tag)
                {
                    it = queue.erase(it);
                    ++count;
                }
                else
                {
                    ++it;
                }
            }

            return count;
        }

    protected:
        typedef std::vector<boost::shared_ptr<boost::thread> > ThreadPtr;
        typedef std::deque<WorkerFunctionWithTag> WorkerQueue;

        void runLoop(void)
        {
            ScopedLock lock(this->mtex);
            while(this->running)
            {
                WorkerFunction f;
                // need to get the lock external to the queue, to make the empty check atomic
                // with repect to the pop
                if(queue.empty())
                {
                    //queue is empty, wait for more things to be added
                    condition.wait(lock);
                }
                else
                {
                    f = queue.front().function;
                    queue.pop_front();
                    ++jobs_running;
                    lock.unlock();
                    call_func(f);
                    lock.lock();
                    --jobs_running;
                }

                if(jobs_running == 0 && queue.size() == 0)
                {
                    wait_condition.notify_all(); // all jobs completed
                }
            }

            // Since the worker exited the above loop, it means you are shutting down.
            // If there was any remaining work in the queue, wait_condition.notify_all()
            // was skipped above, so call it now. That way, any threads blocking on
            // NewWorker::wait() can stop waiting.
            wait_condition.notify_all();
        }

        size_t num_threads;
        size_t jobs_running;
        bool running;
        const std::string _name;

        boost::shared_ptr<boost::thread_group> threadgroupPtr;
        WorkerQueue queue;
        boost::recursive_mutex mtex;
        BOOST_CONDITION_VARIABLE condition;
        BOOST_CONDITION_VARIABLE wait_condition;
    };
    
}


#endif

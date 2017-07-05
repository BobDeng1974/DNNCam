#ifndef BLSAFEQUEUE_HPP
#define BLSAFEQUEUE_HPP

#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

namespace bl
{
    template <class T>
    class SafeQueue
    {
    public:
        SafeQueue(){};
        ~SafeQueue(){};
        
        T PopNolock(void)
        {
            T r(sq.front());
            this->sq.pop();    
            return r;            
        }
        
        T pop()
        {
            boost::mutex::scoped_lock scoped_lock(this->qmutex);
            return PopNolock();
        }

        T FrontNolock(void)
        {
            return this->sq.front();
        }

        T front()
        {
            boost::mutex::scoped_lock scoped_lock(this->qmutex);
            return FrontNolock();
        }
        
        void PushNolock(const T &thing)
        {
            this->sq.push(thing);            
        }
        
        void push(const T& thing)
        {
            boost::mutex::scoped_lock scoped_lock(this->qmutex);
            PushNolock(thing);
        }
        
        bool EmptyNolock(void)
        {
            return this->sq.empty();            
        }
        
        bool empty()
        {
            boost::mutex::scoped_lock scoped_lock(this->qmutex);
            return EmptyNolock();
        }
        
        size_t SizeNolock(void)
        {
            return this->sq.size();            
        }
        
        size_t size()
        {
            boost::mutex::scoped_lock scoped_lock(this->qmutex);
            return SizeNolock();
        }

        size_t ClearNolock(void)
        {
            return this->sq.clear();            
        }
        
        size_t clear()
        {
            boost::mutex::scoped_lock scoped_lock(this->qmutex);
            return ClearNolock();
        }

        boost::mutex &GetMutex(void)
        {
            return qmutex;
        }
    
    private:
        std::queue <T> sq;
        boost::mutex qmutex;
    };

} // ns: bl
#endif // BLSAFEQUEUE_HPP

#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <cstddef>

class Timer{
	public:
		unsigned int GetTickCount();
	private:
		unsigned int start_t;
		unsigned int end_t;
		unsigned int prev_t;
		unsigned int next_t;
};
#endif

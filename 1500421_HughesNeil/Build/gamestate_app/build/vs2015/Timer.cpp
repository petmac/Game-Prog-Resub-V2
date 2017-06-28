#include "Timer.h"



Timer::Timer()
{

	time_ = 0.0f;
}


Timer::~Timer()
{
}


void Timer::set_timer(float t) 
{

	maxTime_ = t;
}

bool Timer::update(float dt) 
{
	time_ += dt;

	if (time_ >= maxTime_) 
	{
		time_ = 0;

		return true;
	}

	return false;
}
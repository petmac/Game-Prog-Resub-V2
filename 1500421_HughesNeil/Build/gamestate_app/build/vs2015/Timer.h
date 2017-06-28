#pragma once
class Timer
{
public:
	Timer();
	~Timer();

	void set_timer(float t);

	bool update(float dt);

private:
	float time_;
	float maxTime_;
};


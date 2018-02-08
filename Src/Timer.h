#pragma once
#include <time.h>


class Timer
{
	struct timespec m_tagStartCPUTime;
	struct timespec m_tagEndCPUTime;
	struct timespec m_tagStartThreadCPUTime;
	struct timespec m_tagEndThreadCPUTime;
public:
	Timer();
	~Timer();
	void Start();
	void Stop();
	void StartThread();
	void StopThread();
	double getElapsedTime_s();
	double getElapsedTime_ms();
	double getElapsedThreadTime_s();
        double getElapsedThreadTime_ms();
	unsigned long long getElapsedTime_100ns();
	unsigned long long getElapsedThreadTime_100ns();
};


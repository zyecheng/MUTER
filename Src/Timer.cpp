#include "Timer.h"
#include <assert.h>
#include <pthread.h>


Timer::Timer()
{
}


Timer::~Timer()
{
}

void Timer::Start()
{
	int iStatus = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &m_tagStartCPUTime);
	assert(iStatus == 0);
}

void Timer::StartThread()
{
	clockid_t cid;
	int iStatus = pthread_getcpuclockid(pthread_self(), &cid);
	assert(iStatus == 0);
	iStatus = clock_gettime(cid, &m_tagStartThreadCPUTime);
        assert(iStatus == 0);
}

void Timer::Stop()
{
	int iStatus = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &m_tagEndCPUTime);
        assert(iStatus == 0);
}

void Timer::StopThread()
{
	clockid_t cid;
        int iStatus = pthread_getcpuclockid(pthread_self(), &cid);
        assert(iStatus == 0);
        iStatus = clock_gettime(cid, &m_tagEndThreadCPUTime);
        assert(iStatus == 0);
}

double Timer::getElapsedTime_s()
{
	double dSec = m_tagEndCPUTime.tv_sec - m_tagStartCPUTime.tv_sec +  (double)(m_tagEndCPUTime.tv_nsec - m_tagStartCPUTime.tv_nsec)/(double)1e9;
	return dSec;
}

double Timer::getElapsedTime_ms()
{
	return 1000 * getElapsedTime_s();
}

double Timer::getElapsedThreadTime_s()
{
        double dSec = m_tagEndThreadCPUTime.tv_sec - m_tagStartThreadCPUTime.tv_sec + (double)(m_tagEndThreadCPUTime.tv_nsec - m_tagStartThreadCPUTime.tv_nsec)/(double)1e9;
        return dSec;
}

double Timer::getElapsedThreadTime_ms()
{
        return getElapsedThreadTime_s() * 1e3;
}


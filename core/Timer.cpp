#include "Timer.h"

namespace core
{
	// -------------------------------------------------------------------------
	Timer::Timer()
		: mCurrTime(0), mPrevTime(0), mFrameTime(0), mTime(0)
	{
	}
	// -------------------------------------------------------------------------
	Timer::~Timer()
	{
	}
	// -------------------------------------------------------------------------
	void Timer::reset()
	{
		mTime = 0;
		mFrameTime = 0;
#ifdef WIN32
		// Based on Humus'es timer

		// Force the main thread to always run on CPU 0.
		// This is done because on some systems QueryPerformanceCounter returns a bit different counter values
		// on the different CPUs (contrary to what it's supposed to do), which can cause negative frame times
		// if the thread is scheduled on the other CPU in the next frame. This can cause very jerky behavior and
		// appear as if frames return out of order.

		SetThreadAffinityMask(GetCurrentThread(), 1);
		QueryPerformanceFrequency(&mFrequency);
#else
		gettimeofday(&mStart, NULL);
#endif
		mCurrTime = getCurrentTime();
	}
	// -------------------------------------------------------------------------
	void Timer::updateTime()
	{
		mPrevTime = mCurrTime;

		mCurrTime = getCurrentTime();
		mFrameTime = mCurrTime - mPrevTime;

		mTime += mFrameTime;
	}
	// -------------------------------------------------------------------------
	float Timer::getFrameTime()
	{
		return mFrameTime;
	}
	// -------------------------------------------------------------------------
	float Timer::getCurrentTime()
	{
#ifdef WIN32
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);
		return static_cast<float>(
			static_cast<double>(time.QuadPart) / static_cast<double>(mFrequency.QuadPart));
#else
		timeval curr;
		gettimeofday(&curr, NULL);
		return (static_cast<float>(curr.tv_sec - mStart.tv_sec) +
			0.000001f * static_cast<float>(curr.tv_usec - mStart.tv_usec));
#endif
	}
	// -------------------------------------------------------------------------
	float Timer::getFps(float sampleInterval)
	{
		static float accTime = 0;
		static int nFrames = 0;
		static float fps = 0;

		accTime += mFrameTime;
		nFrames++;

		if(accTime >= sampleInterval)
		{
			fps = nFrames / accTime;
			nFrames = 0;
			accTime = 0;
		}
		return fps;
	}
	// -------------------------------------------------------------------------
}

#pragma once

#include "Prerequisites.h"

#ifdef WIN32
//  http://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#	define WIN32_LEAN_AND_MEAN
#	define _WIN32_WINNT 0x0501 // Specifies that the minimum required platform is Windows XP.
#	ifndef WINVER
#		define WINVER 0x0501
#	endif
#	include <windows.h>
#else
#	include <sys/time.h>
#endif

namespace core
{
	class Timer
	{
	public:
		Timer();
		~Timer();

		void reset();
		void updateTime();
		
		float getFrameTime();
		float getCurrentTime();

		float getFps(float sampleInterval = 0.1f);

	private:
		float mCurrTime;
		float mPrevTime;
		float mFrameTime;
		// Czas istnienia timera
		float mTime;
#ifdef WIN32
		LARGE_INTEGER mFrequency;
#else
		timeval mStart;
#endif
	};
}
#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_

//#include <windows.h>
#include<chrono>
using namespace std::chrono;

class CELLTimestamp
{
public:
	CELLTimestamp()
	{
		update();
	}
	void update()
	{
		_begin = high_resolution_clock::now();
	}
	//获取当前秒
	double getElapsedSec()
	{
		return getElapsedTimeMicroSec() * 0.000001;
	}
	//获取当前毫秒
	double getElapsedTimeMilliSec()
	{
		return getElapsedTimeMicroSec() * 0.001;
	}
	//获取当前微秒
	long long getElapsedTimeMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
protected:
	time_point<high_resolution_clock> _begin;
};

#endif // !_CELLTimestamp_hpp_
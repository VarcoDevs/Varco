//  Copyright 2008 Alesiani Marco <marco.diiga@gmail.com>
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include "windows.h"

//
//	High resolution timer C++ class for Windows Systems
//
//	Usage:
//
//		CStopWatch m_tmr;
//		m_tmr.startTimer();
//
//		// Performance intensive event here
//		...
//
//		m_tmr.stopTimer();
//		double sec = m_tmr.getElapsedTimeInSeconds();
//		double msec = sec * 1000;
//		cout << "Done in " << sec << " seconds / " << msec << " milliseconds";
//
//	Other alternatives to measure time (platform-independent) are clock() to measure the number of ms elapsed 
//	between two events or gettimeofday (posix-compliant). None of them provide the accuracy of QueryPerformanceCounter
//

typedef struct 
{
    LARGE_INTEGER start;
    LARGE_INTEGER stop;
} stopWatch;

// To measure time accurately under Windows systems the QueryPerformanceCounter API can be used
// Its resolution is accurate in order of useconds (microseconds)
class CStopWatch 
{
private:
    stopWatch timer;
    LARGE_INTEGER frequency;

    // Returns the number of seconds (with a double value) elapsed
    double LIToSecs(LARGE_INTEGER & L)
    {
        return ((double)L.QuadPart /(double)frequency.QuadPart) ;
    }

public:
    CStopWatch()
    {
        timer.start.QuadPart=0;
        timer.stop.QuadPart=0; 
        // Retrieves the frequency of the high-resolution performance counter, if one exists. 
        // The frequency cannot change while the system is running.
        QueryPerformanceFrequency(&frequency) ;
    }
    void startTimer()
    {
        QueryPerformanceCounter(&timer.start) ;
    }
    void stopTimer()
    {
        QueryPerformanceCounter(&timer.stop) ;
    }
    double getElapsedTimeInSeconds()
    {
        LARGE_INTEGER time;
        time.QuadPart = timer.stop.QuadPart - timer.start.QuadPart;
        return LIToSecs(time) ;
    }
};
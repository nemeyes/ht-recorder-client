#ifndef _MEMORY_LEAK_DETECTOR_H_
#define _MEMORY_LEAK_DETECTOR_H_
#if defined(WIN32)
#if defined(_DEBUG)
#include <crtdbg.h> 
static _CRT_REPORT_HOOK prevHook; 
static bool SwallowReport; 
class MemoryLeakDetector
{ 
public: 
        MemoryLeakDetector(); 
        virtual ~MemoryLeakDetector(); 
}; 

int ReportingHook( int reportType, char* userMessage, int* retVal ); 
#endif
#endif
#endif
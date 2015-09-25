#include "Common.h"
#include "MemoryLeakDetector.h"
#if defined(WIN32)
#if defined(_DEBUG)
#include <string.h> 
MemoryLeakDetector::MemoryLeakDetector() 
{ 
    //don't swallow assert and trace reports 
    SwallowReport = false; 
    //change the report function 
    prevHook = _CrtSetReportHook(ReportingHook); 
} 

//this destructor is called after mfc has died 
MemoryLeakDetector::~MemoryLeakDetector() 
{ 
	//make sure that there is memory leak detection at the end of the program 
	_CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF ); 
	//reset the report function to the old one 
	_CrtSetReportHook(prevHook); 
} 

//static MemoryLeakDetector MLD;  //this lives as long as this file 

int ReportingHook( int reportType, char* userMessage, int* retVal ) 
{ 
//_CrtDumpMemoryLeaks() outputs "Detected memory leaks!\n" and calls 
//_CrtDumpAllObjectsSince(NULL) which outputs all leaked objects, 
//ending this (possibly long) list with "Object dump complete.\n" 
//In between those two headings I want to swallow the report. 
    if( (strcmp(userMessage,"Detected memory leaks!\n")==0) || SwallowReport ) 
	{ 
        if( strcmp(userMessage,"Object dump complete.\n")==0 ) SwallowReport = false; 
        else SwallowReport = true; 
        return true;  //swallow it 
    } 
    else return false; //give it back to _CrtDbgReport() 
};
#endif
#endif
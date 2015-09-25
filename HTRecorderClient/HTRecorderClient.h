
// HTRecorderClient.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CHTRecorderClientApp:
// See HTRecorderClient.cpp for the implementation of this class
//

class CHTRecorderClientApp : public CWinApp
{
public:
	CHTRecorderClientApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CHTRecorderClientApp theApp;
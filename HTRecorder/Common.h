// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.

#include <afxwin.h>         // MFC 핵심 및 표준 구성 요소입니다.
#include <afxext.h>         // MFC 확장입니다.

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE 클래스입니다.
#include <afxodlgs.h>       // MFC OLE 대화 상자 클래스입니다.
#include <afxdisp.h>        // MFC 자동화 클래스입니다.
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC 데이터베이스 클래스입니다.
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO 데이터베이스 클래스입니다.
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // Internet Explorer 4 공용 컨트롤에 대한 MFC 지원입니다.
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // Windows 공용 컨트롤에 대한 MFC 지원입니다.
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>            // MFC 소켓 확장입니다.

#define MAX_LIVE_CHANNEL (64)
#define DEFAULT_NCSERVICE_PORT				11550	// Nautilus 레코딩 서버 기본 Port
#define NAUTILUS_FILE_VERSION				10300
#define NAUTILUS_DB_VERSION					5

#include <process.h>
#include <map>
#include <set>
#include <queue>
#include <XML.h>
#include <LiveSession5.h>
#include <MessageSender.h>
#include <LiveSessionDLL.h>
#include <HTRecorderDLL.h>
#include <atlconv.h>

#define MAKEUINT64(l, h)	(unsigned __int64)( ((unsigned __int64)h << 32) | ((unsigned __int64)l & 0xffffffff) )
#define MAKEHIULONG(x)		(unsigned long)(x >> 32)
#define MAKELOULONG(x)		(unsigned long)(x & 0xffffffff)


#define XML_UTF8_HEAD_WSTR					L"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
#define XML_UTF8_HEAD_STR					"<?xml version=\"1.0\" encoding=\"utf-8\"?>"


/*
// memory leak check
#include <crtdbg.h>
#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_CLIENTBLOCK
*/
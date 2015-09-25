#pragma once
#include <atlstr.h>
#include <map>
using namespace std;

class CPairStringRead
{
public:
	CPairStringRead(void);
	CPairStringRead(LPCSTR lpszString, const TCHAR cSplit1 = '=', const TCHAR cSplit2 = '&');
	CPairStringRead(LPCTSTR lpszString, const TCHAR cSplit1 = '=', const TCHAR cSplit2 = '&');
	~CPairStringRead(void);

	void SetString(LPCSTR lpszString, const TCHAR cSplit1 = '=', const TCHAR cSplit2 = '&');
	void SetString(LPCTSTR lpszString, const TCHAR cSplit1 = '=', const TCHAR cSplit2 = '&');
	CString GetParamValue(LPCTSTR lpszName);
	BOOL GetParamValue(LPCTSTR lpszName, CString &strValue);

	map<CString, CString> m_mapParamList;

private:	
	TCHAR m_cSplit1;
	TCHAR m_cSplit2;
};

#undef MFCSTRING

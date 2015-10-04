#include "Common.h"
#include "pairstringread.h"

CPairStringRead::CPairStringRead(void)
{
	m_cSplit1 = '=';
	m_cSplit2 = '&';
}

CPairStringRead::CPairStringRead(LPCSTR lpszString, const TCHAR cSplit1 /*= '='*/, const TCHAR cSplit2 /*= '&'*/)
{
	if(!lpszString)
		return;

	size_t len = MultiByteToWideChar(CP_ACP, 0, lpszString, -1, NULL, 0);
	if(len > 0)
	{
		wchar_t *pUnicode = new wchar_t[len+1];
		MultiByteToWideChar(CP_ACP, 0, lpszString, -1, pUnicode, len);
		pUnicode[len] = L'\0';

		SetString(pUnicode, cSplit1, cSplit2);

		delete [] pUnicode;
	}
}

CPairStringRead::CPairStringRead(LPCTSTR lpszString, const TCHAR cSplit1, const TCHAR cSplit2)
{
	SetString(lpszString, cSplit1, cSplit2);
}

CPairStringRead::~CPairStringRead(void)
{
}

void CPairStringRead::SetString(LPCSTR lpszString, const TCHAR cSplit1 /*= '='*/, const TCHAR cSplit2 /*= '&'*/)
{
	if(!lpszString)
		return;

	size_t len = MultiByteToWideChar(CP_ACP, 0, lpszString, -1, NULL, 0);
	if(len > 0)
	{
		wchar_t *pUnicode = new wchar_t[len+1];
		MultiByteToWideChar(CP_ACP, 0, lpszString, -1, pUnicode, len);
		pUnicode[len] = L'\0';

		SetString(pUnicode, cSplit1, cSplit2);

		delete [] pUnicode;
	}
}

void CPairStringRead::SetString(LPCTSTR lpszString, const TCHAR cSplit1, const TCHAR cSplit2)
{
	m_mapParamList.clear();
	m_cSplit1 = cSplit1;
	m_cSplit2 = cSplit2;

	int done = 0;
	int i;
	TCHAR *ptr;
	const TCHAR *resp = lpszString;
	TCHAR buf[4096];

	while(!done)
	{
		i = 0;

		if( !(resp && *resp) )
			break;

		while(resp && *resp && *resp != cSplit2 && i < 4096-1)
		{
			buf[i++] = *resp;
			resp++;
		}
		buf[i] = '\0';

		if(resp && *resp && i==0)
		{
			resp++;
			continue;
		}

		ptr = buf;
		while(ptr && *ptr && *ptr != cSplit1)
			ptr++;

		if(ptr && *ptr)
			*ptr = 0;
		else
			return;

		ptr++;

		m_mapParamList.insert( pair<CString, CString>(buf, ptr) );


		if(resp && *resp)
			resp++;
		else
			done = true;
	}
}

CString CPairStringRead::GetParamValue(LPCTSTR lpszName)
{
	map<CString, CString>::iterator pos;
	pos = m_mapParamList.find(lpszName);

	if(pos != m_mapParamList.end())
		return pos->second;

	return _T("");
}

BOOL CPairStringRead::GetParamValue(LPCTSTR lpszName, CString &strValue)
{
	BOOL bResult;
	map<CString, CString>::iterator pos;
	pos = m_mapParamList.find(lpszName);

	if(pos != m_mapParamList.end())
	{
		bResult = TRUE;
		strValue = pos->second;
	}
	else
		bResult = FALSE;

	return bResult;
}

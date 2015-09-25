#pragma once

enum EXCEPTION_LEVEL_TYPE
{
	EXCEPTION_LEVEL_NORMAL = 0,
	EXCEPTION_LEVEL_WARN,
	EXCEPTION_LEVEL_ERROR
};

class CExceptionBase
{
public:
	CExceptionBase()
	{
		Initialize(EXCEPTION_LEVEL_NORMAL, L"", L"");
	};

	CExceptionBase(const LPCTSTR lpMessage)
	{
		Initialize(EXCEPTION_LEVEL_NORMAL, L"", lpMessage);
	};

	CExceptionBase(enum EXCEPTION_LEVEL_TYPE eType, LPCTSTR lpMessage)
	{
		Initialize(eType, L"", lpMessage);
	};

	CExceptionBase(enum EXCEPTION_LEVEL_TYPE eType, LPCTSTR lpCategory, LPCTSTR lpMessage)
	{
		Initialize(eType, lpCategory, lpMessage);
	};

	CExceptionBase(const CExceptionBase& e)
	{
		m_eLevel = e.m_eLevel;
		m_strCategory = e.m_strCategory;
		m_strMessage = e.m_strMessage;
	};

	EXCEPTION_LEVEL_TYPE GetLevel() { return m_eLevel; };
	CString GetCategory() { return m_strCategory; };
	CString GetMessage() { return m_strMessage; };

	CString ToString() 
	{ 
		CString strTemp;
		strTemp.Format(L"[%s][%s] %s", ConvertString(m_eLevel), m_strCategory, m_strMessage);
		return strTemp; 
	};

protected:
	EXCEPTION_LEVEL_TYPE m_eLevel;
	CString m_strCategory;
	CString m_strMessage;

	void Initialize(EXCEPTION_LEVEL_TYPE eLevel, LPCTSTR lpCategory, LPCTSTR lpMessage)
	{
		m_eLevel = eLevel;
		m_strCategory = lpCategory;
		m_strMessage = lpMessage;
	};

	LPCTSTR ConvertString(enum EXCEPTION_LEVEL_TYPE eType)
	{
		switch(eType)
		{
		case EXCEPTION_LEVEL_NORMAL:
			return L"INFO";
		case EXCEPTION_LEVEL_WARN:
			return L"WARN";
		case EXCEPTION_LEVEL_ERROR:
			return L"ERROR";
		}

		return L"UNKNOWN";
	};
};
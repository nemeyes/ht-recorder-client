#include "Common.h"
#include "string_helper.h"

void _sprintf_string(string &str, const char *lpFormat, ...)
{
	va_list vlMaker;

	va_start(vlMaker, lpFormat);
	size_t len = _vscprintf(lpFormat, vlMaker);
	str.resize(len);
	vsprintf((char*)str.c_str(), lpFormat, vlMaker);
	va_end(vlMaker);
}

void _wsprintf_string(wstring &str, const wchar_t *lpFormat, ...)
{
	va_list vlMaker;

	va_start(vlMaker, lpFormat);
	size_t len = _vscwprintf(lpFormat, vlMaker);
	str.resize(len);
	vswprintf((wchar_t*)str.c_str(), lpFormat, vlMaker);
	va_end(vlMaker);
}

string	WStringToUTF8(const wchar_t *lpStr)
{
	if(!lpStr)
		return "";

	int len;

	len = WideCharToMultiByte(CP_UTF8, 0, lpStr, -1, NULL, 0, NULL, NULL);
	char *pUTF8 = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, lpStr, -1, pUTF8, len, NULL, NULL);

	string s(pUTF8);

	delete [] pUTF8;

	return s;
}

wstring UTF8ToWString(const char *lpStr)
{
	if(!lpStr)
		return L"";

	wstring s;

	int len;
	len = MultiByteToWideChar(CP_UTF8, 0, lpStr, -1, NULL, 0);
	if(len > 0)
	{
		s.resize(len);
		MultiByteToWideChar(CP_UTF8, 0, lpStr, -1, (wchar_t*)s.c_str(), len);
	}

	return s;
}

wstring	AStringToWString(const char *lpStr)
{
	USES_CONVERSION_EX;
	if(!lpStr)
		return L"";

	wchar_t *ptr = A2W_EX(lpStr, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);

	wstring s(ptr);

	return s;
}

wstring	AStringToWString(const string& str)
{
	USES_CONVERSION_EX;
	if(str.empty())
		return L"";

	wchar_t *ptr = A2W_EX(str.c_str(), _ATL_SAFE_ALLOCA_DEF_THRESHOLD);

	wstring s(ptr);

	return s;
}

string WStringToAString(const wchar_t *lpStr)
{
	USES_CONVERSION_EX;
	if(!lpStr)
		return "";

	char *ptr = W2A_EX(lpStr, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);

	string s(ptr);

	return s;
}

string WStringToAString(const wstring& str)
{
	USES_CONVERSION_EX;
	if(str.empty())
		return "";

	char *ptr = W2A_EX(str.c_str(), _ATL_SAFE_ALLOCA_DEF_THRESHOLD);

	string s(ptr);

	return s;
}

vector<string> MakeArrayStringA(const char *src, const char split)
{
	vector<string> v;

	char sTemp[1024];
	const char *ptr = src;

	size_t i = 0;
	while(ptr && *ptr)
	{
		if(*ptr == split) {
			sTemp[i] = '\0';
			i = 0;
			ptr++;
			v.push_back(string(sTemp));
		}

		sTemp[i++] = *ptr;
		ptr++;
	}

	sTemp[i] = '\0';

	if(i > 0)
		v.push_back(string(sTemp));

	return v;
}

vector<wstring> MakeArrayStringW(const wchar_t *src, const wchar_t split)
{
	vector<wstring> v;

	wchar_t sTemp[1024];
	const wchar_t *ptr = src;

	size_t i = 0;
	while(ptr && *ptr)
	{
		if(*ptr == split) {
			sTemp[i] = L'\0';
			i = 0;
			ptr++;
			v.push_back(wstring(sTemp));

			continue;
		}

		sTemp[i++] = *ptr;
		ptr++;
	}

	sTemp[i] = L'\0';

	if(i > 0)
		v.push_back(wstring(sTemp));

	return v;
}

set<string> MakeSetStringA(const char *src, const char split)
{
	set<string> v;

	char sTemp[1024];
	const char *ptr = src;

	size_t i = 0;
	while(ptr && *ptr)
	{
		if(*ptr == split) {
			sTemp[i] = '\0';
			i = 0;
			ptr++;
			v.insert(string(sTemp));

			continue;
		}

		sTemp[i++] = *ptr;
		ptr++;
	}

	sTemp[i] = '\0';

	if(i > 0)
		v.insert(string(sTemp));

	return v;
}

set<wstring> MakeSetStringW(const wchar_t *src, const wchar_t split)
{
	set<wstring> v;

	wchar_t sTemp[1024];
	const wchar_t *ptr = src;

	size_t i = 0;
	while(ptr && *ptr)
	{
		if(*ptr == split) {
			sTemp[i] = L'\0';
			i = 0;
			ptr++;
			v.insert(wstring(sTemp));
		}

		sTemp[i++] = *ptr;
		ptr++;
	}

	sTemp[i] = L'\0';

	if(i > 0)
		v.insert(wstring(sTemp));

	return v;
}

void _replace_string(string& str, const string& find, const string& dest)
{
	size_t n;

	n = str.find(find);
	while( n != wstring::npos)
	{
		str.replace(n, find.length(), dest);
		n = str.find(find, n + find.length());
	}
}

void _replace_string(wstring& str, const wstring& find, const wstring& dest)
{
	size_t n;

	n = str.find(find);
	while( n != wstring::npos)
	{
		str.replace(n, find.length(), dest);
		n = str.find(find, n + find.length());
	}
}

string convert_html(const string& str)
{
	string s(str);
	_replace_string(s, "&", "&amp;");
	_replace_string(s, "<", "&lt;");
	_replace_string(s, ">", "&gtl");
	_replace_string(s, "\"", "&quot;");
	_replace_string(s, " ", "&nbsp;");

	return s;
}

wstring convert_html(const wstring& str)
{
	wstring s(str);
	_replace_string(s, L"&", L"&amp;");
	_replace_string(s, L"<", L"&lt;");
	_replace_string(s, L">", L"&gtl");
	_replace_string(s, L"\"", L"&quot;");
	_replace_string(s, L" ", L"&nbsp;");

	return s;
}
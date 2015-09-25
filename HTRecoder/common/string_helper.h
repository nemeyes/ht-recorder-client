#ifndef _STRING_HELPER_H
#define _STRING_HELPER_H
#include <string>
#include <vector>
#include <set>
using namespace std;

void _sprintf_string(string &str, const char *lpFormat, ...);
void _wsprintf_string(wstring &str, const wchar_t *lpFormat, ...);
string	WStringToUTF8(const wchar_t *lpStr);
wstring UTF8ToWString(const char *lpStr);
wstring	AStringToWString(const char *lpStr);
wstring	AStringToWString(const string& str);
string WStringToAString(const wchar_t *lpStr);
string WStringToAString(const wstring& str);

vector<string> MakeArrayStringA(const char *src, const char split);
vector<wstring> MakeArrayStringW(const wchar_t *src, const wchar_t split);
set<string> MakeSetStringA(const char *src, const char split);
set<wstring> MakeSetStringW(const wchar_t *src, const wchar_t split);

void _replace_string(string& str, const string& find, const string& dest);
void _replace_string(wstring& str, const wstring& find, const wstring& dest);

string convert_html(const string& str);
wstring convert_html(const wstring& str);

#define	__WTOA		WStringToAString
#define	__ATOW		AStringToWString
#define __WTOUTF8	WStringToUTF8
#define	__UTF8TOW	UTF8ToWString

#endif // _STRING_HELPER_H
#pragma once

class StringUtil
{
public:
	static BOOL convert_wide2multibyte(WCHAR *source, char **destination)
	{
		UINT32 len = WideCharToMultiByte(CP_ACP, 0, source, wcslen(source), NULL, NULL, NULL, NULL);
		(*destination) = new char[NULL, len + 1];
		::ZeroMemory((*destination), (len + 1)*sizeof(char));
		WideCharToMultiByte(CP_ACP, 0, source, -1, (*destination), len, NULL, NULL);
		return TRUE;
	}

	static BOOL convert_multibyte2wide(char *source, WCHAR **destination)
	{
		UINT32 len = MultiByteToWideChar(CP_ACP, 0, source, strlen(source), NULL, NULL);
		(*destination) = SysAllocStringLen(NULL, len + 1);
		::ZeroMemory((*destination), (len + 1)*sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, source, -1, (*destination), len);
		return TRUE;
	}
};
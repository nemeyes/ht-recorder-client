#pragma once
#ifndef _AFX
#include <atlstr.h>
#endif

#pragma comment( lib, "rpcrt4.lib" )

class GuidGenerator
{
public:
#ifndef _AFX
	static ATL::CStringW generate_widechar_string_guid(VOID)
	{
		GUID guidGen;
		CStringW strGUID;
		CoCreateGuid(&guidGen);
		RPC_WSTR guid;
		UuidToStringW(&guidGen, &guid);
		strGUID.Format(_T("urn:uuid:%s"), guid);
		RpcStringFreeW(&guid);
		return strGUID;
	}

	static ATL::CStringA generate_multibyte_string_guid(VOID)
	{
		GUID guidGen;
		CStringA strGUID;
		CoCreateGuid(&guidGen);
		RPC_CSTR guid;
		UuidToStringA(&guidGen, &guid);
		strGUID.Format("urn:uuid:%s", guid);
		RpcStringFreeA(&guid);
		return strGUID;
	}
#else
	static CStringW generate_widechar_string_guid(VOID)
	{
		GUID guidGen;
		CStringW strGUID;
		CoCreateGuid(&guidGen);
		RPC_WSTR guid;
		UuidToStringW(&guidGen, &guid);
		strGUID.Format(_T("%s"), guid);
		RpcStringFreeW(&guid);
		return strGUID;
	}

	static CStringA generate_multibyte_string_guid(VOID)
	{
		GUID guidGen;
		CStringA strGUID;
		CoCreateGuid(&guidGen);
		RPC_CSTR guid;
		UuidToStringA(&guidGen, &guid);
		strGUID.Format("urn:uuid:%s", guid);
		RpcStringFreeA(&guid);
		return strGUID;
	}
#endif


};
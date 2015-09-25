#include "Common.h"
#include "xml.h"

using namespace std;


CXML::CXML(void)
:m_pDoc(NULL)
,m_pRoot(NULL)
{
	::CoInitialize(NULL);
	HRESULT hr = m_pDoc.CreateInstance(MSXML2::CLSID_DOMDocument);
	if(FAILED(hr)) {
		_com_error er(hr);
		::MessageBox(NULL, er.ErrorMessage(), _T("MSXML3 Error"), MB_ICONERROR);
	}
}

CXML::~CXML(void)
{
	if(m_pRoot) {
		m_pRoot.Release();
		m_pRoot = NULL;
	}
	if(m_pDoc) {
		m_pDoc.Release();
		m_pDoc = NULL;
	}
	::CoUninitialize();
}

BOOL CXML::LoadXML(LPCTSTR lpszFileName)
{
#if 1
	_variant_t vResult;
	
	try
	{
		vResult = m_pDoc->load(_variant_t(lpszFileName));
	}
	catch(...)
	{
		vResult.boolVal = FALSE;
	}

	if(vResult.boolVal)
	{
		if(m_pRoot)
		{
			m_pRoot.Release();
			m_pRoot = NULL;
		}
		m_pRoot = m_pDoc->documentElement;
	}
	else
		return FALSE;

	return TRUE;
#else

	BOOL bRet = FALSE;
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD dwSize = GetFileSize(hFile, NULL);
	if(dwSize > 0)
	{
		DWORD dwRead;
		BYTE *pBuf = new BYTE[dwSize + 1];

		ReadFile(hFile, pBuf, dwSize, &dwRead, NULL);
		pBuf[dwSize] = '\0';
		bRet = LoadXMLFromString((LPCTSTR)pBuf);

		delete [] pBuf;

	}

	CloseHandle(hFile);
	return bRet;
#endif
}

BOOL CXML::LoadXMLFromString(const char *lpszFromString)
{
	_variant_t vResult;
#ifdef UNICODE
	// 그냥 utf8로 온다고 무작정 한다...

	_bstr_t bstrXML;
	wchar_t *pUnicode;
	size_t len = MultiByteToWideChar(CP_UTF8, 0, lpszFromString, -1, NULL, 0);
	if(len > 0)
	{
		pUnicode = new wchar_t[len + 1];
		MultiByteToWideChar(CP_UTF8, 0, lpszFromString, -1, pUnicode, len);
		pUnicode[len] = L'\0';

		bstrXML = _bstr_t(pUnicode);

		delete [] pUnicode;
	}
	else
		return FALSE;

#else
	
	_bstr_t bstrXML(lpszFromString);

#endif

	try
	{
		vResult = m_pDoc->loadXML(bstrXML);
	}
	catch(...)
	{
		vResult.boolVal = FALSE;
	}

	if(vResult.boolVal)
	{
		if(m_pRoot)
		{
			m_pRoot.Release();
			m_pRoot = NULL;
		}
		m_pRoot = m_pDoc->documentElement;
	}
	else
		return FALSE;

	return TRUE;
}

BOOL CXML::LoadXMLFromString(const wchar_t *lpszFromString)
{
	_variant_t vResult;
	_bstr_t bstrXML(lpszFromString);

	try
	{
		vResult = m_pDoc->loadXML(bstrXML);
	}
	catch(...)
	{
		vResult.boolVal = FALSE;
	}

	if(vResult.boolVal)
	{
		if(m_pRoot)
		{
			m_pRoot.Release();
			m_pRoot = NULL;
		}
		m_pRoot = m_pDoc->documentElement;
	}
	else
		return FALSE;

	return TRUE;
}

BOOL CXML::LoadHttpXML(LPCTSTR lpszURL, LPCTSTR lpszData, LPCTSTR lpszMethod)
{
	_variant_t vResult;
	MSXML2::IXMLHTTPRequestPtr pHttp = NULL;
	pHttp.CreateInstance(MSXML2::CLSID_XMLHTTP);

	if(_tcscmp(lpszMethod, _T("POST")) == 0)
	{
		try
		{
			pHttp->open(_bstr_t(lpszMethod), _bstr_t(lpszURL), _variant_t(VARIANT_FALSE));
			pHttp->setRequestHeader(_bstr_t("Content-Type"), _bstr_t("application/x-www-form-urlencoded"));
			pHttp->send(_variant_t(lpszData));

			vResult = m_pDoc->loadXML(pHttp->responseText);
		}
		catch(...)
		{
			vResult.boolVal = FALSE;
		}
	}
	else if(_tcscmp(lpszMethod, _T("GET")) == 0)
	{
		try
		{
			pHttp->open(_bstr_t(lpszMethod), _bstr_t(lpszURL), _variant_t(VARIANT_FALSE));
			pHttp->send();
			
			vResult = m_pDoc->loadXML(pHttp->responseText);
		}
		catch(...)
		{
			vResult.boolVal = FALSE;
		}
	}
	else
		vResult.boolVal = FALSE;

	pHttp.Release();

	if(vResult.boolVal)
	{
		if(m_pRoot)
		{
			m_pRoot.Release();
			m_pRoot = NULL;
		}
		m_pRoot = m_pDoc->documentElement;
	}

	return vResult.boolVal;
}

void CXML::Clear()
{
	if(m_pRoot)
	{
		m_pRoot.Release();
		m_pRoot = NULL;
		
		m_pDoc->loadXML(_bstr_t(""));
		m_pRoot = m_pDoc->documentElement;
	}
}

MSXML2::IXMLDOMNodeListPtr CXML::FindNodeList(LPCTSTR lpszNodeName)
{
	MSXML2::IXMLDOMNodeListPtr pList = NULL;
	_bstr_t node(lpszNodeName);

	pList = m_pDoc->selectNodes(node);
	long nLen = pList->Getlength();
	if(nLen > 0)
		return pList;

	pList.Release();
	pList = NULL;

	return NULL;
}

MSXML2::IXMLDOMNodePtr CXML::FindNode(LPCTSTR lpszNodeName)
{
	MSXML2::IXMLDOMNodePtr pNode = NULL;
	_bstr_t node(lpszNodeName);

	pNode = m_pDoc->selectSingleNode(node);

	return pNode;
}

MSXML2::IXMLDOMNodePtr CXML::GetChildNode(MSXML2::IXMLDOMNodePtr pNode, LPCTSTR lpszNodeName, LPCTSTR lpszQuery /*= NULL*/)
{
	if(pNode == NULL)
		return NULL;

	MSXML2::IXMLDOMNodeListPtr pNodeList = NULL;
	MSXML2::IXMLDOMNodePtr pChild = NULL;
	_bstr_t node(lpszNodeName);

	pNodeList = pNode->GetchildNodes();
	if(pNodeList)
	{
		long count = pNodeList->Getlength();
		for(long i=0; i<count; i++)
		{
			pChild = pNodeList->Getitem(i);
			if(node == pChild->GetbaseName())
			{
				if(lpszQuery)
				{
					CString	strQueryName(lpszQuery);
					CString	strQueryValue;

					int nFind = strQueryName.Find(_T("="));
					if(nFind >= 0)
					{
						strQueryValue	= strQueryName.Mid(nFind + 1);
						strQueryName	= strQueryName.Left(nFind);
					}

					MSXML2::IXMLDOMNamedNodeMapPtr pMap = pChild->Getattributes();
					if(pMap)
					{
						long attCount = pMap->Getlength();
						MSXML2::IXMLDOMNodePtr pAtt;
						for(long att=0; att<attCount; att++)
						{
							pAtt = pMap->Getitem(att);
							if(_bstr_t((LPCTSTR)strQueryName) == pAtt->GetbaseName() && _bstr_t((LPCTSTR)strQueryValue) == pAtt->Gettext())
				                                return pChild;
		                                }
				        }
			        }
				else
					return pChild;
			}
		}
		pNodeList.Release();
	}

	return NULL;
}

CString CXML::GetChildNodeText(MSXML2::IXMLDOMNodePtr pNode, LPCTSTR lpszNodeName, LPCTSTR lpszQuery /*= NULL*/)
{
	if(pNode == NULL)
		return _T("");

	CString strText;
	MSXML2::IXMLDOMNodeListPtr pNodeList = NULL;
	MSXML2::IXMLDOMNodePtr pChild = NULL;
	_bstr_t node(lpszNodeName);

	pNodeList = pNode->GetchildNodes();
	if(pNodeList)
	{
		long count = pNodeList->Getlength();
		for(long i=0; i<count; i++)
		{
			pChild = pNodeList->Getitem(i);
			if(node == pChild->GetbaseName())
			{
				if(lpszQuery)
				{
					CString	strQueryName(lpszQuery);
					CString	strQueryValue;

					int nFind = strQueryName.Find(_T("="));
					if(nFind >= 0)
					{
						strQueryValue	= strQueryName.Mid(nFind + 1);
						strQueryName	= strQueryName.Left(nFind);
					}

					MSXML2::IXMLDOMNamedNodeMapPtr pMap = pChild->Getattributes();
					if(pMap)
					{
						long attCount = pMap->Getlength();
						MSXML2::IXMLDOMNodePtr pAtt;
						for(long att=0; att<attCount; att++)
						{
							pAtt = pMap->Getitem(att);
							if(_bstr_t((LPCTSTR)strQueryName) == pAtt->GetbaseName() && _bstr_t((LPCTSTR)strQueryValue) == pAtt->Gettext())
							{
								strText = (LPCTSTR)pChild->Gettext();
								return strText;
							}
						}
					}
				}
				else
				{
					strText = (LPCTSTR)pChild->Gettext();
					return strText;
				}
			}
		}
		pNodeList.Release();
	}

	return strText;
}

_bstr_t CXML::GetXML()
{
	if(m_pDoc)
		return m_pDoc->xml;

	return _bstr_t("");
}

//MSXML2::IXMLDOMElementPtr CXML::CreateElement(LPCTSTR tagName)
//{
//	_bstr_t name(tagName);
//
//	return m_pDoc->createElement(name);
//}
		
//HRESULT CXML::SaveXML(LPCTSTR lpszFileName, MSXML2::IXMLDOMNodePtr pChild)
//{
//	HRESULT hr;
//
//	m_pDoc->preserveWhiteSpace = VARIANT_TRUE;
//	_bstr_t PIText(L"version=\"1.0\" encoding=\"utf-16\"");
//	MSXML2::IXMLDOMProcessingInstructionPtr pPI = m_pDoc->createProcessingInstruction(_bstr_t("xml"), PIText);
//		
//	m_pDoc->appendChild(pPI);
//	m_pDoc->appendChild(pChild);
//
//	_variant_t destination(lpszFileName);
//	hr = m_pDoc->save(destination);
//	return hr;
//}

CString CXML::GetText(MSXML2::IXMLDOMNodePtr pNode)
{
	USES_CONVERSION;

	if(pNode == NULL)
		return _T("");

	CString strValue;
	_bstr_t bstrValue;

	try
	{
		bstrValue = pNode->Gettext();
		strValue = W2T(bstrValue);
	}
	catch(...)
	{
		strValue = _T("");
	}

	return strValue;
}


CString CXML::GetNodeName(MSXML2::IXMLDOMNodePtr pNode)
{
	USES_CONVERSION;

	if(pNode == NULL)
		return _T("");

	CString strValue;
	_bstr_t bstrValue;

	try
	{
		bstrValue = pNode->GetnodeName();
		strValue = W2T(bstrValue);
	}
	catch(...)
	{
		strValue = _T("");
	}

	return strValue;
}


CString CXML::GetAttributeValue(MSXML2::IXMLDOMNodePtr pNode, LPWSTR lpwName)
{
	USES_CONVERSION;

	if(pNode == NULL)
		return _T("");

	CString strValue;
	_bstr_t bstrName(lpwName);
	try
	{
		strValue = W2T( pNode->attributes->getNamedItem(bstrName)->text );
	}
	catch(...)
	{
		strValue = _T("");
	}

	return strValue;
}

std::string CXML::GetAttributeValueSTD(MSXML2::IXMLDOMNodePtr pNode, LPWSTR lpwName)
{
	USES_CONVERSION_EX;

	if(pNode == NULL)
		return "";

	string strValue;
	_bstr_t bstrName(lpwName);
	try
	{
		strValue = W2A_EX( pNode->attributes->getNamedItem(bstrName)->text, _ATL_SAFE_ALLOCA_DEF_THRESHOLD );
	}
	catch(...)
	{
		strValue = "";
	}

	return strValue;
}

CString CXML::ConvertSymbol(CString str)
{
	// HTML 형식의 코드로 변경함. 4개만
	// '<' : '&lt;'
	// '>' : '&gt;'
	// '"' : '&quot;'
	// '&' : '&amp;'

	str.Replace(_T("&"), _T("&amp;"));
	str.Replace(_T("\""), _T("&quot;"));
	str.Replace(_T("<"), _T("&lt;"));
	str.Replace(_T(">"), _T("&gt;"));

	return str;
}

#pragma once
#import <msxml4.dll> named_guids
#include <string>

class CXML
{
public:
	CXML(void);
	virtual ~CXML(void);

	BOOL LoadXML(LPCTSTR lpszFileName);
	BOOL LoadXMLFromString(const wchar_t *lpszFromString);	// UTF-16
	BOOL LoadXMLFromString(const char *lpszFromString);	// UTF-8
	BOOL LoadHttpXML(LPCTSTR lpszURL, LPCTSTR lpszData, LPCTSTR lpszMethod);
	void Clear();

	MSXML2::IXMLDOMElementPtr GetRootElementPtr() const { return m_pRoot; }
	MSXML2::IXMLDOMNodeListPtr FindNodeList(LPCTSTR lpszNodeName);
	MSXML2::IXMLDOMNodePtr FindNode(LPCTSTR lpszNodeName);
	_bstr_t GetXML();

	//HRESULT SaveXML(LPCTSTR lpszFileName, MSXML2::IXMLDOMNodePtr pChild);
	//MSXML2::IXMLDOMElementPtr CreateElement(LPCTSTR tagName);

	static CString GetAttributeValue(MSXML2::IXMLDOMNodePtr pNode, LPWSTR lpwName);
	static std::string GetAttributeValueSTD(MSXML2::IXMLDOMNodePtr pNode, LPWSTR lpwName);
	static CString GetText(MSXML2::IXMLDOMNodePtr pNode);
	static CString GetNodeName(MSXML2::IXMLDOMNodePtr pNode);

	// Query 사용법 : name=value ( value 에 " 는 넣지 않는다, text, 숫자 동일함 )
	static MSXML2::IXMLDOMNodePtr GetChildNode(MSXML2::IXMLDOMNodePtr pNode, LPCTSTR lpszNodeName, LPCTSTR lpszQuery = NULL);
	static CString GetChildNodeText(MSXML2::IXMLDOMNodePtr pNode, LPCTSTR lpszNodeName, LPCTSTR lpszQuery = NULL);

	/*
		&	-> &amp;
		<	-> &lt;
		>	-> &gt;
		"	-> &quot;
	*/
	static CString ConvertSymbol(CString str);

protected:
	MSXML2::IXMLDOMDocumentPtr	m_pDoc;
	MSXML2::IXMLDOMElementPtr	m_pRoot;
};

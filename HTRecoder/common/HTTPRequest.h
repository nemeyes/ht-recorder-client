#pragma once
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <winsock.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <schannel.h>

#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>

#include <string>
using namespace std;

#include <string>
#include <vector>
#include <HTRecorderDLL.h>
#include "StringLineParser.h"
#include "Security.h"

#ifndef WM_MYPROGRESSINC
#define WM_MYPROGRESSINC	WM_USER + 30
#define WM_MYPROGRESSSUC	WM_MYPROGRESSINC + 1
#endif

using namespace std;

typedef struct _HTTP_DOWNLOAD_FILE
{
	char	*pBuffer;
	size_t	nBufferLength;
	LPARAM	lParam;

} HTTP_DOWNLOAD_FILE;

typedef void (__stdcall CallbackHttpDownload)(HTTP_DOWNLOAD_FILE*);
typedef CallbackHttpDownload *CallbackHttpDwonloadFunPtr;

class CHTTPRequest
{
public:
	enum enHTTP_METHOD {
		HTTP_GET,
		HTTP_POST,
		HTTP_PUT,
		HTTP_DELETE
	};
	enum enHTTP_TYPE { HTTP, HTTPS };
	enum enHTTP_CHAR {
		CHAR_UTF8,
		CHAR_UTF16,
		CHAR_EUCKR
	};

	CHTTPRequest(void);
	virtual ~CHTTPRequest(void);

	void	SetProxy(const char *lpAddress, UINT nPort, const char *lpUser, const char *lpPassword);
	void	SetProxy(const wchar_t *lpAddress, UINT nPort, const wchar_t *lpUser, const wchar_t *lpPassword);

	BOOL	SendRequest(const char *lpHost, const char *lpUrl, UINT nPort, 
		LPVOID lpFrmData = NULL, size_t dwFrmData = 0,
		enHTTP_METHOD method = HTTP_GET,
		const char *lpUser = NULL, const char *lpPassword = NULL,
		BOOL bSSL = FALSE, DWORD dwTimeout = 0);

	BOOL	SendRequest(const wchar_t *lpHost, const wchar_t *lpUrl, UINT nPort, 
		LPVOID lpFrmData = NULL, size_t dwFrmData = 0,
		enHTTP_METHOD method = HTTP_GET,
		const wchar_t *lpUser = NULL, const wchar_t *lpPassword = NULL,
		BOOL bSSL = FALSE, DWORD dwTimeout = 0);

	const char* GetHeader();
	const wchar_t* GetHeaderW();
	const char* GetResponse();
	const wchar_t* GetResponseW();
	size_t GetResponseSize() const;
	size_t GetContentLength() const;

	int GetResponseCode() const;

	BOOL DownloadFile(HWND hWnd, const char *lpHost, const char *lpUrl, CallbackHttpDwonloadFunPtr pCallback,
		UINT nPort = 80,
		BOOL bHeaderOnly = FALSE,
		LPARAM lParam = NULL,
		const char *lpUser = NULL, const char *lpPassword = NULL,
		BOOL bSSL = FALSE, DWORD dwTimeout = 0);

	BOOL DownloadFile(HWND hWnd, const wchar_t *lpHost, const wchar_t *lpUrl, CallbackHttpDwonloadFunPtr pCallback,
		UINT nPort = 80,
		BOOL bHeaderOnly = FALSE,
		LPARAM lParam = NULL,
		const wchar_t *lpUser = NULL, const wchar_t *lpPassword = NULL,
		BOOL bSSL = FALSE, DWORD dwTimeout = 0);

	//////////////////////////////////////////////////////////////////////
	// Upload
	/*
	구조체가 일반 변수일 경우...
	isValue : TRUE
	strName : 변수명
	strValue: 변수값

	구조체가 파일인 경우...
	isValue : FALSE
	strName : 변수명
	strValue: 로컬 파일 경로...
	*/
	typedef struct _HTTP_UPLOAD_DATA
	{
		string	sBoundary;
		string	sName;
		string	sValue;
		BOOL	isValue;

	} HTTP_UPLOAD_DATA;

	void InitUpload();
	BOOL AddUploadData(LPCTSTR lpName, LPCTSTR lpValue, BOOL bValue = TRUE);
	BOOL UploadProc(HWND hWnd, LPCTSTR lpHost, LPCTSTR lpUrl, UINT nPort, 
		LPCTSTR lpUser = NULL, LPCTSTR lpPassword = NULL,
		BOOL bSSL = FALSE);
	void SetItemData(void *pParam) { m_pParam = pParam; };
	void* GetItemData() { return m_pParam; }

	// URL 파서
	static BOOL Parse_URL(const char *pURL, string &sAddress, unsigned short &nPort, string &sPage, string &sQuery, BOOL &bSSL, BOOL &bIPv6);
	static BOOL Parse_URL(const wchar_t *pURL, wstring &sAddress, unsigned short &nPort, wstring &sPage, wstring &sQuery, BOOL &bSSL, BOOL &bIPv6);
	static BOOL GetAddressString(const char *pURL, string &sAddress);
	static BOOL GetAddressString(const wchar_t *pURL, wstring &sAddress);
	static std::string URLEncoding(const char* pStr);
	static const char _C2HEX(const char src);
	static const char _HEX2C(const char hex_up, const char hex_low);

	void	Stop();

protected:
	BOOL	Connect(const char *lpAddress, const short nPort, BOOL bSSL = FALSE);
	void	Disconnect();
	int		SendData(char *pBuffer, size_t nLength);
	int		SendDataOV(char *pBuffer, size_t nLength);
	int		SendDataBL(char *pBuffer, size_t nLength);
	int		(CHTTPRequest::*m_pSendFunc)(char *pBuffer, size_t nLength);

	int		RecvData();
	int		ProcessRecv(char *pRecvData, size_t nLength);

	int		ProcessContents();
	int		ProcessChunkedData();
	void	MakeAuthField(const char *lpUser, const char *lpPass);
	int		SendProxyConnect(const char* pAddress, int nPort);

	CBuffer	m_recvBuffer;
	CBuffer m_sendBuffer;
	CBuffer m_secBuffer;	// Send하기전 Security용 버퍼
	
	CBuffer m_headBuffer;
	CBuffer m_dataBuffer;
	wstring m_whead;
	wstring m_wdata;

	int		m_nResultCode;

	CStringLineParser m_parser;
	
	string	m_sAuth;
	string	m_sAgent;
	string	m_sAddHeaders;

	BOOL	m_bIPv6;
	SOCKET	m_hSocket;
	WSADATA	m_wsaData;
	BOOL	m_bSSL;
	DWORD	m_dwTimeout;

	int		m_nSendSize;

	BOOL	m_bReceivedHeader;
	size_t	m_nContentLength;
	BOOL	m_bConnectionClose;

	BOOL	m_bChunked;		// Chunked encoding 방식인가??

	enHTTP_CHAR	m_charset;

	CSecurity	m_ssl;

	// download file
	HWND m_hwndParent;
	BOOL m_bHeaderOnly;
	HTTP_DOWNLOAD_FILE m_http_down;
	CallbackHttpDwonloadFunPtr m_pCallback;
	size_t	m_nRecvDataSize;

	// upload
	string			m_sBoundary;
	unsigned long	m_lTotalFileSize;
	void*			m_pParam;
	vector<HTTP_UPLOAD_DATA> m_vData;

	// proxy
	string	m_sProxyAddress;
	size_t	m_nProxyPort;
	string	m_sProxyAuth;
};

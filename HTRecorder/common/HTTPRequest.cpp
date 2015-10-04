#include "Common.h"
#include "httprequest.h"
#include "StringLineParser.h"
#include "string_helper.h"
#include <AtlConv.h>
#include <atlenc.h>
#include <time.h>

static const char HTTP_METHOD_STR[][12] = {
	"GET", "POST", "PUT", "DELETE"
};

static const char szContentLength[] = "Content-Length:";
static const char szContentType[] = "Content-Type:";
static const char szCharset[] = "charset=";
static const char szTransferEncoding[] = "Transfer-Encoding:";
static const char szConnection[] = "Connection:";

BOOL CHTTPRequest::GetAddressString(const char *pURL, string &sAddress)
{
	string addr, page, query;
	BOOL ssl, ipv6;
	unsigned short port;

	if(!Parse_URL(pURL, addr, port, page, query, ssl, ipv6))
		return FALSE;

	if(ipv6)
		_sprintf_string(sAddress, "[%s]:%d", addr.c_str(), port);
	else
	        _sprintf_string(sAddress, "%s:%d", addr.c_str(), port);

	return TRUE;
}

BOOL CHTTPRequest::GetAddressString(const wchar_t *pURL, wstring &sAddress)
{
	string sAddr = WStringToAString(pURL);
	string sRet;

	if(!GetAddressString(sAddr.c_str(), sRet))
		return FALSE;

	sAddress = AStringToWString(sRet.c_str());

	return TRUE;
}

BOOL CHTTPRequest::Parse_URL(const char *pURL, string &sAddress, unsigned short &nPort, string &sPage, string &sQuery, BOOL &bSSL, BOOL &bIPv6)
{
	if(!pURL)
		return FALSE;

	CStringLineParser parser(pURL);
	BOOL bRTSP = FALSE;

	if(parser.NumEqualIgnoreCase("http://"))
	{
		bSSL = FALSE;
		parser.ConsumeLength( strlen("http://") );
	}
	else if(parser.NumEqualIgnoreCase("https://"))
	{
		bSSL = TRUE;
		parser.ConsumeLength( strlen("https://") );
	}
	else
		bSSL = FALSE;

	// rtsp도 추가
	if(parser.NumEqualIgnoreCase("rtsp://"))
	{
		parser.ConsumeLength( strlen("rtsp://") );
		bRTSP = TRUE;
	}

	// 첫번째 주소형식이 v6형식 표기인지 체크..
	if(*parser.GetPtr() == '[')	// IP v6 인경우 url 형식 [주소]
	{
		parser.ConsumeLength(1);	// skip '['
		parser.ConsumeUntil(']');

		sAddress = parser.GetString();

		parser.ConsumeLength(1);	// skip ']'

		bIPv6 = TRUE;
	}
	else
		bIPv6 = FALSE;

	// 포트 정보가 있는지 확인하자..
	nPort = ( bSSL == TRUE ) ? 443 : 80;	// 웹서버 포트겠죠..
	if(bRTSP)
		nPort = 554;

	char *pPort = (char*)strstr(parser.GetPtr(), ":");
	if(pPort)
	{
		parser.ConsumeUntil(':');
		TRACE("ipv4: %s\n", parser.GetString());
		if(sAddress.empty())
			sAddress = parser.GetString();

		parser.ConsumeLength(1);

		nPort = parser.ConsumeInteger();
	}

	parser.ConsumeUntil('/');	// url page 있는곳까지...

	if(sAddress.empty())	// 포트정보가 url에 없었다면... 여기에 주소가...
		sAddress = parser.GetString();

	parser.ConsumeUntil('?');	// ? 가 있는곳까지...
	sPage = URLEncoding( parser.GetString() );

	if(*parser.GetPtr() == '?')
	{
		parser.ConsumeLength(1);
		sQuery = parser.GetPtr();
	}

	if(sAddress.empty() || sPage.empty())
		return FALSE;

	return TRUE;	
}

BOOL CHTTPRequest::Parse_URL(const wchar_t *pURL, wstring &sAddress, unsigned short &nPort, wstring &sPage, wstring &sQuery, BOOL &bSSL, BOOL &bIPv6)
{
	if(!pURL)
		return FALSE;

	string sAddr = WStringToAString(pURL);
	string address, page, query;

	BOOL bRet = Parse_URL(sAddr.c_str(), address, nPort, page, query, bSSL, bIPv6);

	sAddress	= AStringToWString( address.c_str() );
	sPage		= AStringToWString( page.c_str() );
	sQuery		= AStringToWString( query.c_str() );

	return bRet;
}

std::string CHTTPRequest::URLEncoding(const char* pStr)
{
	if(pStr == NULL)
		return "";

	string input(pStr);
	string output;
	string::const_iterator itr = input.begin();
	while(itr != input.end())
	{
		const string::value_type c = *itr;
		if(isascii(c) && isalnum(c))
			output += c;
		else if(c == '@' || c == '.' || c == '/' || c == '\\' || c == '-' || c == '_' || c == ':')
			output += c;
		else
		{
			output += "%";
			output += _C2HEX( c >> 4 );
			output += _C2HEX( c );
		}
		itr++;
	}

	return output;
}

const char CHTTPRequest::_C2HEX(const char src)
{
	return "0123456789abcdef"[ 0x0f & src ];
}

const char CHTTPRequest::_HEX2C(const char hex_up, const char hex_low)
{
	char digit;

	digit = 16 * (hex_up >= 'A') ? (hex_up & 0xdf) - 'A' + 10 : hex_up - '0';
	digit += (hex_low >= 'A') ? (hex_low & 0xdf) - 'A' + 10 : hex_low - '0';

	return digit;
}
//
//char *qURLdecode(char *str) 
//{
//        int i, j;
//
//        if(!str) return NULL;
//
//        for(i = j = 0; str[j]; i++, j++) 
//        {
//                switch(str[j]) 
//                {
//                case '+':
//                        str[i] = ' ';
//                        break;
//
//                case '%':
//                        str[i] = _x2c(str[j + 1], str[j + 2]);
//                        j += 2;
//                        break;
//
//                default:
//                        str[i] = str[j];
//                        break;
//                }
//        }
//        str[i]='';
//
//        return str;
//}

CHTTPRequest::CHTTPRequest(void)
:m_hSocket(INVALID_SOCKET)
,m_sendBuffer(4096)
,m_recvBuffer(8192)
{
	WSAStartup(MAKEWORD(2, 2), &m_wsaData);
	m_dwTimeout		= 30000;	// 30초
	m_sAgent		= "EON-HTTP Client";
	m_pSendFunc		= &CHTTPRequest::SendDataBL;
	m_sAddHeaders	= "Content-Type: application/x-www-form-urlencoded\r\n";
	m_charset		= CHTTPRequest::CHAR_UTF8;
	m_bSSL			= FALSE;
	m_bIPv6			= FALSE;

	m_hwndParent	= NULL;
	m_bHeaderOnly	= FALSE;
	m_pCallback		= NULL;

	time_t now = time(NULL);
	tm t;
#if _MSC_VER >= 1500
	localtime_s( &t, &now );
#else
	t = *localtime( &now );
#endif

	_sprintf_string(m_sBoundary, "------------------------%04d%02d%02d%02d%02d%02da",
		t.tm_year + 1900,
		t.tm_mon + 1,
		t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec);
}

CHTTPRequest::~CHTTPRequest(void)
{
	Disconnect();
	WSACleanup();
}

BOOL CHTTPRequest::Connect(const char *lpAddress, const short nPort, BOOL bSSL /*= FALSE*/)
{
	Disconnect();

	struct addrinfo hints, *res = NULL;
	//struct addrinfo conn_addr;
	int rc;
	char szRemoteAddress[128];
	char szRemotePort[8];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family		= AF_UNSPEC;
	hints.ai_socktype	= SOCK_STREAM;
	hints.ai_protocol	= IPPROTO_TCP;

	if(m_sProxyAddress.empty())
	{
	strcpy(szRemoteAddress, lpAddress);
	sprintf(szRemotePort, "%d", nPort);
	}
	else
	{
		strcpy(szRemoteAddress, m_sProxyAddress.c_str());
		sprintf(szRemotePort, "%d", m_nProxyPort);
	}

	rc = getaddrinfo(szRemoteAddress, szRemotePort, &hints, &res);
	if(rc != NO_ERROR)
	{
		// 이름 해석 실패
		TRACE("getaddrinfo 에러 %d", WSAGetLastError());
		return FALSE;
	}
	
	m_hSocket = WSASocket(res->ai_family, res->ai_socktype, res->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(m_hSocket == INVALID_SOCKET)
	{
		if(res) freeaddrinfo(res);
		return FALSE;
	}

	if(res->ai_family == AF_INET6)
		m_bIPv6 = TRUE;

	struct linger ling;
	ling.l_onoff	= 1;
	ling.l_linger	= 0;
	setsockopt(m_hSocket, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling)); 

	if(m_dwTimeout == 0)
	{
		rc = WSAConnect(m_hSocket, res->ai_addr, (int)res->ai_addrlen, NULL, NULL, NULL, NULL);
		if(rc == SOCKET_ERROR)
		{
			if(res) freeaddrinfo(res);
			Disconnect();
			return FALSE;
		}
	}
	else
	{
		u_long nonBlock = 1;
		ioctlsocket(m_hSocket, FIONBIO, &nonBlock);
		rc = connect(m_hSocket, res->ai_addr, (int)res->ai_addrlen);

		struct timeval timevalue;
		fd_set fdWrite, fdRead;

		FD_ZERO(&fdWrite);
		FD_SET(m_hSocket, &fdWrite);
		fdRead = fdWrite;
		timevalue.tv_sec = m_dwTimeout / 1000;
		timevalue.tv_usec = 0;

		if(select(m_hSocket + 1, &fdRead, &fdWrite, NULL, &timevalue) == 0)	// 타임아웃
		{
			if(res) freeaddrinfo(res);
			Disconnect();
			return FALSE;
		}

		if(FD_ISSET(m_hSocket, &fdWrite) || FD_ISSET(m_hSocket, &fdRead))	// 연결시도중 연결종료: closesocket호출
		{
			int error;
			int len = sizeof(error);
			if(getsockopt(m_hSocket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
			{
				if(res) freeaddrinfo(res);
				Disconnect();
				return FALSE;
			}
		}

		nonBlock = 0;
		ioctlsocket(m_hSocket, FIONBIO, &nonBlock);

		int opt = m_dwTimeout;
		setsockopt(m_hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&opt, sizeof(opt));
		setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&opt, sizeof(opt));
	}

	// proxy
		if(!m_sProxyAddress.empty())
		{
		if(SendProxyConnect(lpAddress, nPort) != NO_ERROR)
			{
			TRACE("SendProxyConnect 에러 [%s:%d]\n", lpAddress, nPort);
			if(res) freeaddrinfo(res);
				Disconnect();
				return FALSE;
			}
		}

	freeaddrinfo(res);

	if(bSSL)
	{
		if(m_ssl.Initialize(m_hSocket, lpAddress))
			m_bSSL = bSSL;
		else
		{
			Disconnect();
			return FALSE;
		}
	}
	else
		m_bSSL = bSSL;

	return TRUE;
}

void CHTTPRequest::Disconnect()
{
	if(m_bSSL)
	{
		m_ssl.Destroy();
		m_bSSL = FALSE;
	}

	if(m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}
}

int CHTTPRequest::SendProxyConnect(const char* pAddress, int nPort)
{
	char szRemoteAddress[MAX_PATH];
	char szRemotePort[16];
	struct addrinfo hints, *res = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family		= AF_UNSPEC;
	hints.ai_socktype	= SOCK_STREAM;
	hints.ai_protocol	= IPPROTO_TCP;

	strcpy(szRemoteAddress, pAddress);
	sprintf(szRemotePort, "%d", nPort);

	int rc = getaddrinfo(szRemoteAddress, szRemotePort, &hints, &res);
	if(rc != NO_ERROR)
		return -1;

	char szAddress[MAX_PATH];
	DWORD dwAddrSize = MAX_PATH;
	if(WSAAddressToStringA(res->ai_addr, res->ai_addrlen, NULL, szAddress, &dwAddrSize) == SOCKET_ERROR)
		return -1;

	char szHost[MAX_PATH];
	if(res->ai_family == AF_INET6)
	{
		if(strstr(szHost, ":"))
			sprintf_s(szHost, MAX_PATH, "[%s]", pAddress);
		else
			strcpy_s(szHost, MAX_PATH, pAddress);
	}
	else
		strcpy_s(szHost, MAX_PATH, pAddress);

	freeaddrinfo(res);

	m_sendBuffer.Assign(NULL, 0);
	m_nSendSize = sprintf(m_sendBuffer.GetPtr(),
		"CONNECT %s HTTP/1.0\r\n" \
		"User-Agent: %s\r\n" \
		"Proxy-Connection: Keep-Alive\r\n" \
		"Content-Length: 0\r\n" \
		"Host: %s\r\n" \
		"Pragma: no-cache\r\n" \
		"%s\r\n",

		szAddress,
		m_sAgent.c_str(),
		szHost,
		m_sProxyAuth.c_str());
	m_sendBuffer.SetSize( m_nSendSize );

	if(SendData( m_sendBuffer.GetPtr(), m_sendBuffer.GetSize() ) != NO_ERROR)
		return -1;

	BOOL bHeaderOnly = m_bHeaderOnly;
	m_bHeaderOnly = TRUE;
	if(RecvData() != NO_ERROR)
	{
		m_bHeaderOnly = bHeaderOnly;
		return -1;
	}

	m_bHeaderOnly = bHeaderOnly;

	if(m_nResultCode != 200)
		return -1;

	return NO_ERROR;
}

void CHTTPRequest::SetProxy(const char *lpAddress, UINT nPort, const char *lpUser, const char *lpPassword)
{
	m_sProxyAddress	= lpAddress;
	m_nProxyPort	= nPort;

	if(strlen(lpUser) > 0 && strlen(lpPassword) > 0)
	{
		char szAuth[MAX_PATH], szEncodeAuth[MAX_PATH];
		sprintf_s(szAuth, "%s:%s", lpUser, lpPassword);
		int len = MAX_PATH;

		if(Base64Encode((BYTE*)szAuth, strlen(szAuth), szEncodeAuth, &len))
		{
			szEncodeAuth[len] = '\0';
			_sprintf_string(m_sProxyAuth, "Proxy-Authorization: Basic %s\r\n", szEncodeAuth);
		}
	}
	else
		m_sProxyAuth.clear();
}

void CHTTPRequest::SetProxy(const wchar_t *lpAddress, UINT nPort, const wchar_t *lpUser, const wchar_t *lpPassword)
{
	string addr = WStringToAString(lpAddress);
	string user = WStringToAString(lpUser);
	string pass = WStringToAString(lpPassword);

	SetProxy(addr.c_str(), nPort, user.c_str(), pass.c_str());
}

BOOL CHTTPRequest::SendRequest(const char *lpHost, const char *lpUrl, UINT nPort, 
		LPVOID lpFrmData /*= NULL*/, size_t dwFrmData /*= 0*/,
		enHTTP_METHOD method /*= HTTP_GET*/,
		const char *lpUser /*= NULL*/, const char *lpPassword /*= NULL*/,
		BOOL bSSL /*= FALSE*/, DWORD dwTimeout /*= 0*/)
{
	if(dwTimeout != 0)
		m_dwTimeout = dwTimeout;

	if(!Connect(lpHost, nPort, bSSL))
		return FALSE;

	MakeAuthField(lpUser, lpPassword);

	m_nResultCode		= 0;

	char szHost[MAX_PATH];
	if(m_bIPv6)
	{
		if(strstr(lpHost, ":"))
			sprintf_s(szHost, sizeof(szHost)/sizeof(szHost[0]), "[%s]", lpHost);
		else
			strcpy_s(szHost, sizeof(szHost)/sizeof(szHost[0]), lpHost);
	}
	else
		strcpy_s(szHost, sizeof(szHost)/sizeof(szHost[0]), lpHost);

	__try
	{
		m_sendBuffer.Assign(NULL, 0);
		m_nSendSize = sprintf(m_sendBuffer.GetPtr(),
			"%s %s HTTP/1.1\r\n" \
			"Accept: */*\r\n" \
			"User-Agent: %s\r\n" \
			"Host: %s\r\n" \
			"%sConnection: Keep-Alive\r\n%s",

			HTTP_METHOD_STR[method],
			lpUrl,
			m_sAgent.c_str(),
			szHost,
			m_sAuth.c_str(),
			m_sAddHeaders.c_str()
			);
		m_sendBuffer.SetSize( m_nSendSize );


		if(lpFrmData && dwFrmData > 0)
		{
			int len = sprintf(m_sendBuffer.GetPtr() + m_nSendSize, "Content-Length: %lu\r\n\r\n", dwFrmData);
			m_nSendSize += len;

			m_sendBuffer.SetSize( m_nSendSize );

			m_sendBuffer.Append(lpFrmData, dwFrmData);
		}
		else
		{
			m_sendBuffer.Append("\r\n", 2);
		}

		if(SendData( m_sendBuffer.GetPtr(), m_sendBuffer.GetSize() ) != NO_ERROR)
			return FALSE;


		if(RecvData() != NO_ERROR)
			return FALSE;

	}
	__finally
	{
		Disconnect();
	}

	return TRUE;
}

BOOL CHTTPRequest::SendRequest(const wchar_t *lpHost, const wchar_t *lpUrl, UINT nPort, 
		LPVOID lpFrmData /*= NULL*/, size_t dwFrmData /*= 0*/,
		enHTTP_METHOD method /*= HTTP_GET*/,
		const wchar_t *lpUser /*= NULL*/, const wchar_t *lpPassword /*= NULL*/,
		BOOL bSSL /*= FALSE*/, DWORD dwTimeout /*= 0*/)
{
	string host	= WStringToAString(lpHost);
	string url	= WStringToAString(lpUrl);
	string user	= WStringToAString(lpUser);
	string pass	= WStringToAString(lpPassword);

	return SendRequest(host.c_str(), url.c_str(), nPort, lpFrmData, dwFrmData, method, user.c_str(), pass.c_str(), bSSL, dwTimeout);
}

int CHTTPRequest::SendData(char *pBuffer, size_t nLength)
{
	return (this->*m_pSendFunc)(pBuffer, nLength);
}

int CHTTPRequest::SendDataOV(char *pBuffer, size_t nLength)
{
	return NO_ERROR;
}

int CHTTPRequest::SendDataBL(char *pBuffer, size_t nLength)
{
	int nBytesSent = 0;
	int nTotalBytesSent = 0;
	int nSendBufferLen = (int)nLength;
	char *p = pBuffer;

	if(m_bSSL)
	{
		m_ssl.Encrypt(pBuffer, nLength, &m_secBuffer);
		p = m_secBuffer.GetPtr();
		nSendBufferLen = (int)m_secBuffer.GetSize();
	}

	do
	{
		nBytesSent = send(m_hSocket, p + nTotalBytesSent, nSendBufferLen - nTotalBytesSent, 0);
		if(nBytesSent == SOCKET_ERROR)
			return -1;

		nTotalBytesSent += nBytesSent;

	} while(nTotalBytesSent < nSendBufferLen);

	return NO_ERROR;
}

int CHTTPRequest::RecvData()
{
	int nErr;
	int nRecvLen;
	int nTotalRecvLen = 0;
	CBuffer recvBuf(8192);
	int nRecvBufferSize;

	m_recvBuffer.Assign(NULL, 0);

	int nResult = NO_ERROR;
	BOOL bDone = FALSE;

	m_bChunked			= FALSE;
	m_bReceivedHeader	= FALSE;
	//m_bConnectionClose	= TRUE;
	m_bConnectionClose	= FALSE;
	m_nContentLength	= -1;
	m_nRecvDataSize		= 0;
	m_dataBuffer.Assign(NULL, 0);

	char *p = recvBuf.GetPtr();

	while(!bDone)
	{
		nRecvBufferSize = (int)(recvBuf.GetCapacity() - recvBuf.GetSize() - 1);
		nRecvLen = recv(m_hSocket, p + recvBuf.GetSize(), nRecvBufferSize, 0);
		if(nRecvLen < 0)
		{
			nErr = GetLastError();
			//TRACE("CHTTPRequest::RecvData 에러 : %d\n", nErr);
			return nErr;
		}
		if(nRecvLen == 0)
		{
			return NO_ERROR;
		}

		recvBuf.SetSize( recvBuf.GetSize() + nRecvLen );

		if(m_bSSL)
		{
			nResult = m_ssl.Decrypt(p, recvBuf.GetSize(), &m_recvBuffer);
			if(nResult == ERROR_MORE_DATA)
			{
				recvBuf.SetReserve( recvBuf.GetSize() * 2 );
				p = recvBuf.GetPtr();
				
				continue;
			}
			else if(nResult == NO_ERROR)
			{
				recvBuf.SetSize(0);
			}
			else if(nResult == SEC_I_CONTEXT_EXPIRED)
			{
				return NO_ERROR;
			}
			else if(nResult != NO_ERROR)
				return -1;
		}
		else
		{
			m_recvBuffer.Append(p, nRecvLen);
			recvBuf.SetSize(0);
		}

		// NO_ERROR: No error, ERROR_MORE_DATA: need continue, ERROR_LOGON_FAILURE: http error (ex: invalid user and password)
		nResult = ProcessRecv(m_recvBuffer.GetPtr(), m_recvBuffer.GetSize());
		if(nResult == NO_ERROR)
			bDone = TRUE;
		else
		{
			// magic : 승언PC에서만 H.264계속 타임아웃나는데...
			Sleep(2);
		}
#if 0
		else if(nResult == ERROR_LOGON_FAILURE)
		{
			return NO_ERROR;
			//return ERROR_LOGON_FAILURE;
		}
#endif
	}

	return nResult;
}

// nLength 받은놈의 총길이...
int CHTTPRequest::ProcessRecv(char *pRecvData, size_t nLength)
{
	int nResult = ERROR_MORE_DATA;// NO_ERROR;
	size_t nContentRecvLen = 0;
	size_t nContentLen = -1;
	BOOL bConnectionClose = FALSE;
	size_t nHeaderLen = 0;
	BOOL bFindContentLength = FALSE;
	size_t nPassLength = 4;	//\r\n\r\n 길이

	if(!m_bReceivedHeader)
	{
		const char *pResponseData = strstr(pRecvData, "\r\n\r\n");
		if(!pResponseData)	// 예외 \r\n이 아닌경우도 있네.. \n으로만 나오는 웹서버도 있다... 쩝..
		{
			pResponseData = strstr(pRecvData, "\n\n");
			if(pResponseData)
			{
			if(pRecvData) nPassLength = 2;
		        }
			else
			{
				// 이런... \n\r\n 이런것도 있어??? 나참...
				pResponseData = strstr(pRecvData, "\n\r\n");
				if(pResponseData)
				{
					if(pRecvData) nPassLength = 3;
				}

			}
		}

		if(pResponseData)
		{
			m_nContentLength = 0;
			m_bReceivedHeader = TRUE;

			// "\r\n\r\n" 길이만큼 통과
			pResponseData += nPassLength;

			nHeaderLen = (int)(pResponseData - pRecvData);

			// 헤더에 넣는다..
			m_headBuffer.Assign(pRecvData, nHeaderLen);

			// 버퍼 삭제
			m_recvBuffer.Erase(0, nHeaderLen);

			// 파서 구성
			m_parser.SetString(m_headBuffer.GetPtr());

			do
			{
				if(m_parser.NumEqualIgnoreCase("HTTP"))
				{
					m_parser.ConsumeUntil(CStringLineParser::sEOLWhitespaceMask);
					m_parser.ConsumeUntil(CStringLineParser::sDigitMask);

					m_nResultCode = m_parser.ConsumeInteger();

					//TRACE("HTTP RESULT: %d\n", m_nResultCode);

				}
				else if(m_parser.NumEqualIgnoreCase(szContentLength))
				{
					m_parser.ConsumeLength(sizeof(szContentLength));
					nContentLen = m_parser.ConsumeInteger();

					m_nContentLength = nContentLen;

					bFindContentLength = TRUE;

					// 다 못받음.. 계속
					//if(nLength < (nHeaderLen + nContentLen))
					//{
					//	return ERROR_MORE_DATA;	// 계속받아야한다..
					//}
				}
				else if(m_parser.NumEqualIgnoreCase(szConnection))
				{
					m_parser.ConsumeLength(sizeof(szConnection));
					m_parser.ConsumeUntil(CStringLineParser::sEOLMask);

					if( !_stricmp(m_parser.GetString(), "close") )
					{
						m_bConnectionClose = TRUE;
						bConnectionClose = TRUE;
					}
					else
					{
						m_bConnectionClose = FALSE;
						bConnectionClose = FALSE;
					}
				}
				else if(m_parser.NumEqualIgnoreCase(szContentType))
				{
					m_parser.ConsumeUntil(CStringLineParser::sEOLMask);
					int nFind = m_parser.FindIgnoreCase( m_parser.GetString(), strlen(m_parser.GetString()),
						szCharset, strlen(szCharset));
					if(nFind > -1)
					{
						CStringLineParser sr(m_parser.GetString());
						sr.ConsumeLength(nFind + strlen(szCharset));
						//TRACE("CHARSET: %s\n", sr.GetPtr());

						if(sr.NumEqualIgnoreCase("utf-8"))
							m_charset = CHAR_UTF8;
						else if(sr.NumEqualIgnoreCase("utf-16"))
							m_charset = CHAR_UTF16;
						else if(sr.NumEqualIgnoreCase("euc-kr"))
							m_charset = CHAR_EUCKR;
						
					}
					
				}
				else if(m_parser.NumEqualIgnoreCase(szTransferEncoding))
				{
					m_parser.ConsumeLength(sizeof(szTransferEncoding));
					// chunked encoding 방식
					// 16진수 bytes \r\n
					// <- Data ->
					// 16진수 bytes \r\n
					// <- Data ->
					// 0\r\n\r\n -> 끝을 표시
					m_parser.ConsumeUntil(CStringLineParser::sEOLWhitespaceMask);

					if(!strcmp(m_parser.GetString(), "chunked"))
						m_bChunked = TRUE;
				}

			} while(m_parser.GoNextLine());

			if(m_bHeaderOnly)	// 헤더만 받기 원할경우 연결끊도록 함..
				return NO_ERROR;

			if(m_bChunked)
				nResult = ProcessChunkedData();
			else
			{
				nResult = ProcessContents();

				if(bConnectionClose == FALSE && nContentLen == -1)
					return ERROR_MORE_DATA;
			}

			//if(m_nResultCode == 401)
			//	return ERROR_LOGON_FAILURE;

			// 기타 내부에러는 연결해제 하지 않음...			

			return nResult;
		}
	}
	else
	{
		if(m_bChunked)
			nResult = ProcessChunkedData();
		else
			nResult = ProcessContents();
	}

	return nResult;
}

int CHTTPRequest::ProcessContents()
{
	int nResult = NO_ERROR;

	if(m_bConnectionClose /*&& m_nContentLength == 0*/)
		nResult = ERROR_MORE_DATA;	// Connection: close 이고, Content-Length가 존재하지 않는 경우.. 서버에서 연결이 끊어지도록 유도한다..

	if(!m_pCallback)
	{
		size_t nSize = m_recvBuffer.GetSize() + m_dataBuffer.GetSize();
		if(m_nContentLength > nSize)
			nResult = ERROR_MORE_DATA;
		else if(m_nContentLength > 0 && m_nContentLength == nSize)
			nResult = NO_ERROR;

		m_dataBuffer.Append( m_recvBuffer.GetPtr(), m_recvBuffer.GetSize() );
	}
	else
	{
		m_nRecvDataSize += m_recvBuffer.GetSize();
		if(m_recvBuffer.GetSize() > 0)
		{
			m_http_down.pBuffer			= m_recvBuffer.GetPtr();
			m_http_down.nBufferLength	= m_recvBuffer.GetSize();
			m_pCallback(&m_http_down);
		}

		if(m_nContentLength > m_nRecvDataSize)
			nResult = ERROR_MORE_DATA;
	}

	m_recvBuffer.Erase();


	return nResult;
}

int CHTTPRequest::ProcessChunkedData()
{
	const char *p;
	size_t nDataSize;
	int nFind;
	char HEX[16], *pEnd = NULL;
	unsigned long nChunkSize;

	do
	{
		nDataSize = m_recvBuffer.GetSize();

		if(nDataSize == 0)
			return ERROR_MORE_DATA;

		p = m_recvBuffer.GetPtr();
		m_parser.SetString(p);

		nFind = m_parser.Find(p, nDataSize, "\r\n", 2);
		if(nFind == -1)
			return ERROR_MORE_DATA;

		strncpy(HEX, p, nFind);
		HEX[nFind] = '\0';

		p += (nFind + 2);	// \r\n

		nChunkSize = strtoul(HEX, &pEnd, 16);
		if(nChunkSize > nDataSize)
			return ERROR_MORE_DATA;

		if(nChunkSize == 0)	// 데이타 끝.
		{
			if(nDataSize == 5)	// 데이타 끝은 "0\r\n\r\n"
				return NO_ERROR;
			else
				return ERROR_MORE_DATA;
		}

		if(!m_pCallback)
			m_dataBuffer.Append( (void*)p, nChunkSize );
		else
		{
			m_nRecvDataSize += m_recvBuffer.GetSize();
			m_http_down.pBuffer			= (char*)p;
			m_http_down.nBufferLength	= nChunkSize;
			m_pCallback(&m_http_down);
		}
		m_recvBuffer.Erase(0, nFind + nChunkSize + 4 );	// 4 => chunk->\r\n data->\r\n 삭제함.

	} while(m_recvBuffer.GetSize() > 0);
	

	return ERROR_MORE_DATA;
}

const char* CHTTPRequest::GetHeader()
{
	return m_headBuffer.GetPtr();
}

const wchar_t* CHTTPRequest::GetHeaderW()
{
	if(m_headBuffer.GetSize() == 0)
		m_whead.clear();
	else
		m_whead = AStringToWString(m_headBuffer.GetPtr());

	return m_whead.c_str();
}

const char* CHTTPRequest::GetResponse()
{
	return m_dataBuffer.GetPtr();
}

const wchar_t* CHTTPRequest::GetResponseW()
{
	if(m_dataBuffer.GetSize() == 0)
		m_wdata.clear();
	else
	{
		if(m_charset == CHAR_UTF8)
			m_wdata = UTF8ToWString(m_dataBuffer.GetPtr());
		else
			m_wdata = AStringToWString(m_dataBuffer.GetPtr());
	}

	return m_wdata.c_str();
}

size_t CHTTPRequest::GetResponseSize() const
{
	return m_dataBuffer.GetSize();
}

size_t CHTTPRequest::GetContentLength() const
{
	return m_nContentLength;
}

void CHTTPRequest::MakeAuthField(const char *lpUser, const char *lpPass)
{
	// 현재 Basic 만

	if(lpUser == NULL || lpPass == NULL)
	{
		m_sAuth.clear();
		return;
	}

	char szAuth[MAX_PATH], szEncodeAuth[MAX_PATH];
	sprintf(szAuth, "%s:%s", lpUser, lpPass);
	int len = MAX_PATH;

	if(Base64Encode((BYTE*)szAuth, strlen(szAuth), szEncodeAuth, &len))
	{
		szEncodeAuth[len] = '\0';
		_sprintf_string(m_sAuth, "Authorization: Basic %s\r\n", szEncodeAuth);
	}
	else
		m_sAuth.clear();
}

int CHTTPRequest::GetResponseCode() const
{
	return m_nResultCode;
}

BOOL CHTTPRequest::DownloadFile(HWND hWnd, const char *lpHost, const char *lpUrl, CallbackHttpDwonloadFunPtr pCallback,
		UINT nPort /*= 80*/,
		BOOL bHeaderOnly /*= FALSE*/,
		LPARAM lParam /*= NULL*/,
		const char *lpUser /*= NULL*/, const char *lpPassword /*= NULL*/,
		BOOL bSSL /*= FALSE*/, DWORD dwTimeout /*= 0*/)
{
	BOOL bRet;
	m_pCallback			= pCallback;
	m_hwndParent		= hWnd;
	m_http_down.lParam	= lParam;
	m_bHeaderOnly		= bHeaderOnly;

	bRet = SendRequest(lpHost, lpUrl, nPort, NULL, 0, HTTP_GET, lpUser, lpPassword, bSSL, dwTimeout);

	m_pCallback			= NULL;
	m_hwndParent		= NULL;
	m_bHeaderOnly		= FALSE;

	return TRUE;
}

BOOL CHTTPRequest::DownloadFile(HWND hWnd, const wchar_t *lpHost, const wchar_t *lpUrl, CallbackHttpDwonloadFunPtr pCallback,
		UINT nPort /*= 80*/,
		BOOL bHeaderOnly /*= FALSE*/,
		LPARAM lParam /*= NULL*/,
		const wchar_t *lpUser /*= NULL*/, const wchar_t *lpPassword /*= NULL*/,
		BOOL bSSL /*= FALSE*/, DWORD dwTimeout /*= 0*/)
{
	string host, url, user, pass;
	host	= WStringToAString(lpHost);
	url		= WStringToAString(lpUrl);
	user	= WStringToAString(lpUser);
	pass	= WStringToAString(lpPassword);

	return DownloadFile(hWnd, host.c_str(), url.c_str(), pCallback, nPort, bHeaderOnly, lParam, user.c_str(), pass.c_str(), bSSL, dwTimeout);
}

BOOL CHTTPRequest::AddUploadData(LPCTSTR lpName, LPCTSTR lpValue, BOOL bValue /*= TRUE*/)
{
	USES_CONVERSION_EX;

	HTTP_UPLOAD_DATA data;
#if _UNICODE
	data.sName		= WStringToUTF8(lpName);
	data.sValue		= WStringToUTF8(lpValue);
#else
	wstring sName	= AStringToWString(lpName);
	wstring sValue	= AStringToWString(lpValue);

	data.sName		= WStringToUTF8(sName.c_str());
	data.sName		= WStringToUTF8(sValue.c_str());
#endif
	data.isValue	= bValue;

	if(!data.isValue)
	{
		HANDLE hFile = CreateFile(lpValue, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		
		m_lTotalFileSize += dwFileSize;

		CloseHandle(hFile);
	}

	m_vData.push_back(data);

	return TRUE;
}

void CHTTPRequest::InitUpload()
{
	m_vData.clear();
	m_lTotalFileSize	= 0;
	m_hwndParent		= NULL;
}

BOOL CHTTPRequest::UploadProc(HWND hWnd, LPCTSTR lpHost, LPCTSTR lpUrl, UINT nPort, 
		LPCTSTR lpUser /*= NULL*/, LPCTSTR lpPassword /*= NULL*/,
		BOOL bSSL /*= FALSE*/)
{
	USES_CONVERSION_EX;

	string host	= T2A_EX((LPTSTR)lpHost,		_ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	string url	= T2A_EX((LPTSTR)lpUrl,			_ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	string user	= T2A_EX((LPTSTR)lpUser,		_ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	string pass	= T2A_EX((LPTSTR)lpPassword,	_ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	string sContFooter;
	size_t nPostSize;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwFileSize, dwRead, dwTotalRead;
	int nPercent = 0;
	char szHost[MAX_PATH];

	if(!Connect(host.c_str(), nPort, bSSL))
		return FALSE;

	if(m_bIPv6)
	{
		if(strstr(host.c_str(), ":"))
			sprintf_s(szHost, MAX_PATH, "[%s]", host.c_str());
		else
			strcpy_s(szHost, MAX_PATH, host.c_str());
	}
	else
		strcpy_s(szHost, MAX_PATH, host.c_str());

	MakeAuthField(user.c_str(), pass.c_str());

	m_sendBuffer.SetReserve( 10240 );
	m_sendBuffer.Assign(NULL, 0);

	size_t nDataLength = 0;
	vector<HTTP_UPLOAD_DATA>::iterator pos = m_vData.begin();
	while(pos != m_vData.end())
	{
		if(pos->isValue)
		{
			_sprintf_string(pos->sBoundary, "--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s",
				m_sBoundary.c_str(), pos->sName.c_str(), pos->sValue.c_str());
		}
		else
		{
			_sprintf_string(pos->sBoundary, "--%s\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: application/octet-stream\r\n\r\n",
				m_sBoundary.c_str(), pos->sName.c_str(), pos->sValue.c_str());
		}

		// 2를 더한것은 미테 \r\n 의 길이를 미리 더한것이다..
		nDataLength += pos->sBoundary.size() + 2;

		pos++;
	}
	_sprintf_string(sContFooter, "--%s--\r\n", m_sBoundary.c_str());

	nPostSize = nDataLength + m_lTotalFileSize + sContFooter.size();

	m_nSendSize = sprintf(m_sendBuffer.GetPtr(),
			"POST %s HTTP/1.1\r\n" \
			"Accept: */*\r\n" \
			"User-Agent: %s\r\n" \
			"Host: %s\r\n" \
			"%sContent-Type: multipart/form-data; boundary=%s\r\n" \
			"Content-Length: %d\r\n" \
			"Connection: Keep-Alive\r\n\r\n",

			url.c_str(),
			m_sAgent.c_str(),
			szHost,
			m_sAuth.c_str(),
			m_sBoundary.c_str(),
			nPostSize
			);
	m_sendBuffer.SetSize( m_nSendSize );

	// 헤더 보내고...
	if(SendData( m_sendBuffer.GetPtr(), m_sendBuffer.GetSize() ) != NO_ERROR)
		return FALSE;

	// 데이타 보내고..
	pos = m_vData.begin();
	while(pos != m_vData.end())
	{
		m_sendBuffer.Assign( (void*)pos->sBoundary.c_str(), pos->sBoundary.size() );
		if(SendData( m_sendBuffer.GetPtr(), m_sendBuffer.GetSize() ) != NO_ERROR)
			return FALSE;

		if(!pos->isValue)
		{
			// 파일인 경우
			hFile = CreateFileW( UTF8ToWString(pos->sValue.c_str()).c_str(), GENERIC_READ,
				FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
			dwFileSize = GetFileSize(hFile, NULL);

			dwRead = 0;
			dwTotalRead = 0;

			//MSG msg;
			do
			{
				ReadFile(hFile, (void*)m_sendBuffer.GetPtr(), m_sendBuffer.GetCapacity() - 1, &dwRead, NULL);
				m_sendBuffer.SetSize(dwRead);
				dwTotalRead += dwRead;

				nPercent = (int)((dwTotalRead / (float)dwFileSize) * 100);
				TRACE(">>> %d\n", nPercent);

				if(SendData( m_sendBuffer.GetPtr(), m_sendBuffer.GetSize() ) != NO_ERROR)
					return FALSE;

				if(hWnd)
				{
					::SendMessage(hWnd, WM_MYPROGRESSINC, nPercent, (LPARAM)m_pParam);
				}

				//Sleep(50);

			} while( dwTotalRead < dwFileSize );

			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}

		if(SendData( "\r\n", 2 ) != NO_ERROR)
			return FALSE;

		pos++;
	}

	if(SendData( (char*)sContFooter.c_str(), sContFooter.size() ) != NO_ERROR)
		return FALSE;


	// 용량큰거 업로드시 타임아웃 걸릴 수 도 있구만..
	int nTimeout = 60000;	// 60초 타임아웃 설정 : 메가픽셀 너무 오래걸린다.
	setsockopt(m_hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(nTimeout));

	if(RecvData() != NO_ERROR)
		return FALSE;

	nTimeout = (int)m_dwTimeout;
	setsockopt(m_hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(nTimeout));

	if(hWnd)
		::SendMessage(hWnd, WM_MYPROGRESSSUC, 0, (LPARAM)m_pParam);

	return TRUE;
}

void CHTTPRequest::Stop()
{
	Disconnect();
	m_nResultCode = 0;

	TRACE("CHTTPRequest::Stop()\n");
}
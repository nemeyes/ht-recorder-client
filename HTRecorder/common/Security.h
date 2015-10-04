#pragma once
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

#include <winsock2.h>
#include <winsock.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <schannel.h>

#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>

#include <HTRecorderDLL.h>

class CSecurity
{
public:
	CSecurity(void);
	virtual ~CSecurity(void);

	BOOL Initialize(SOCKET hSocket, const char *lpAddress);
	void Destroy();

	//int SendData(char *pBuffer, size_t nLength);
	int Encrypt(char *pBuffer, size_t nLength, CBuffer *pBuf);
	int Decrypt(char *pBuffer, size_t nLength, CBuffer *pBuf);

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Security
	BOOL LoadSecurityLibrary(void);
	void UnloadSecurityLibrary();

	SECURITY_STATUS CreateCredentials(LPSTR lpszUserName, PCredHandle phCreds);
	SECURITY_STATUS PerformClientHandshake(
		SOCKET          Socket,
		PCredHandle     phCreds,
		LPTSTR          pszServerName,
		CtxtHandle *    phContext,
		SecBuffer *     pExtraData
		);
	SECURITY_STATUS ClientHandshakeLoop(
		SOCKET          Socket,
		PCredHandle     phCreds,
		CtxtHandle *    phContext,
		BOOL            fDoInitialRead,
		SecBuffer *     pExtraData);
	void GetNewClientCredentials(CredHandle *phCreds, CtxtHandle *phContext);
	DWORD VerifyServerCertificate(PCCERT_CONTEXT pServerCert, PTSTR pszServerName, DWORD dwCertFlags);

	void DisplayCertChain(PCCERT_CONTEXT pServerCert, BOOL fLocal);
	void DisplayWinVerifyTrustError(DWORD Status);
	void DisplayConnectionInfo(CtxtHandle *phContext);

	HMODULE m_hSecurity;
	PSecurityFunctionTable m_pSSPI;
	HCERTSTORE      m_hMyCertStore;
	SCHANNEL_CRED   m_SchannelCred;

	ALG_ID			m_aiKeyExch;
	DWORD			m_dwProtocol;
	
	CredHandle		m_hClientCreds;
	CtxtHandle		m_hContext;
	SecBuffer		m_ExtraData;
	PCCERT_CONTEXT	m_pRemoteCertContext;

	BOOL m_fCredsInitialized;
	BOOL m_fContextInitialized;

	PBYTE			m_pbIoBuffer;
	DWORD			m_dwIoLength;
	///////////////////////////////////////////////////////////////////////////////////////////////

	SOCKET			m_hSocket;
	BOOL			m_bSSL;
};

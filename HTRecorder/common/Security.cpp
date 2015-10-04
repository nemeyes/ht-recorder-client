#include "Common.h"
#include "security.h"
#include "string_helper.h"

#define IO_BUFFER_SIZE	0x10000

static const TCHAR DLL_NAME[] = _T("Secur32.dll");
static const TCHAR NT4_DLL_NAME[] = _T("Security.dll");

static const BYTE msTestPubKey1[] = {
0x30,0x47,0x02,0x40,0x81,0x55,0x22,0xb9,0x8a,0xa4,0x6f,0xed,0xd6,0xe7,0xd9,
0x66,0x0f,0x55,0xbc,0xd7,0xcd,0xd5,0xbc,0x4e,0x40,0x02,0x21,0xa2,0xb1,0xf7,
0x87,0x30,0x85,0x5e,0xd2,0xf2,0x44,0xb9,0xdc,0x9b,0x75,0xb6,0xfb,0x46,0x5f,
0x42,0xb6,0x9d,0x23,0x36,0x0b,0xde,0x54,0x0f,0xcd,0xbd,0x1f,0x99,0x2a,0x10,
0x58,0x11,0xcb,0x40,0xcb,0xb5,0xa7,0x41,0x02,0x03,0x01,0x00,0x01 };

static const BYTE msTestPubKey2[] = {
0x30,0x48,0x02,0x41,0x00,0x81,0x55,0x22,0xb9,0x8a,0xa4,0x6f,0xed,0xd6,0xe7,
0xd9,0x66,0x0f,0x55,0xbc,0xd7,0xcd,0xd5,0xbc,0x4e,0x40,0x02,0x21,0xa2,0xb1,
0xf7,0x87,0x30,0x85,0x5e,0xd2,0xf2,0x44,0xb9,0xdc,0x9b,0x75,0xb6,0xfb,0x46,
0x5f,0x42,0xb6,0x9d,0x23,0x36,0x0b,0xde,0x54,0x0f,0xcd,0xbd,0x1f,0x99,0x2a,
0x10,0x58,0x11,0xcb,0x40,0xcb,0xb5,0xa7,0x41,0x02,0x03,0x01,0x00,0x01 };

static const BYTE msTestPubKey3[] = {
0x30,0x47,0x02,0x40,0x9c,0x50,0x05,0x1d,0xe2,0x0e,0x4c,0x53,0xd8,0xd9,0xb5,
0xe5,0xfd,0xe9,0xe3,0xad,0x83,0x4b,0x80,0x08,0xd9,0xdc,0xe8,0xe8,0x35,0xf8,
0x11,0xf1,0xe9,0x9b,0x03,0x7a,0x65,0x64,0x76,0x35,0xce,0x38,0x2c,0xf2,0xb6,
0x71,0x9e,0x06,0xd9,0xbf,0xbb,0x31,0x69,0xa3,0xf6,0x30,0xa0,0x78,0x7b,0x18,
0xdd,0x50,0x4d,0x79,0x1e,0xeb,0x61,0xc1,0x02,0x03,0x01,0x00,0x01 };


CSecurity::CSecurity(void)
:m_hSocket(INVALID_SOCKET)
{
	m_bSSL					= FALSE;
	m_fCredsInitialized		= FALSE;
	m_fContextInitialized	= FALSE;
	m_hSecurity				= NULL;
	m_pSSPI					= NULL;
	m_hMyCertStore			= NULL;
	m_pRemoteCertContext	= NULL;
	
	m_aiKeyExch				= 0;	// defaul
	// CALG_RSA_KEYX : RSA, CALG_DH_EPHEM : DH

	m_dwProtocol			= 0;
	// SP_PROT_PCT1	PCT 1.0
	// SP_PROT_SSL2	SSL 2.0
	// SP_PROT_SSL3	SSL 3.0
	// SP_PROT_TLS1	TLS 1.0

	m_pbIoBuffer			= NULL;
	m_dwIoLength			= 0;
}

CSecurity::~CSecurity(void)
{
	Destroy();
}

BOOL CSecurity::Initialize(SOCKET hSocket, const char *lpAddress)
{
	SECURITY_STATUS Status;
	if(hSocket == INVALID_SOCKET)
		return FALSE;

	if(!LoadSecurityLibrary())
		return FALSE;

	if(!m_fCredsInitialized)
	{
		if(CreateCredentials(NULL, &m_hClientCreds))
		{
			Destroy();
			return FALSE;
		}
		else
			m_fCredsInitialized = TRUE;
	}

	wstring sAddr = AStringToWString(lpAddress);

	m_hSocket = hSocket;

	// Perform handshake
	Status = PerformClientHandshake(m_hSocket, &m_hClientCreds, (LPTSTR)sAddr.c_str(), &m_hContext, &m_ExtraData);
	TRACE("PerformClientHandshake Status :%x\n", Status);
	if(Status != SEC_E_OK)
	{
		return FALSE;
	}

	m_fContextInitialized = TRUE;

	// Authenticate server's credentials.
	// Get server's certificate.
	m_pSSPI->QueryContextAttributes(&m_hContext, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (PVOID)&m_pRemoteCertContext);

	// Display server certificate chain.
	DisplayCertChain(m_pRemoteCertContext, FALSE);

	// Attempt to validate server certificate.
	Status = VerifyServerCertificate(m_pRemoteCertContext, (PTSTR)sAddr.c_str(), 0);

	if(Status)
	{
		// The server certificate did not validate correctly. At this
		// point, we cannot tell if we are connecting to the correct 
		// server, or if we are connecting to a "man in the middle" 
		// attack server.

		// It is therefore best if we abort the connection.

		TRACE("**** Error 0x%x authenticating server credentials!\n", Status);
		//        goto cleanup;

		// Destroy();
		// return FALSE;
	}

	// Free the server certificate context.
	CertFreeCertificateContext(m_pRemoteCertContext);
	m_pRemoteCertContext = NULL;

	// Display connection info. 
	DisplayConnectionInfo(&m_hContext);

	m_bSSL = TRUE;

	return TRUE;
}

void CSecurity::Destroy()
{
	if(m_bSSL)
	{
		DWORD           dwType;
		PBYTE           pbMessage;
		DWORD           cbMessage;
		DWORD           cbData;

		SecBufferDesc   OutBuffer;
		SecBuffer       OutBuffers[1];
		DWORD           dwSSPIFlags;
		DWORD           dwSSPIOutFlags;
		TimeStamp       tsExpiry;
		DWORD           Status;

		// Notify schannel that we are about to close the connection.
		dwType = SCHANNEL_SHUTDOWN;

		OutBuffers[0].pvBuffer   = &dwType;
		OutBuffers[0].BufferType = SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer   = sizeof(dwType);

		OutBuffer.cBuffers  = 1;
		OutBuffer.pBuffers  = OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		Status = m_pSSPI->ApplyControlToken(&m_hContext, &OutBuffer);

		if(FAILED(Status))
		{
			TRACE("**** Error 0x%x returned by ApplyControlToken\n", Status);
			goto cleanup;
		}

		dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
			ISC_REQ_REPLAY_DETECT     |
			ISC_REQ_CONFIDENTIALITY   |
			ISC_RET_EXTENDED_ERROR    |
			ISC_REQ_ALLOCATE_MEMORY   |
			ISC_REQ_STREAM;

		OutBuffers[0].pvBuffer   = NULL;
		OutBuffers[0].BufferType = SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer   = 0;

		OutBuffer.cBuffers  = 1;
		OutBuffer.pBuffers  = OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		Status = m_pSSPI->InitializeSecurityContext(
			&m_hClientCreds,
			&m_hContext,
			NULL,
			dwSSPIFlags,
			0,
			SECURITY_NATIVE_DREP,
			NULL,
			0,
			&m_hContext,
			&OutBuffer,
			&dwSSPIOutFlags,
			&tsExpiry);

		if(FAILED(Status))
		{
			TRACE("**** Error 0x%x returned by InitializeSecurityContext\n", Status);
			goto cleanup;
		}

		pbMessage = (PBYTE)OutBuffers[0].pvBuffer;
		cbMessage = OutBuffers[0].cbBuffer;

		if(pbMessage != NULL && cbMessage != 0)
		{
			cbData = send(m_hSocket, (const char*)pbMessage, cbMessage, 0);
			if(cbData == SOCKET_ERROR || cbData == 0)
			{
				Status = WSAGetLastError();
				TRACE("**** Error %d sending close notify\n", Status);
				goto cleanup;
			}

			TRACE("Sending Close Notify\n");
			TRACE("%d bytes of handshake data sent\n", cbData);

			//			if(fVerbose)
			{
				//PrintHexDump(cbData, pbMessage);
				//printf("\n");
			}

			// Free output buffer.
			m_pSSPI->FreeContextBuffer(pbMessage);
		}



	}

cleanup:

	// Free the server certificate context.
	if(m_pRemoteCertContext)
	{
		CertFreeCertificateContext(m_pRemoteCertContext);
		m_pRemoteCertContext = NULL;
	}

	// Free SSPI context handle.
	if(m_fContextInitialized)
	{
		m_pSSPI->DeleteSecurityContext(&m_hContext);
		m_fContextInitialized = FALSE;
	}

	// Free SSPI credentials handle.
	if(m_fCredsInitialized)
	{
		m_pSSPI->FreeCredentialsHandle(&m_hClientCreds);
		m_fCredsInitialized = FALSE;
	}

	if(m_pbIoBuffer)
	{
		LocalFree(m_pbIoBuffer);
		m_pbIoBuffer = NULL;
		m_dwIoLength = 0;
	}

	// Close "MY" certificate store.
	if(m_hMyCertStore)
	{
		CertCloseStore(m_hMyCertStore, 0);
		m_hMyCertStore = NULL;
	}

	UnloadSecurityLibrary();

	m_bSSL = FALSE;
}

BOOL CSecurity::LoadSecurityLibrary(void)
{
	INIT_SECURITY_INTERFACE			pInitSecurityInterface;
	//	QUERY_CREDENTIALS_ATTRIBUTES_FN	pQueryCredentialsAttributes;
	OSVERSIONINFO					VerInfo;
	TCHAR lpszDLL[MAX_PATH];

	//
    //  Find out which security DLL to use, depending on
    //  whether we are on Win2K, NT or Win9x
    //
	VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	if (!GetVersionEx (&VerInfo))
		return FALSE;

	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && VerInfo.dwMajorVersion == 4)
		_tcscpy(lpszDLL, NT4_DLL_NAME);
	else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ||
		VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
		_tcscpy(lpszDLL, DLL_NAME);
	else
		return FALSE;

	//
    //  Load Security DLL
    //
	m_hSecurity = LoadLibrary(lpszDLL);
	if(m_hSecurity == NULL)
		return FALSE;

#if defined(UNICODE)
	pInitSecurityInterface = (INIT_SECURITY_INTERFACE)GetProcAddress(m_hSecurity, "InitSecurityInterfaceW");
#else
	pInitSecurityInterface = (INIT_SECURITY_INTERFACE)GetProcAddress(m_hSecurity, "InitSecurityInterfaceA");
#endif

	if(pInitSecurityInterface == NULL)
		return FALSE;

	m_pSSPI = pInitSecurityInterface();
	if(m_pSSPI == NULL)
		return FALSE;

	return TRUE;
}

void CSecurity::UnloadSecurityLibrary()
{
	if(m_hSecurity)
	{
		FreeLibrary(m_hSecurity);
		m_hSecurity = NULL;
	}
}

SECURITY_STATUS CSecurity::CreateCredentials(LPSTR lpszUserName, PCredHandle phCreds)
{
	TimeStamp       tsExpiry;
	SECURITY_STATUS Status = SEC_E_NO_CREDENTIALS;
	
	DWORD           cSupportedAlgs = 0;
	ALG_ID          rgbSupportedAlgs[16];

	PCCERT_CONTEXT  pCertContext = NULL;


	// Open the "MY" certificate store, which is where Internet Explorer
    // stores its client certificates.
    if(m_hMyCertStore == NULL)
	{
		m_hMyCertStore = CertOpenSystemStore(NULL, _T("My"));
		if(!m_hMyCertStore)
		{
			return SEC_E_NO_CREDENTIALS;
		}
	}

	//
    // If a user name is specified, then attempt to find a client
    // certificate. Otherwise, just create a NULL credential.
    //
	//TRACE(_T("A: %u, %W: %u\n"), CERT_FIND_SUBJECT_STR_A, CERT_FIND_SUBJECT_STR_W, CERT_FIND_SUBJECT_STR);
	if(lpszUserName)
	{
		// Find client certificate. Note that this sample just searchs for a 
        // certificate that contains the user name somewhere in the subject name.
        // A real application should be a bit less casual.
		pCertContext = CertFindCertificateInStore(m_hMyCertStore, X509_ASN_ENCODING, 0,
			CERT_FIND_SUBJECT_STR_A,
			lpszUserName,
			NULL);
		if(pCertContext == NULL)
			return SEC_E_NO_CREDENTIALS;
	}
	//
    // Build Schannel credential structure. Currently, this sample only
    // specifies the protocol to be used (and optionally the certificate, 
    // of course). Real applications may wish to specify other parameters 
    // as well.
    //

    ZeroMemory(&m_SchannelCred, sizeof(m_SchannelCred));
	m_SchannelCred.dwVersion  = SCHANNEL_CRED_VERSION;
	if(pCertContext)
	{
		m_SchannelCred.cCreds     = 1;
        m_SchannelCred.paCred     = &pCertContext;
	}
	m_SchannelCred.grbitEnabledProtocols = m_dwProtocol;

	if(m_aiKeyExch)
	{
		rgbSupportedAlgs[cSupportedAlgs++] = m_aiKeyExch;
	}

	if(cSupportedAlgs)
	{
		m_SchannelCred.cSupportedAlgs    = cSupportedAlgs;
        m_SchannelCred.palgSupportedAlgs = rgbSupportedAlgs;
	}

	m_SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;

	// The SCH_CRED_MANUAL_CRED_VALIDATION flag is specified because
    // this sample verifies the server certificate manually. 
    // Applications that expect to run on WinNT, Win9x, or WinME 
    // should specify this flag and also manually verify the server
    // certificate. Applications running on newer versions of Windows can
    // leave off this flag, in which case the InitializeSecurityContext
    // function will validate the server certificate automatically.
    m_SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;

	//
	// Create an SSPI credential.
	//
#if defined(UNICODE)
	Status = m_pSSPI->AcquireCredentialsHandleW(
		NULL,                   // Name of principal    
		UNISP_NAME_W,           // Name of package
		SECPKG_CRED_OUTBOUND,   // Flags indicating use
		NULL,                   // Pointer to logon ID
		&m_SchannelCred,          // Package specific data
		NULL,                   // Pointer to GetKey() func
		NULL,                   // Value to pass to GetKey()
		phCreds,                // (out) Cred Handle
		&tsExpiry);             // (out) Lifetime (optional)
#else
	Status = m_pSSPI->AcquireCredentialsHandleA(
		NULL,                   // Name of principal    
		UNISP_NAME_A,           // Name of package
		SECPKG_CRED_OUTBOUND,   // Flags indicating use
		NULL,                   // Pointer to logon ID
		&m_SchannelCred,          // Package specific data
		NULL,                   // Pointer to GetKey() func
		NULL,                   // Value to pass to GetKey()
		phCreds,                // (out) Cred Handle
		&tsExpiry);             // (out) Lifetime (optional)
#endif

	if(Status != SEC_E_OK)
	{
		// todo log
		goto cleanup;
	}

cleanup:
	//
    // Free the certificate context. Schannel has already made its own copy.
    //

	if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    return Status;
}

SECURITY_STATUS CSecurity::PerformClientHandshake(
		SOCKET          Socket,
		PCredHandle     phCreds,
		LPTSTR          pszServerName,
		CtxtHandle *    phContext,
		SecBuffer *     pExtraData
		)
{
	SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];
    DWORD           dwSSPIFlags;
    DWORD           dwSSPIOutFlags;
    TimeStamp       tsExpiry;
    SECURITY_STATUS scRet;
    DWORD           cbData;

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_RET_EXTENDED_ERROR    |
                  ISC_REQ_ALLOCATE_MEMORY   |
                  ISC_REQ_STREAM;

    //
    //  Initiate a ClientHello message and generate a token.
    //
	OutBuffers[0].pvBuffer   = NULL;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = 0;

    OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	scRet = m_pSSPI->InitializeSecurityContext(
		phCreds,
		NULL,
		pszServerName,
		dwSSPIFlags,
		0,
		SECURITY_NATIVE_DREP,
		NULL,
		0,
		phContext,
		&OutBuffer,
		&dwSSPIOutFlags,
		&tsExpiry);

	if(scRet != SEC_I_CONTINUE_NEEDED)
		return scRet;

	// Send response to server if there is one.
    if(OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
	{
		cbData = send(m_hSocket,
			(const char*)OutBuffers[0].pvBuffer,
			OutBuffers[0].cbBuffer,
			0);
		if(cbData == SOCKET_ERROR || cbData == 0)
		{
			m_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
			m_pSSPI->DeleteSecurityContext(phContext);
			return SEC_E_INTERNAL_ERROR;
		}

		m_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
		OutBuffers[0].pvBuffer = NULL;
	}

	return ClientHandshakeLoop(Socket, phCreds, phContext, TRUE, pExtraData);
}

SECURITY_STATUS CSecurity::ClientHandshakeLoop(
		SOCKET          Socket,
		PCredHandle     phCreds,
		CtxtHandle *    phContext,
		BOOL            fDoInitialRead,
		SecBuffer *     pExtraData)
{
	SecBufferDesc   InBuffer;
    SecBuffer       InBuffers[2];
    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];
    DWORD           dwSSPIFlags;
    DWORD           dwSSPIOutFlags;
    TimeStamp       tsExpiry;
    SECURITY_STATUS scRet;
    DWORD           cbData;

    PUCHAR          IoBuffer;
    DWORD           cbIoBuffer;
    BOOL            fDoRead;


    dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_RET_EXTENDED_ERROR    |
                  ISC_REQ_ALLOCATE_MEMORY   |
                  ISC_REQ_STREAM;

    //
    // Allocate data buffer.
    //
	IoBuffer = (PUCHAR)LocalAlloc(LMEM_FIXED, IO_BUFFER_SIZE);
	if(IoBuffer == NULL)
		return SEC_E_INTERNAL_ERROR;

	cbIoBuffer	= 0;
	fDoRead		= fDoInitialRead;

	// 
    // Loop until the handshake is finished or an error occurs.
    //
	scRet = SEC_I_CONTINUE_NEEDED;

	while(scRet == SEC_I_CONTINUE_NEEDED        ||
          scRet == SEC_E_INCOMPLETE_MESSAGE     ||
          scRet == SEC_I_INCOMPLETE_CREDENTIALS) 
	{
		//
        // Read data from server.
        //

        if(0 == cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE)
        {
            if(fDoRead)
            {
                cbData = recv(Socket, 
                              (char*)(IoBuffer + cbIoBuffer),
                              IO_BUFFER_SIZE - cbIoBuffer,
                              0);
                if(cbData == SOCKET_ERROR)
                {
                    TRACE("**** Error %d reading data from server\n", WSAGetLastError());
                    scRet = SEC_E_INTERNAL_ERROR;

					return SEC_E_INTERNAL_ERROR;
                }
                else if(cbData == 0)
                {
                    TRACE("**** Server unexpectedly disconnected\n");
                    scRet = SEC_E_INTERNAL_ERROR;

					return SEC_E_INTERNAL_ERROR;
                }

                TRACE("%d bytes of handshake data received\n", cbData);

                //if(fVerbose)
                //{
                    //PrintHexDump(cbData, IoBuffer + cbIoBuffer);
                    //TRACE("\n");
                //}

                cbIoBuffer += cbData;
            }
            else
            {
                fDoRead = TRUE;
            }
        }

		//
        // Set up the input buffers. Buffer 0 is used to pass in data
        // received from the server. Schannel will consume some or all
        // of this. Leftover data (if any) will be placed in buffer 1 and
        // given a buffer type of SECBUFFER_EXTRA.
        //

        InBuffers[0].pvBuffer   = IoBuffer;
        InBuffers[0].cbBuffer   = cbIoBuffer;
        InBuffers[0].BufferType = SECBUFFER_TOKEN;

        InBuffers[1].pvBuffer   = NULL;
        InBuffers[1].cbBuffer   = 0;
        InBuffers[1].BufferType = SECBUFFER_EMPTY;

        InBuffer.cBuffers       = 2;
        InBuffer.pBuffers       = InBuffers;
        InBuffer.ulVersion      = SECBUFFER_VERSION;

        //
        // Set up the output buffers. These are initialized to NULL
        // so as to make it less likely we'll attempt to free random
        // garbage later.
        //

        OutBuffers[0].pvBuffer  = NULL;
        OutBuffers[0].BufferType= SECBUFFER_TOKEN;
        OutBuffers[0].cbBuffer  = 0;

        OutBuffer.cBuffers      = 1;
        OutBuffer.pBuffers      = OutBuffers;
		OutBuffer.ulVersion     = SECBUFFER_VERSION;

		//
		// Call InitializeSecurityContext.
		//

		scRet = m_pSSPI->InitializeSecurityContext(phCreds,
			phContext,
			NULL,
			dwSSPIFlags,
			0,
			SECURITY_NATIVE_DREP,
			&InBuffer,
			0,
			NULL,
			&OutBuffer,
			&dwSSPIOutFlags,
			&tsExpiry);

		//
        // If InitializeSecurityContext was successful (or if the error was 
        // one of the special extended ones), send the contends of the output
        // buffer to the server.
        //

        if(scRet == SEC_E_OK                ||
           scRet == SEC_I_CONTINUE_NEEDED   ||
           FAILED(scRet) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR))
        {
			if(OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
			{
				cbData = send(Socket,
                              (const char*)OutBuffers[0].pvBuffer,
                              OutBuffers[0].cbBuffer,
                              0);
                if(cbData == SOCKET_ERROR || cbData == 0)
                {
                    TRACE("**** Error %d sending data to server (2)\n", 
                        WSAGetLastError());
                    m_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
                    m_pSSPI->DeleteSecurityContext(phContext);
                    return SEC_E_INTERNAL_ERROR;
                }

				TRACE("%d bytes of handshake data sent\n", cbData);

				// Free output buffer.
                m_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
                OutBuffers[0].pvBuffer = NULL;
			}
		}

		//
        // If InitializeSecurityContext returned SEC_E_INCOMPLETE_MESSAGE,
        // then we need to read more data from the server and try again.
        //

		if(scRet == SEC_E_INCOMPLETE_MESSAGE)
			continue;

		//
        // If InitializeSecurityContext returned SEC_E_OK, then the 
        // handshake completed successfully.
        //
		if(scRet == SEC_E_OK)
		{
			//
            // If the "extra" buffer contains data, this is encrypted application
            // protocol layer stuff. It needs to be saved. The application layer
            // will later decrypt it with DecryptMessage.
            //
			TRACE("Handshake was successful\n");

			if(InBuffers[1].BufferType == SECBUFFER_EXTRA)
			{
				pExtraData->pvBuffer = LocalAlloc(LMEM_FIXED, InBuffers[1].cbBuffer);
				if(pExtraData->pvBuffer == NULL)
				{
					TRACE("**** Out of memory (2)\n");
                    return SEC_E_INTERNAL_ERROR;
				}

				MoveMemory(pExtraData->pvBuffer,
                           IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer),
                           InBuffers[1].cbBuffer);

				pExtraData->cbBuffer   = InBuffers[1].cbBuffer;
                pExtraData->BufferType = SECBUFFER_TOKEN;

				TRACE("%d bytes of app data was bundled with handshake data\n", pExtraData->cbBuffer);
			}
			else
            {
                pExtraData->pvBuffer   = NULL;
                pExtraData->cbBuffer   = 0;
                pExtraData->BufferType = SECBUFFER_EMPTY;
            }

			//
            // Bail out to quit
            //

            break;
		}

		//
        // Check for fatal error.
        //

		if(FAILED(scRet))
        {
            TRACE("**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet);
            break;
        }

		//
        // If InitializeSecurityContext returned SEC_I_INCOMPLETE_CREDENTIALS,
        // then the server just requested client authentication. 
        //
		if(scRet == SEC_I_INCOMPLETE_CREDENTIALS)
		{
			//
            // Busted. The server has requested client authentication and
            // the credential we supplied didn't contain a client certificate.
            //

            // 
            // This function will read the list of trusted certificate
            // authorities ("issuers") that was received from the server
            // and attempt to find a suitable client certificate that
            // was issued by one of these. If this function is successful, 
            // then we will connect using the new certificate. Otherwise,
            // we will attempt to connect anonymously (using our current
            // credentials).
            //
            
            GetNewClientCredentials(phCreds, phContext);

            // Go around again.
            fDoRead = FALSE;
            scRet = SEC_I_CONTINUE_NEEDED;
            continue;
		}

		//
        // Copy any leftover data from the "extra" buffer, and go around
        // again.
        //

		if ( InBuffers[1].BufferType == SECBUFFER_EXTRA )
		{
			MoveMemory(IoBuffer,
                       IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer),
                       InBuffers[1].cbBuffer);

            cbIoBuffer = InBuffers[1].cbBuffer;
		}
		else
			cbIoBuffer = 0;
	}

	// Delete the security context in the case of a fatal error.
	if(FAILED(scRet))
    {
        m_pSSPI->DeleteSecurityContext(phContext);
    }

    LocalFree(IoBuffer);


	return 0;
}

void CSecurity::GetNewClientCredentials(CredHandle *phCreds, CtxtHandle *phContext)
{
	CredHandle hCreds;
	SecPkgContext_IssuerListInfoEx IssuerListInfo;
	PCCERT_CHAIN_CONTEXT pChainContext;
	CERT_CHAIN_FIND_BY_ISSUER_PARA FindByIssuerPara;
	PCCERT_CONTEXT  pCertContext;
	TimeStamp       tsExpiry;
	SECURITY_STATUS Status;

	//
	// Read list of trusted issuers from schannel.
	//

	Status = m_pSSPI->QueryContextAttributes(phContext,
		SECPKG_ATTR_ISSUER_LIST_EX,
		(PVOID)&IssuerListInfo);

	if(Status != SEC_E_OK)
    {
		TRACE("Error 0x%x querying issuer list info\n", Status);
        return;
    }

	//
    // Enumerate the client certificates.
    //

    ZeroMemory(&FindByIssuerPara, sizeof(FindByIssuerPara));

    FindByIssuerPara.cbSize = sizeof(FindByIssuerPara);
    FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
    FindByIssuerPara.dwKeySpec = 0;
    FindByIssuerPara.cIssuer   = IssuerListInfo.cIssuers;
    FindByIssuerPara.rgIssuer  = IssuerListInfo.aIssuers;

	pChainContext = NULL;

	while(TRUE)
	{
		// Find a certificate chain.
        pChainContext = CertFindChainInStore(m_hMyCertStore,
                                             X509_ASN_ENCODING,
                                             0,
                                             CERT_CHAIN_FIND_BY_ISSUER,
                                             &FindByIssuerPara,
                                             pChainContext);
        if(pChainContext == NULL)
        {
            TRACE("Error 0x%x finding cert chain\n", GetLastError());
            break;
        }
        TRACE("\ncertificate chain found\n");

		// Get pointer to leaf certificate context.
        pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;

		// Create schannel credential.
        m_SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
        m_SchannelCred.cCreds = 1;
        m_SchannelCred.paCred = &pCertContext;

		Status = m_pSSPI->AcquireCredentialsHandle(
			NULL,                   // Name of principal
			UNISP_NAME,           // Name of package
			SECPKG_CRED_OUTBOUND,   // Flags indicating use
			NULL,                   // Pointer to logon ID
			&m_SchannelCred,        // Package specific data
			NULL,                   // Pointer to GetKey() func
			NULL,                   // Value to pass to GetKey()
			&hCreds,                // (out) Cred Handle
			&tsExpiry);             // (out) Lifetime (optional)

		if(Status != SEC_E_OK)
		{
			TRACE("**** Error 0x%x returned by AcquireCredentialsHandle\n", Status);
            continue;
		}
		TRACE("\nnew schannel credential created\n");

		// Destroy the old credentials.
        m_pSSPI->FreeCredentialsHandle(phCreds);

		*phCreds = hCreds;

		//
        // As you can see, this sample code maintains a single credential
        // handle, replacing it as necessary. This is a little unusual.
        //
        // Many applications maintain a global credential handle that's
        // anonymous (that is, it doesn't contain a client certificate),
        // which is used to connect to all servers. If a particular server
        // should require client authentication, then a new credential 
        // is created for use when connecting to that server. The global
        // anonymous credential is retained for future connections to
        // other servers.
        //
        // Maintaining a single anonymous credential that's used whenever
        // possible is most efficient, since creating new credentials all
        // the time is rather expensive.
        //

        break;
	}
}

DWORD CSecurity::VerifyServerCertificate(PCCERT_CONTEXT pServerCert, PTSTR pszServerName, DWORD dwCertFlags)
{
	HTTPSPolicyCallbackData  polHttps;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;

	LPSTR rgszUsages[] = {  szOID_PKIX_KP_SERVER_AUTH,
                            szOID_SERVER_GATED_CRYPTO,
                            szOID_SGC_NETSCAPE };
    DWORD cUsages = sizeof(rgszUsages) / sizeof(LPSTR);

	PWSTR   pwszServerName	= NULL;
    DWORD   cchServerName	= 0;
    DWORD   Status;

	if(pServerCert == NULL)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        goto cleanup;
    }

	//
    // Convert server name to unicode.
    //

    if(pszServerName == NULL || _tcslen(pszServerName) == 0)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        goto cleanup;
    }
#ifndef UNICODE
	cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName, -1, NULL, 0);
    pwszServerName = (PWSTR)LocalAlloc(LMEM_FIXED, cchServerName * sizeof(WCHAR));
    if(pwszServerName == NULL)
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto cleanup;
    }
    cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName, -1, pwszServerName, cchServerName);
    if(cchServerName == 0)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        goto cleanup;
    }
#else
	pwszServerName = pszServerName;
#endif

	//
    // Build certificate chain.
    //

    ZeroMemory(&ChainPara, sizeof(ChainPara));
	ChainPara.cbSize = sizeof(ChainPara);
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
    ChainPara.RequestedUsage.Usage.cUsageIdentifier     = cUsages;
    ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

	if(!CertGetCertificateChain(
                            NULL,
                            pServerCert,
                            NULL,
                            pServerCert->hCertStore,
                            &ChainPara,
                            0,
                            NULL,
                            &pChainContext))
    {
        Status = GetLastError();
        TRACE("Error 0x%x returned by CertGetCertificateChain!\n", Status);
        goto cleanup;
    }

	//
    // Validate certificate chain.
    // 

    ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
    polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
    polHttps.dwAuthType         = AUTHTYPE_SERVER;
    polHttps.fdwChecks          = dwCertFlags;
    polHttps.pwszServerName     = pwszServerName;

    memset(&PolicyPara, 0, sizeof(PolicyPara));
    PolicyPara.cbSize            = sizeof(PolicyPara);
    PolicyPara.pvExtraPolicyPara = &polHttps;

    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

	if(!CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_SSL,
                            pChainContext,
                            &PolicyPara,
                            &PolicyStatus))
    {
        Status = GetLastError();
        TRACE("Error 0x%x returned by CertVerifyCertificateChainPolicy!\n", Status);
        goto cleanup;
    }

	if(PolicyStatus.dwError)
    {
        Status = PolicyStatus.dwError;
        DisplayWinVerifyTrustError(Status); 
        goto cleanup;

		// CERT_E_UNTRUSTEDROOT일때 테스트 해보자.. 먼지 모르겠지만..
#if 0
		CERT_PUBLIC_KEY_INFO msPubKey = {{0}};
		BOOL isMSTestRoot = FALSE;
		PCCERT_CONTEXT failingCert = pChainContext->rgpChain[PolicyStatus.lChainIndex]->rgpElement[PolicyStatus.lElementIndex]->pCertContext;
		DWORD i;
		CRYPT_DATA_BLOB keyBlobs[] = {
			{ sizeof(msTestPubKey1), msTestPubKey1 },
			{ sizeof(msTestPubKey2), msTestPubKey2 },
			{ sizeof(msTestPubKey3), msTestPubKey3 },
		};
		/* Check whether the root is an MS test root */
		for(int i=0; !isMSTestRoot && i<SIZEOF_ARRAY(keyBlobs); i++)
		{
			msPubKey.PublicKey.cbData = keyBlobs[i].cbData;
			msPubKey.PublicKey.pbData = keyBlobs[i].pbData;
			if(CertComparePublicKeyInfo(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				&failingCert->pCertInfo->SubjectPublicKeyInfo, &msPubKey))
				isMSTestRoot = TRUE;
		}
#endif
    }


    Status = SEC_E_OK;

cleanup:

    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }
#ifndef UNICODE
    if(pwszServerName)
    {
        LocalFree(pwszServerName);
    }
#endif

    return Status;
}

void CSecurity::DisplayCertChain(PCCERT_CONTEXT pServerCert, BOOL fLocal)
{
	TCHAR szName[1000];
    PCCERT_CONTEXT pCurrentCert;
    PCCERT_CONTEXT pIssuerCert;
    DWORD dwVerificationFlags;

	TRACE("\n");

    // display leaf name
    if(!CertNameToStr(pServerCert->dwCertEncodingType,
                      &pServerCert->pCertInfo->Subject,
                      CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                      szName, sizeof(szName)/sizeof(szName[0]) ))
    {
        TRACE("**** Error 0x%x building subject name\n", GetLastError());
    }

	if(fLocal)
    {
        TRACE("Client subject: %s\n", szName);
    }
    else
    {
        TRACE("Server subject: %s\n", szName);
    }

	if(!CertNameToStr(pServerCert->dwCertEncodingType,
                      &pServerCert->pCertInfo->Issuer,
                      CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                      szName, sizeof(szName)/sizeof(szName[0]) ))
	{
		TRACE("**** Error 0x%x building issuer name\n", GetLastError());
	}

	if(fLocal)
    {
        TRACE("Client issuer: %s\n", szName);
    }
    else
    {
        TRACE("Server issuer: %s\n\n", szName);
    }

	// display certificate chain
    pCurrentCert = pServerCert;
	while(pCurrentCert != NULL)
	{
		dwVerificationFlags = 0;
        pIssuerCert = CertGetIssuerCertificateFromStore(pServerCert->hCertStore,
                                                        pCurrentCert,
                                                        NULL,
                                                        &dwVerificationFlags);
        if(pIssuerCert == NULL)
        {
            if(pCurrentCert != pServerCert)
            {
                CertFreeCertificateContext(pCurrentCert);
            }
            break;
        }

		if(!CertNameToStr(pIssuerCert->dwCertEncodingType,
                          &pIssuerCert->pCertInfo->Subject,
                          CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                          szName, sizeof(szName)/sizeof(szName[0]) ))
        {
            TRACE("**** Error 0x%x building subject name\n", GetLastError());
        }
        TRACE("CA subject: %s\n", szName);

		if(!CertNameToStr(pIssuerCert->dwCertEncodingType,
                          &pIssuerCert->pCertInfo->Issuer,
                          CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                          szName, sizeof(szName)/sizeof(szName[0]) ))
        {
            TRACE("**** Error 0x%x building issuer name\n", GetLastError());
        }
        TRACE("CA issuer: %s\n\n", szName);

		if(pCurrentCert != pServerCert)
        {
            CertFreeCertificateContext(pCurrentCert);
        }
        pCurrentCert = pIssuerCert;
        pIssuerCert = NULL;
	}
}

void CSecurity::DisplayWinVerifyTrustError(DWORD Status)
{
	LPSTR pszName = NULL;

    switch(Status)
    {
    case CERT_E_EXPIRED:                pszName = "CERT_E_EXPIRED";                 break;
    case CERT_E_VALIDITYPERIODNESTING:  pszName = "CERT_E_VALIDITYPERIODNESTING";   break;
    case CERT_E_ROLE:                   pszName = "CERT_E_ROLE";                    break;
    case CERT_E_PATHLENCONST:           pszName = "CERT_E_PATHLENCONST";            break;
    case CERT_E_CRITICAL:               pszName = "CERT_E_CRITICAL";                break;
    case CERT_E_PURPOSE:                pszName = "CERT_E_PURPOSE";                 break;
    case CERT_E_ISSUERCHAINING:         pszName = "CERT_E_ISSUERCHAINING";          break;
    case CERT_E_MALFORMED:              pszName = "CERT_E_MALFORMED";               break;
    case CERT_E_UNTRUSTEDROOT:          pszName = "CERT_E_UNTRUSTEDROOT";           break;
    case CERT_E_CHAINING:               pszName = "CERT_E_CHAINING";                break;
    case TRUST_E_FAIL:                  pszName = "TRUST_E_FAIL";                   break;
    case CERT_E_REVOKED:                pszName = "CERT_E_REVOKED";                 break;
    case CERT_E_UNTRUSTEDTESTROOT:      pszName = "CERT_E_UNTRUSTEDTESTROOT";       break;
    case CERT_E_REVOCATION_FAILURE:     pszName = "CERT_E_REVOCATION_FAILURE";      break;
    case CERT_E_CN_NO_MATCH:            pszName = "CERT_E_CN_NO_MATCH";             break;
    case CERT_E_WRONG_USAGE:            pszName = "CERT_E_WRONG_USAGE";             break;
    default:                            pszName = "(unknown)";                      break;
    }

    TRACE("Error 0x%x (%s) returned by CertVerifyCertificateChainPolicy!\n", 
        Status, pszName);
}

void CSecurity::DisplayConnectionInfo(CtxtHandle *phContext)
{
	SECURITY_STATUS Status;
    SecPkgContext_ConnectionInfo ConnectionInfo;

    Status = m_pSSPI->QueryContextAttributes(phContext,
                                    SECPKG_ATTR_CONNECTION_INFO,
                                    (PVOID)&ConnectionInfo);

	if(Status != SEC_E_OK)
    {
        TRACE("Error 0x%x querying connection info\n", Status);
        return;
    }

	TRACE("\n");

	switch(ConnectionInfo.dwProtocol)
    {
        case SP_PROT_TLS1_CLIENT:
            TRACE("Protocol: TLS1\n");
            break;

        case SP_PROT_SSL3_CLIENT:
            TRACE("Protocol: SSL3\n");
            break;

        case SP_PROT_PCT1_CLIENT:
            TRACE("Protocol: PCT\n");
            break;

        case SP_PROT_SSL2_CLIENT:
            TRACE("Protocol: SSL2\n");
            break;

        default:
            TRACE("Protocol: 0x%x\n", ConnectionInfo.dwProtocol);
    }

    switch(ConnectionInfo.aiCipher)
    {
        case CALG_RC4: 
            TRACE("Cipher: RC4\n");
            break;

        case CALG_3DES: 
            TRACE("Cipher: Triple DES\n");
            break;

        case CALG_RC2: 
            TRACE("Cipher: RC2\n");
            break;

        case CALG_DES: 
        case CALG_CYLINK_MEK:
            TRACE("Cipher: DES\n");
            break;

        case CALG_SKIPJACK: 
            TRACE("Cipher: Skipjack\n");
            break;

        default: 
            TRACE("Cipher: 0x%x\n", ConnectionInfo.aiCipher);
    }

	TRACE("Cipher strength: %d\n", ConnectionInfo.dwCipherStrength);

    switch(ConnectionInfo.aiHash)
    {
        case CALG_MD5: 
            TRACE("Hash: MD5\n");
            break;

        case CALG_SHA: 
            TRACE("Hash: SHA\n");
            break;

        default: 
            TRACE("Hash: 0x%x\n", ConnectionInfo.aiHash);
    }

    TRACE("Hash strength: %d\n", ConnectionInfo.dwHashStrength);

    switch(ConnectionInfo.aiExch)
    {
        case CALG_RSA_KEYX: 
        case CALG_RSA_SIGN: 
            TRACE("Key exchange: RSA\n");
            break;

        case CALG_KEA_KEYX: 
            TRACE("Key exchange: KEA\n");
            break;

        case CALG_DH_EPHEM:
            TRACE("Key exchange: DH Ephemeral\n");
            break;

        default: 
            TRACE("Key exchange: 0x%x\n", ConnectionInfo.aiExch);
    }

    TRACE("Key exchange strength: %d\n", ConnectionInfo.dwExchStrength);
}

int CSecurity::Encrypt(char *pBuffer, size_t nLength, CBuffer *pBuf)
{
	SecPkgContext_StreamSizes Sizes;
	SECURITY_STATUS scRet;
	SecBufferDesc   Message;
	SecBuffer       Buffers[4];

	DWORD cbIoBufferLength;
	PBYTE pbMessage;
	DWORD cbMessage;

	scRet = m_pSSPI->QueryContextAttributes(&m_hContext, SECPKG_ATTR_STREAM_SIZES, &Sizes);
	if(scRet != SEC_E_OK)
	{
		TRACE("**** Error 0x%x reading SECPKG_ATTR_STREAM_SIZES\n", scRet);
		return scRet;
	}

	TRACE("\nHeader: %d, Trailer: %d, MaxMessage: %d\n",
		Sizes.cbHeader,
		Sizes.cbTrailer,
		Sizes.cbMaximumMessage);

	// Allocate a working buffer. The plaintext sent to EncryptMessage
	// should never be more than 'Sizes.cbMaximumMessage', so a buffer 
	// size of this plus the header and trailer sizes should be safe enough.

	cbIoBufferLength = Sizes.cbHeader + 
		Sizes.cbMaximumMessage +
		Sizes.cbTrailer;

	if(m_pbIoBuffer == NULL)
	{
		m_pbIoBuffer = (PBYTE)LocalAlloc(LMEM_FIXED, cbIoBufferLength);
		m_dwIoLength = cbIoBufferLength;
	}

	if(m_pbIoBuffer == NULL)
	{
		TRACE("**** Out of memory (2)\n");
		return SEC_E_INTERNAL_ERROR;
	}

	// Build the HTTP request offset into the data buffer by "header size"
	// bytes. This enables Schannel to perform the encryption in place,
	// which is a significant performance win.
	pbMessage = m_pbIoBuffer + Sizes.cbHeader;

	// Build HTTP request. Note that I'm assuming that this is less than
	// the maximum message size. If it weren't, it would have to be broken up.
	//strcpy((char*)pbMessage, pBuffer);

	// TODO:: 버퍼크기 관련처리할 것...
	memcpy((char*)pbMessage, pBuffer, nLength);

	cbMessage = nLength;

	// Encrypt the HTTP request.
	Buffers[0].pvBuffer     = m_pbIoBuffer;
	Buffers[0].cbBuffer     = Sizes.cbHeader;
	Buffers[0].BufferType   = SECBUFFER_STREAM_HEADER;

	Buffers[1].pvBuffer     = pbMessage;
	Buffers[1].cbBuffer     = cbMessage;
	Buffers[1].BufferType   = SECBUFFER_DATA;

	Buffers[2].pvBuffer     = pbMessage + cbMessage;
	Buffers[2].cbBuffer     = Sizes.cbTrailer;
	Buffers[2].BufferType   = SECBUFFER_STREAM_TRAILER;

	Buffers[3].BufferType   = SECBUFFER_EMPTY;

	Message.ulVersion       = SECBUFFER_VERSION;
	Message.cBuffers        = 4;
	Message.pBuffers        = Buffers;

	scRet = m_pSSPI->EncryptMessage(&m_hContext, 0, &Message, 0);

	if(FAILED(scRet))
	{
		TRACE("**** Error 0x%x returned by EncryptMessage\n", scRet);

		//LocalFree(IoBuffer);
		return scRet;
	}

	//pBuf->Append(m_pbIoBuffer, Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer);
	pBuf->Assign(m_pbIoBuffer, Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer);

#if 0

	// Send the encrypted data to the server.
	int nBytesSent = 0;
	int nTotalBytesSent = 0;
	int nSendBufferLen = Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer;

	do
	{
		nBytesSent = send(m_hSocket, (const char*)m_pbIoBuffer + nTotalBytesSent, nSendBufferLen - nTotalBytesSent, 0);
		if(nBytesSent == SOCKET_ERROR)
		{
			TRACE("Send Error: %d\n", WSAGetLastError());
			//LocalFree(IoBuffer);
			return -1;
		}

		nTotalBytesSent += nBytesSent;

	} while(nTotalBytesSent < nSendBufferLen);

#endif

	return NO_ERROR;
}

int CSecurity::Decrypt(char *pBuffer, size_t nLength, CBuffer *pBuf)
{
	int nResult = NO_ERROR;
	SECURITY_STATUS scRet = SEC_E_INCOMPLETE_MESSAGE;
	SecBuffer       Buffers[4];
	SecBuffer		*pDataBuffer = NULL;
	SecBuffer		*pExtraBuffer = NULL;
	SecBuffer       ExtraBuffer;
	SecBufferDesc	Message;
	DWORD			cbIoBuffer = 0;

	do
	{
	        // Attempt to decrypt the received data.
	        Buffers[0].pvBuffer     = pBuffer;// m_pbIoBuffer;
	        Buffers[0].cbBuffer		= nLength;
	        Buffers[0].BufferType	= SECBUFFER_DATA;
        
	        Buffers[1].BufferType   = SECBUFFER_EMPTY;
	        Buffers[2].BufferType   = SECBUFFER_EMPTY;
	        Buffers[3].BufferType   = SECBUFFER_EMPTY;
        
	        Message.ulVersion       = SECBUFFER_VERSION;
	        Message.cBuffers        = 4;
	        Message.pBuffers        = Buffers;
        
	        scRet = m_pSSPI->DecryptMessage(&m_hContext, &Message, 0, NULL);
	        if(scRet == SEC_E_INCOMPLETE_MESSAGE)
	        {
		        // The input buffer contains only a fragment of an
		        // encrypted record. Loop around and read some more
		        // data.
		        return ERROR_MORE_DATA;
	        }
        
        
	        // Server signalled end of session
	        if(scRet == SEC_I_CONTEXT_EXPIRED)
		        return SEC_I_CONTEXT_EXPIRED;
		        //return -1;
        
	        if( scRet != SEC_E_OK && 
		        scRet != SEC_I_RENEGOTIATE && 
		        scRet != SEC_I_CONTEXT_EXPIRED)
	        {
		        TRACE("**** Error 0x%x returned by DecryptMessage\n", (SECURITY_STATUS)scRet);
		        return scRet;
	        }
        
	        // Locate data and (optional) extra buffers.
	        pDataBuffer  = NULL;
	        pExtraBuffer = NULL;
	        for(int i = 1; i < 4; i++)
	        {
		        if(pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA)
		        {
			        pDataBuffer = &Buffers[i];
			        TRACE("Buffers[%d].BufferType = SECBUFFER_DATA\n",i);
		        }
		        if(pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA)
		        {
			        TRACE("Buffers[%d].BufferType = SECBUFFER_EXTRA\n",i);
			        pExtraBuffer = &Buffers[i];
		        }
	        }
        
	        // Display or otherwise process the decrypted data.
	        if(pDataBuffer)
	        {
		        //TRACE("Decrypted data: %d bytes\n", pDataBuffer->cbBuffer); 
		        *(((char*)pDataBuffer->pvBuffer) + pDataBuffer->cbBuffer) = 0;
		        //TRACE(">> %s\n", (char*)pDataBuffer->pvBuffer);
        
		        //nResult = ProcessRecv((char*)pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);
		        //if(nResult == NO_ERROR)
		        //	bDone = TRUE;
		        pBuf->Append(pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);
	        }
        
	        // Move any "extra" data to the input buffer.
	        if(pExtraBuffer)
	        {
		        MoveMemory(m_pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
		        cbIoBuffer = pExtraBuffer->cbBuffer;
        
			pBuffer = (char*)m_pbIoBuffer;
			nLength = cbIoBuffer;

			continue;
	        }
	        else
	        {
		        cbIoBuffer = 0;
	        }
        
	        if(scRet == SEC_I_RENEGOTIATE)
	        {
		        // The server wants to perform another handshake
		        // sequence.
        
		        TRACE("Server requested renegotiate!\n");
        
		        scRet = ClientHandshakeLoop(m_hSocket, 
			        &m_hClientCreds, 
			        &m_hContext, 
			        FALSE, 
			        &ExtraBuffer);
		        if(scRet != SEC_E_OK)
		        {
			        return scRet;
		        }
        
		        // Move any "extra" data to the input buffer.
		        if(ExtraBuffer.pvBuffer)
		        {
			        MoveMemory(m_pbIoBuffer, ExtraBuffer.pvBuffer, ExtraBuffer.cbBuffer);
			        cbIoBuffer = ExtraBuffer.cbBuffer;
        
				        pBuffer = (char*)m_pbIoBuffer;
				        nLength = cbIoBuffer;
        
				        continue;
		        }
	        }

		break;

	} while(1);

	return NO_ERROR;
}

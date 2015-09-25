#include "Common.h"
#include "crypt.h"

CCrypt::CCrypt(void)
{
	m_hCryptProv = NULL;
}

CCrypt::~CCrypt(void)
{
	if(m_hCryptProv)
		CryptReleaseContext(m_hCryptProv, 0);
}

BOOL CCrypt::Init()
{
	if(m_hCryptProv)
		return TRUE;
	
	if(!CryptAcquireContext(&m_hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, NULL))
	{
		DWORD dwErr = GetLastError();
		if(dwErr == NTE_BAD_KEYSET)
		{
			if(CryptAcquireContext(&m_hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET))
				return TRUE;
		}

		return FALSE;
	}

	return TRUE;
}

BOOL CCrypt::Encrypt(const char *src, int srcLen, char *password, char **dst, int *dstLen)
{

	HCRYPTKEY hKey;
	HCRYPTHASH hHash;

	DWORD dwLength = srcLen;
	*dstLen = -1;

	if(!CryptCreateHash(m_hCryptProv, CALG_MD5, 0, 0, &hHash))
		return FALSE;

	if(!CryptHashData(hHash, (BYTE*)password, (DWORD)strlen(password), 0))
	{
		CryptDestroyHash(hHash);
		return FALSE;
	}

	if(!CryptDeriveKey(m_hCryptProv, ENCRYPT_ALGORITHM, hHash, KEYLENGTH, &hKey))
	{
		CryptDestroyHash(hHash);
		return FALSE;
	}

	CryptDestroyHash(hHash);
	hHash = 0;

	*dst = new char[dwLength + 1];

	memset(*dst, 0, dwLength + 1);
	memcpy(*dst, src, dwLength);

	if(!CryptEncrypt(hKey, 0, TRUE, 0, (BYTE*)*dst, &dwLength, dwLength))
	{
		delete [] *dst;
		*dst = NULL;

		return FALSE;
	}

	*dstLen = dwLength;

	CryptDestroyKey(hKey);

	return TRUE;
	
}

BOOL CCrypt::Decrypt(const char *src, int srcLen, char *password, char **dst, int *dstLen)
{
	HCRYPTKEY hKey;
	HCRYPTHASH hHash;

	DWORD dwLength = srcLen;
	*dstLen = -1;

	if(!CryptCreateHash(m_hCryptProv, CALG_MD5, 0, 0, &hHash))
		return FALSE;

	if(!CryptHashData(hHash, (BYTE*)password, (DWORD)strlen(password), 0))
	{
		CryptDestroyHash(hHash);
		return FALSE;
	}

	if(!CryptDeriveKey(m_hCryptProv, ENCRYPT_ALGORITHM, hHash, KEYLENGTH, &hKey))
	{
		CryptDestroyHash(hHash);
		return FALSE;
	}

	CryptDestroyHash(hHash);
	hHash = 0;

	*dst = new char[dwLength + 1];
	memset(*dst, 0, dwLength + 1);
	memcpy(*dst, src, dwLength);

	if(!CryptDecrypt(hKey, 0, TRUE, 0, (BYTE*)*dst, &dwLength))
	{
		delete [] *dst;
		*dst = NULL;

		return FALSE;
	}

	*dstLen = dwLength;
	CryptDestroyKey(hKey);

	return TRUE;

}

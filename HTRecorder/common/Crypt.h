#pragma once

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

#include <wincrypt.h>
#define KEYLENGTH			0x00800000
#define ENCRYPT_ALGORITHM	CALG_RC4
#define ENCRYPT_BLOCK_SIZE	8

class CCrypt
{
public:
	CCrypt(void);
	~CCrypt(void);

	BOOL Init();
	BOOL Encrypt(const char *src, int srcLen, char *password, char **dst, int *dstLen);
	BOOL Decrypt(const char *src, int srcLen, char *password, char **dst, int *dstLen);

protected:
	HCRYPTPROV m_hCryptProv;
};

#pragma once

class CStringLineParser
{
private:
	inline void SetAt(unsigned int n, const char &c);
	const char *m_ptr;
	char *m_pBuffer;
	size_t m_nBufferLength;

public:
	CStringLineParser();
	CStringLineParser(const char* pszStr);
	~CStringLineParser(void);

	static BYTE sEOLMask[];
	static BYTE sWhitespaceMask[];
	static BYTE sEOLWhitespaceMask[];
	static BYTE sCaseInsensitiveMask[];
	static BYTE sDigitMask[];
	static BYTE sWordMask[];

	void SetString(const char *pszStr);
	BOOL GoNextLine();
	BOOL GoNextWord(char c);
    
	int CopyUntil(char *pDest, BYTE *inMask);
    void ConsumeLength(size_t nLen);
	void ConsumeUntil(BYTE *inMask);
	void ConsumeUntil(char c);
	int ConsumeInteger();
	float ConsumeFloat();

	const char* GetString() const;
	const char* GetPtr() const;

	BOOL NumEqualIgnoreCase(const char* pszCompare);
	BOOL NumEqualIgnoreCase(const char* pszCompare, size_t nCompareLen);

	static int Find(const char *pszSrc, size_t SrcLen, const char *pszStr, size_t StrLen);
	static int FindIgnoreCase(const char *pszSrc, size_t SrcLen, const char *pszStr, size_t StrLen);


};

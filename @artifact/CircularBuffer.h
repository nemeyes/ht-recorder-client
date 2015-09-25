/**
 * @file CircularBuffer.h
 * @desc
 * Declaration of the CCircularBuffer class.
 **/
#ifndef __MH_CIRCULARBUFFER__
#define __MH_CIRCULARBUFFER__

/**
 * @brief CircularBuffer Class.
 * 
 * Circular buffer is used to store data.
 */
class CCircularBuffer
{
public:
	CCircularBuffer(void);
	CCircularBuffer(int size);
	~CCircularBuffer();

private:
	BYTE  *_pBuffer;
	int    _Size, _In, _Out;
	BOOL   _bReadHeader;      // TRUE : header를 읽어라
	                           // incomming TCP data buffer로 쓰기 위해 필요
	//HANDLE _HanNomManCBuffer; // 1 thread at a time
	CRITICAL_SECTION _lock;

public:
	int Read(BYTE *dest, int length);
	int Write(BYTE *src, int length);
	int Peek(BYTE *dest, int length);
	int Dump(int length);
	int MoveData(CCircularBuffer *src, int length);

	int FillFromFile(FILE *file, int length);
	int Write2File(FILE *file);

	int   UsedSpace(void);
	int   FreeSpace(void);
	float FreeSpaceRatio(void);

	int  MovePos(int add);
	int  Search(const char* str);
	// Hard Coded Searcher
	int  Search2(int *pos);

	void SetBufferSize(int size);
	int  GetBufferSize(void) { return _Size; }
	void Reset(void);

	BOOL IsReadHeader(void)     {return _bReadHeader;}
	void SetReadHeader(BOOL rh) {_bReadHeader = rh;}

	void Lock() { 	EnterCriticalSection( &_lock ); }
	void Unlock() { LeaveCriticalSection( &_lock ); }
};

#endif
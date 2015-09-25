#pragma once

#include <windows.h>
#include <process.h>

#include "CircularBuffer.h"

#define SAFE_DELETE(p) if((p)) { delete (p); p = NULL; }
#define DEFAULT_BUFFER_SIZE 1024*1024
#define NUM_FRAMES 60
#define GET_NEXT_16BIT_SEQUENCE(x)  ((unsigned int)((x)+1+0x10000) & 0xFFFF)

typedef struct
{
	unsigned int av;       // A/V
	unsigned int size;     // SIZE
	unsigned int sequence; // SEQUENCE
	unsigned int key;
	unsigned int info1;
	unsigned int info2;
	unsigned int info3;
	unsigned char* data;     // data pointer
} FRAME_DATA2;

class CFrameBuffer2
{
public:
	CFrameBuffer2( void );
	CFrameBuffer2( int size );
	~CFrameBuffer2( void );

private:
	int _Size;
	int _MaxSize;
	int _Seed;
	int _First;
	int _Last;
	int _WriteSeq;
	int _ReadSeq;

	CRITICAL_SECTION _lock;

	unsigned char *_pBuffer;
	unsigned char *_pCurr;
	// For H264
	int  _spsSize, _ppsSize;
	// Common
	int  _HeaderSize;
	unsigned char *_pHeader;
	FRAME_DATA2 _Frames[NUM_FRAMES];

public:
	// For H264.
	int  SaveSps(unsigned char* sps, int spsSize);
	int  SavePps(unsigned char* pps, int ppsSize);

	// Common
	int  SpsSize( void ) { return _spsSize; }
	int  PpsSize( void ) { return _ppsSize; }
	int  HeaderSize( void ) { return _HeaderSize; }
	int  SaveHeader(unsigned char* head, int headSize);
	int  Write(unsigned char* head, int headSize, unsigned char* data, int dataSize, int addheader=0);
	// VIDEO: info1->width, info2->height, info3->codec
	// AUDIO: info1->samplerate, info2->channel, info3->codec
	int  WriteFromCBuffer(CCircularBuffer* pCBuffer, int size, int info1, int info2, int info3, int key);
	int  Read(unsigned char* data, int *info1=NULL, int *info2=NULL, int *info3=NULL, int *key=NULL);
	int  NumFrames( void );
	void ResetFrameBuffer(void);

	void Lock() { 	EnterCriticalSection( &_lock ); }
	void Unlock() { LeaveCriticalSection( &_lock ); }
};
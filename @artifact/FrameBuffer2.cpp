#include "StdAfx.h"
#include "FrameBuffer2.h"

CFrameBuffer2::CFrameBuffer2( void )
{
	::InitializeCriticalSection( &_lock );
	_pBuffer = (unsigned char*)malloc(DEFAULT_BUFFER_SIZE);
	_pCurr   = _pBuffer;
	_Size    = DEFAULT_BUFFER_SIZE;
	_First = _Last = -1;
	_ReadSeq = _WriteSeq = -1;
	_spsSize = _ppsSize = _HeaderSize = 0;
	memset(_Frames, 0, sizeof(FRAME_DATA2)*NUM_FRAMES);
	_pHeader = (unsigned char*)malloc(1024);
}

CFrameBuffer2::CFrameBuffer2( int size )
{
	::InitializeCriticalSection( &_lock );
	_pBuffer = (unsigned char*)malloc(size);
	_pCurr   = _pBuffer;
	_Size    = size;
	_First = _Last = -1;
	_ReadSeq = _WriteSeq = -1;
	_spsSize = _ppsSize = _HeaderSize = 0;
	memset(_Frames, 0, sizeof(FRAME_DATA2)*NUM_FRAMES);
	_pHeader = (unsigned char*)malloc(1024);
}

CFrameBuffer2::~CFrameBuffer2( void )
{
	Lock();

	if(_pBuffer) free(_pBuffer); _pBuffer = NULL;
	if(_pHeader) free(_pHeader); _pHeader = NULL;
	
	_Size = 0;

	Unlock();

	::DeleteCriticalSection( &_lock );
}

void CFrameBuffer2::ResetFrameBuffer(void)
{
	Lock();

	_pCurr   = _pBuffer;
	_First = _Last = -1;
	_ReadSeq = _WriteSeq = -1;
	_spsSize = _ppsSize = _HeaderSize = 0;
	memset(_Frames, 0, sizeof(FRAME_DATA2)*NUM_FRAMES);

	Unlock();
}
int CFrameBuffer2::SaveSps(unsigned char* sps, int spsSize)
{
	unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};

	if(sps==NULL || spsSize==0) return 0;

	if(spsSize > 256) return 0;

	Lock();

	if(_spsSize<spsSize && _ppsSize!=0)
		memmove(_pHeader+spsSize+4, _pHeader+_spsSize, _ppsSize);

	memcpy(_pHeader, start_code, 4);
	memcpy(_pHeader+4, sps, spsSize);

	_spsSize = spsSize+4;
	_HeaderSize = _spsSize + _ppsSize;

	//printf("Save SPS %d Bytes\n", _spsSize);

	Unlock();

	return 1;
}

int CFrameBuffer2::SavePps(unsigned char* pps, int ppsSize)
{
	unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};

	if(pps==NULL || ppsSize==0) return 0;

	if(ppsSize > 256) return 0;

	Lock();

	memcpy(_pHeader+_spsSize, start_code, 4);
	memcpy(_pHeader+_spsSize+4, pps, ppsSize);

	_ppsSize = ppsSize+4;
	_HeaderSize = _spsSize + _ppsSize;

	Unlock();
	//printf("Save PPS %d Bytes\n", _ppsSize);

	return 1;
}

int CFrameBuffer2::SaveHeader(unsigned char* head, int headSize)
{
	if(head==NULL || headSize<=0) return 0;

	Lock();

	memcpy(_pHeader, head, headSize);
	_HeaderSize = headSize;

	Unlock();

	return 1;
}

int CFrameBuffer2::Write(unsigned char* head, int headSize, unsigned char* data, int dataSize, int addheader)
{
	FRAME_DATA2 *pFrame;
	int i, first, last, framesize, size=0, pos=0;
	unsigned char* oldpos;
	unsigned char* newpos;
	unsigned char* datapos;

	// 2013.08.14 - buffering bug patch by fullluck
	unsigned char* endpos = NULL;

	if(data==NULL || dataSize<=0) return 0;
	if(headSize>0 && head==NULL) return 0;

	if(headSize<=0) headSize = 0;
	if(dataSize<=0) dataSize = 0;

	Lock();

	if(addheader && _HeaderSize>0) size += _HeaderSize;

	size+=headSize;
	size+=dataSize;

	framesize = ((size + 3)>>2)<<2;

	// Rewind
	// 2013.08.14 - buffering bug patch by fullluck
#if 0
	if(_pCurr - _pBuffer + framesize > _Size)
		_pCurr = _pBuffer;
#else
	if(_pCurr - _pBuffer + framesize > _Size)
	{
		endpos = _pCurr;
		_pCurr = _pBuffer;
	}
	else
	{
		endpos = NULL;
	}
#endif

	if(addheader && _HeaderSize>0)
	{
		memcpy(_pCurr+pos, _pHeader, _HeaderSize);
		pos += _HeaderSize;
	}

	if(headSize>0)
	{
		memcpy(_pCurr+pos, head, headSize);
		pos += headSize;
	}

	if(dataSize>0)
	{
		memcpy(_pCurr+pos, data, dataSize);
		pos += dataSize;
	}

	oldpos = _pCurr;
	newpos = oldpos + framesize;

	first = _First;
	last  = (_Last+1)%NUM_FRAMES;

	if     ( first==-1  ) first = 0;
	else if( first==last) first = (last+1)%NUM_FRAMES;

	// 2013.08.14 - buffering bug patch by fullluck
#if 0
	for(i=first; i!=last; i=(i+1)%NUM_FRAMES)
	{
		datapos = _Frames[i].data;
		if(oldpos <= datapos && datapos < newpos)
			first = (i+1)%NU_FRAMES;
		else
			break;
	}
#else
	for(i=first; i!=last; i=(i+1)%NUM_FRAMES)
	{
		datapos = _Frames[i].data;
		if( (oldpos <= datapos && datapos < newpos) || (endpos!=NULL && endpos<=datapos) )
			first = (i+1)%NUM_FRAMES;
		else
			break;
	}
#endif

	_First = first;

	pFrame = &_Frames[last];
	memset(pFrame, 0, sizeof(FRAME_DATA2));

	if(_WriteSeq<0) _WriteSeq = 0;
	pFrame->sequence = _WriteSeq;
	pFrame->data     = _pCurr;
	pFrame->size     = size;

	_Last = last;

	_pCurr += framesize;
	_WriteSeq = GET_NEXT_16BIT_SEQUENCE(_WriteSeq);

	Unlock();

	return 1;
}

int  CFrameBuffer2::WriteFromCBuffer(CCircularBuffer* pCBuffer, int size, int info1, int info2, int info3, int key)
{
	FRAME_DATA2 *pFrame;
	int i, first, last, framesize;
	unsigned char* oldpos;
	unsigned char* newpos;
	unsigned char* datapos;

	if(pCBuffer==NULL || size<=0) return 0;

	Lock();

	// alignment
	framesize = ((size + 3)>>2)<<2;
	////////////////////////////////////////////////////////////////////////////////////
	// Header Validation Added - 2013.06.19
	if(framesize > _Size)
	{
		Unlock();
		return 0;
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Rewind
	if(_pCurr - _pBuffer + framesize > _Size)
		_pCurr = _pBuffer;

	if(size>0)
		pCBuffer->Read(_pCurr, size);

////////////////////////////////////////////////////////////
#if 1
	//20130621
	int dec = 0;
	do {
		/*printf(" 5: %c, 4: %c, 3: %c, 2: %c, 1: %c\n",
			*(char*)(_pCurr + size - 5),
			*(char*)(_pCurr + size - 4),
			*(char*)(_pCurr + size - 3),
			*(char*)(_pCurr + size - 2),
			*(char*)(_pCurr + size - 1));*/
			
		if( *(char*)(_pCurr + size - 4)=='s' && *(char*)(_pCurr + size - 3)=='u' && *(char*)(_pCurr + size - 2)=='f' && *(char*)(_pCurr + size - 1)=='x') { dec = 4; break; }
		if( *(char*)(_pCurr + size - 5)=='s' && *(char*)(_pCurr + size - 4)=='u' && *(char*)(_pCurr + size - 3)=='f' && *(char*)(_pCurr + size - 2)=='x') { dec = 5; break; }
		TRACE("broken sufx\n");
		Unlock();
		return 0;
	} while(0);
	//printf("OK\n");
	size = size - dec;
	framesize = ((size + 3)>>2)<<2;
#endif
////////////////////////////////////////////////////////////

	oldpos = _pCurr;
	newpos = oldpos + framesize;

	first = _First;
	last  = (_Last+1)%NUM_FRAMES;

	if     ( first==-1  ) first = 0;
	else if( first==last) first = (last+1)%NUM_FRAMES;

	for(i=first; i!=last; i=(i+1)%NUM_FRAMES)
	{
		datapos = _Frames[i].data;
		if(oldpos <= datapos && datapos < newpos)
			first = (i+1)%NUM_FRAMES;
		else
			break;
	}

	_First = first;

	pFrame = &_Frames[last];
	memset(pFrame, 0, sizeof(FRAME_DATA2));

	if(_WriteSeq<0) _WriteSeq = 0;
	pFrame->sequence = _WriteSeq;
	pFrame->data     = _pCurr;
	pFrame->size     = size;
	pFrame->info1    = info1;
	pFrame->info2    = info2;
	pFrame->info3    = info3;
	pFrame->key      = key;

	_Last = last;

	_pCurr += framesize;
	_WriteSeq = GET_NEXT_16BIT_SEQUENCE(_WriteSeq);

	Unlock();

	return 1;
}

int CFrameBuffer2::Read(unsigned char* data, int *info1, int *info2, int *info3, int *key)
{
	FRAME_DATA2 *pFrame;
	int fseq, lseq, first, last, size=0;
	int match, index;
	int dbg = 0;

	if(data==NULL) return 0;

	Lock();

	first = _First;
	last  = (_Last - 1 + NUM_FRAMES)%NUM_FRAMES;

	if(last < 0 || first < 0)
	{
		Unlock();
		return 0;
	}

	if(_ReadSeq<0)
	{
		pFrame = &_Frames[last];
		size = pFrame->size;
		_ReadSeq = pFrame->sequence;
		if(size < 0)
		{
			Unlock();
			return 0;
		}
		memcpy(data, pFrame->data, size);
		if(info1) *info1 = pFrame->info1;
		if(info2) *info2 = pFrame->info2;
		if(info3) *info3 = pFrame->info3;
		if(key)   *key   = pFrame->key;
		Unlock();
		return size;
	}

	fseq = _Frames[first].sequence;
	lseq = _Frames[last].sequence;

	if(fseq==lseq || _ReadSeq==lseq)
	{
		Unlock();
		return 0;
	}
	// Check out of order
	// Due to Circle Compare fseq can be larger than lseq
	if(fseq<lseq)
	{
		if(_ReadSeq<fseq || _ReadSeq>lseq)
		{
			pFrame = &_Frames[last];
			size = pFrame->size;
			_ReadSeq = pFrame->sequence;
			if(size < 0)
			{
				Unlock();
				return 0;
			}
			memcpy(data, pFrame->data, size);
			if(info1) *info1 = pFrame->info1;
			if(info2) *info2 = pFrame->info2;
			if(info3) *info3 = pFrame->info3;
			if(key)   *key   = pFrame->key;
			Unlock();
			return size;
		}
	}
	else if(fseq>lseq)
	{
		if(_ReadSeq>lseq && _ReadSeq<fseq)
		{
			pFrame = &_Frames[last];
			size = pFrame->size;
			_ReadSeq = pFrame->sequence;
			if(size < 0)
			{
				Unlock();
				return 0;
			}
			memcpy(data, pFrame->data, size);
			if(info1) *info1 = pFrame->info1;
			if(info2) *info2 = pFrame->info2;
			if(info3) *info3 = pFrame->info3;
			if(key)   *key   = pFrame->key;
			Unlock();
			return size;
		}
	}

	match = -1;
	index = last + 1;

	do {
		index = ( index - 1 + NUM_FRAMES )%NUM_FRAMES;
		if( _Frames[ index ].sequence == _ReadSeq )
		{
			match = index;
			break;
		}
	} while( index != first );

	if( match < 0 )
	{
		Unlock();
		return 0;
	}

	if(match != last)
	{
		match = (match+1)%NUM_FRAMES;
		pFrame = &_Frames[match];
		size = pFrame->size;
		_ReadSeq = pFrame->sequence;

		if(size < 0)
		{
			Unlock();
			return 0;
		}
		memcpy(data, pFrame->data, size);
		if(info1) *info1 = pFrame->info1;
		if(info2) *info2 = pFrame->info2;
		if(info3) *info3 = pFrame->info3;
		if(key)   *key   = pFrame->key;
		Unlock();
		return size;
	}
	else
	{
		Unlock();
		return 0;
	}

	Unlock();

	return size;
}

int CFrameBuffer2::NumFrames( void )
{
	int frames = 0;

	Lock();
	if(_Last < 0 && _First < 0)
		frames = 0;
	if(_First<0)
		frames = _Last + 1;
	else
		frames = (_Last - _First + NUM_FRAMES ) % NUM_FRAMES;
	Unlock();

	return frames;
}
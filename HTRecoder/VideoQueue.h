#pragma once

#include "queue"
#include "Recorder/RecorderIF.h"



struct VIDEO_DATA
{
	RECORD_TIME rec_time;
	UINT size;
	//UINT year;
	//UINT month;
	//UINT day;
	//UINT hour;
	//UINT minute;
	//UINT second;
	BYTE *data;

	VIDEO_DATA( VOID )
	{

		size = 0;
		data = NULL;
	}
};

class CVideoQueue
{
public:

	queue<VIDEO_DATA *> m_Queue;
	CRITICAL_SECTION m_lock;

	void AddData(VIDEO_DATA *data);
	VIDEO_DATA * GetData();
	int GetCount();
	void releaseQueueData();

	CVideoQueue(void);
	~CVideoQueue(void);
};


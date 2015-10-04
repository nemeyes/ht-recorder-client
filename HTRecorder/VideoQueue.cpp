#include "stdafx.h"
#include "VideoQueue.h"


CVideoQueue::CVideoQueue(void)
{
	::InitializeCriticalSection( &m_lock );
}


CVideoQueue::~CVideoQueue(void)
{
	EnterCriticalSection( &m_lock );
	while( !m_Queue.empty() )
	{
		VIDEO_DATA * videoData;
		videoData = m_Queue.front();
		if( videoData )
		{
			m_Queue.pop();
			if(videoData)
			{
				if(videoData->data) av_free (videoData->data); 
				videoData->data = NULL;
			}
			delete videoData;
			videoData = NULL;
		}
	}
	LeaveCriticalSection( &m_lock );

	::DeleteCriticalSection( &m_lock );
}

void CVideoQueue::releaseQueueData()
{
	EnterCriticalSection( &m_lock );
	while( !m_Queue.empty() )
	{
		VIDEO_DATA * videoData;
		videoData = m_Queue.front();
		if( videoData )
		{
			m_Queue.pop();
			if(videoData)
			{
				if(videoData->data) av_free (videoData->data); 
				videoData->data = NULL;
			}
			delete videoData;
			videoData = NULL;
		}
	}
	LeaveCriticalSection( &m_lock );
}

void CVideoQueue::AddData(VIDEO_DATA * data)
{
	EnterCriticalSection( &m_lock );
	m_Queue.push(data);
	LeaveCriticalSection( &m_lock );
}

VIDEO_DATA * CVideoQueue::GetData()
{
	VIDEO_DATA * data;
	EnterCriticalSection( &m_lock );
	if(!m_Queue.empty())
	{
#if 1
		int size = m_Queue.size();
		
		if(size > 60) 
		{
			while( !m_Queue.empty() )
			{
				VIDEO_DATA * videoData;
				videoData = m_Queue.front();
				if( videoData )
				{
					m_Queue.pop();
					if(videoData)
					{
						if(videoData->data) 
						{
							av_free (videoData->data); 
							videoData->data = NULL;
						}
						delete videoData;
						videoData = NULL;
					}
				}
			}
			LeaveCriticalSection( &m_lock );
			return NULL;
		}

#endif

		data = m_Queue.front();
		m_Queue.pop();

		LeaveCriticalSection( &m_lock );
		return data;
	}
	else
	{
		LeaveCriticalSection( &m_lock );
		return NULL;
	}

}

int CVideoQueue::GetCount()
{
	::EnterCriticalSection( &m_lock );
	int size = m_Queue.size();
	::LeaveCriticalSection( &m_lock );
	return size;
}
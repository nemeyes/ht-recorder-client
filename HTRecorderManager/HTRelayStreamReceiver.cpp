#include "stdafx.h"
#include "HTRelayStreamReceiver.h"

HTRelayStreamReceiver::HTRelayStreamReceiver(HTRecorder * service)
	: m_bConntected(FALSE)
	, m_bRun(FALSE)
	, m_HTRecorder(service)
	, m_pVideoExtraData(0)
	, m_nVideoExtraDataSize(0)
	, m_pAudioExtraData(0)
	, m_nAudioExtraDataSize(0)
{
}

HTRelayStreamReceiver::~HTRelayStreamReceiver( VOID )
{
	if (m_pVideoExtraData)
	{
		free(m_pVideoExtraData);
		m_pVideoExtraData = 0;
	}
	if (m_pAudioExtraData)
	{
		free(m_pAudioExtraData);
		m_pAudioExtraData = 0;
	}

	m_nVideoExtraDataSize = 0;
	m_nAudioExtraDataSize = 0;
}

VOID HTRelayStreamReceiver::Start( VOID )
{
	m_bRun = TRUE;
}

VOID HTRelayStreamReceiver::Stop( VOID )
{
	m_bRun = FALSE;
}

VOID HTRelayStreamReceiver::SetStreamInfo( CString UUID )
{
	m_relayUUID = UUID;
}

VOID HTRelayStreamReceiver::GetTime( UINT date, UINT time, UINT& year, UINT& month, UINT& day, UINT& hour, UINT& minute, UINT& second )
{
	TCHAR szDate[50]={0};
	TCHAR szTime[50]={0};
	TCHAR szYear[10] = {0};
	TCHAR szMonth[10] = {0};
	TCHAR szDay[10] = {0};
	TCHAR szHour[10] = {0};
	TCHAR szMinute[10] = {0};
	TCHAR szSecond[10] = {0};

	_sntprintf( szDate, sizeof(szDate)-1, _T("%.8d"), date );
	_sntprintf( szTime, sizeof(szTime)-1, _T("%.9d"), time );

	memcpy( szYear, szDate, sizeof(TCHAR)*4 );
	memcpy( szMonth, szDate+4, sizeof(TCHAR)*2 );
	memcpy( szDay, szDate+6, sizeof(TCHAR)*2 );
	memcpy( szHour, szTime, sizeof(TCHAR)*2 );
	memcpy( szMinute, szTime+2, sizeof(TCHAR)*2 );
	memcpy( szSecond, szTime+4, sizeof(TCHAR)*2 );

	year = _ttoi( szYear );
	month = _ttoi( szMonth );
	day = _ttoi( szDay );
	hour = _ttoi( szHour );
	minute = _ttoi( szMinute );
	second = _ttoi( szSecond );
}

VOID HTRelayStreamReceiver::OnNotifyMessage( LiveNotifyMsg* pNotify )
{
	if(pNotify->nMessage == LIVE_STREAM_CONNECT) 
	{
		if(pNotify->nError == enLIVE_NO_ERROR)
		{
			TRACE( _T("릴레이 연결 시작 \n") );
			m_bConntected = TRUE;
		}
	} 
	else if(pNotify->nMessage == LIVE_STREAM_DISCONNECT) 
	{
		TRACE( _T("릴레이 연결 종료 \n") );
		m_bConntected = FALSE;
	}
}

VOID HTRelayStreamReceiver::OnReceive( LPStreamData Data )
{
	if(!m_bRun) 
		return;

	if( Data->Type==FRAME_XML )
	{
		RS_RELAY_INFO_T rlInfo;
		rlInfo.szID = m_relayUUID;
		m_HTRecorder->GetRelayInfo( (VOID*)Data->pData, &rlInfo );
		if( rlInfo.isDisconnected )
		{
			TRACE( _T("비정상 서버 종료 \n") );
		}
		if( rlInfo.isNoDevice )
		{
			TRACE( _T("Relay할 카메라가 존재하지 않음\n") );
		}
	}
	else if( Data->Type==FRAME_VIDEO_CODEC )
	{
		RS_VIDEO_CODEC_T *pVideo = (RS_VIDEO_CODEC_T*)Data->pData;
		switch( pVideo->type )
		{
		case RS_VIDEO_MJPEG :
			m_relayInfo.videoType = AVMEDIA_JPEG;
			break;
		case RS_VIDEO_MPEG4 :
			m_relayInfo.videoType = AVMEDIA_MPEG4;
			break;
		case RS_VIDEO_H264 :
			m_relayInfo.videoType = AVMEDIA_H264;
			break;
		default:
			m_relayInfo.videoType = AVMEDIA_UNKNOWN;
			break;
		}

		m_relayInfo.stream = pVideo->stream;
		m_relayInfo.szVideoSize.cx = pVideo->width;
		m_relayInfo.szVideoSize.cy = pVideo->height;
		if( pVideo->extra_size>0 )
		{
			RS_STREAM_BUFFER_T *p = new RS_STREAM_BUFFER_T( Data->pData, Data->nDataSize );
			p->dwChannel = Data->nChannel;
			p->nLength = Data->nDataSize;
			p->pBuffer = (BYTE*) p->Buffer.GetPtr();
			p->nWidth = m_relayInfo.szVideoSize.cx;
			p->nHeight = m_relayInfo.szVideoSize.cy;
			p->br = 0;
			p->sr = 0;
			p->mediaType = m_relayInfo.videoType;
			p->tReceived = Data->tReceived;
			p->nFrameType = CODED_SPS;

			m_nVideoType = p->mediaType;

			if (m_pVideoExtraData)
			{
				free(m_pVideoExtraData);
				m_pVideoExtraData = 0;

			}
			m_pVideoExtraData = (BYTE*)malloc(pVideo->extra_size);
			memcpy(m_pVideoExtraData, (BYTE*)(p->pBuffer + sizeof(RS_VIDEO_CODEC_T)), pVideo->extra_size);
			m_nVideoExtraDataSize = pVideo->extra_size;

			delete p;
		}
	}
	else if( Data->Type==FRAME_AUDIO_CODEC )
	{
		RS_AUDIO_CODEC_T *pAudio = (RS_AUDIO_CODEC_T*)Data->pData;
		switch(pAudio->type)
		{
			case RS_AUDIO_PCM8:
			case RS_AUDIO_PCM16:
				m_relayInfo.audioType = AVMEDIA_PCM;
				break;
			case RS_AUDIO_G711U:
				m_relayInfo.audioType = AVMEDIA_PCMU;
				break;
			case RS_AUDIO_G711A:
				m_relayInfo.audioType = AVMEDIA_PCMA;
				break;
			case RS_AUDIO_G726:
				{
					if( pAudio->bit_rate==16000 )
						m_relayInfo.audioType = AVMEDIA_G726_16;
					else if( pAudio->bit_rate==24000 )
						m_relayInfo.audioType = AVMEDIA_G726_24;
					else if( pAudio->bit_rate==32000 )
						m_relayInfo.audioType = AVMEDIA_G726_32;
					else if( pAudio->bit_rate==40000 )
						m_relayInfo.audioType = AVMEDIA_G726_40;
					else
						m_relayInfo.audioType = AVMEDIA_UNKNOWN;
				}
				break;
			case RS_AUDIO_AAC:
				m_relayInfo.audioType = AVMEDIA_AAC;
				break;
			default:
				m_relayInfo.audioType = AVMEDIA_UNKNOWN;
				break;
		}
	}
	else if( Data->Type==FRAME_DATA )
	{
		RS_FRAME_HEADER_T * pHeader = (RS_FRAME_HEADER_T*)Data->pData;
		const char* pData = (const char*)Data->pData + sizeof(RS_FRAME_HEADER_T);

		if (pHeader->type == RS_FRAME_VIDEO)
		{
			int frameType = 0;
			if (pHeader->keyframe)
			{
				switch (m_relayInfo.videoType)
				{
				case AVMEDIA_H264:
				{
					BYTE btType = pData[4] & 0x1F;
					switch (btType)
					{
					case 7:
						frameType = CODED_SPS;
						//TRACE( _T("SPS \n") );
						break;
					case 8:
						//TRACE( _T("PPS \n") );
						frameType = CODED_PPS;
						break;
						//case 5:
						//TRACE( _T("IDR \n") );
						//frameType = CODED_IDR;
						//	break;
					default:
						frameType = CODED_IDR;
						break;
					}
				}
				break;
				case AVMEDIA_MPEG4:
					frameType = CODING_TYPE_INTRA;
					break;
				case AVMEDIA_JPEG:
					frameType = 0;
					break;
				default:
					frameType = -1;
					break;
				}
			}
			else
				frameType = -100;

			if (pHeader->type == RS_FRAME_VIDEO)
			{
				/*
				if( !g_flag_end ) return;
				UINT year, month, day, hour, minute, second;
				GetTime( pHeader->date, pHeader->time, year, month, day, hour, minute, second );
				DATA_HEADER header;
				header.type = HEADER_TYPE_VIDEO;
				header.year = year;
				header.month = month;
				header.day = day;
				header.hour = hour;
				header.minute = minute;
				header.second = second;

				if( m_PlaybackStream->m_extra_video && ((pData[4] & 0x1F) == 5))
				{
				header.size = pHeader->frame_size + m_PlaybackStream->m_extra_size;
				m_PlaybackStream->m_buffer->Write2(header,m_PlaybackStream->m_extra_video,m_PlaybackStream->m_extra_size,(BYTE *)pData,pHeader->frame_size);
				}
				else
				{
				header.size = pHeader->frame_size;
				m_PlaybackStream->m_buffer->Write(header,(BYTE*)pData);
				}
				*/
			}
		}

		else if (pHeader->type == RS_FRAME_AUDIO)
		{
			/*
			DATA_HEADER header;
			header.type = HEADER_TYPE_AUDIO;
			header.size = pHeader->frame_size;
			m_PlaybackStream->m_buffer->Write(header,(BYTE*)pData);
			*/
		}
	}
	else
	{
	}
}

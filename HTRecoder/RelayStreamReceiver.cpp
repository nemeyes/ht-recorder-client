#include "stdafx.h"
#include "RelayStreamReceiver.h"
#include "ScopedLock.h"

#ifdef VMS_RECORDER

RelayStreamReceiver::RelayStreamReceiver( Recorder * service )
	: _bConntected(FALSE)
	, _bRun(FALSE)
{
	InitializeCriticalSection( &_lock );
	_service = service;
}

RelayStreamReceiver::~RelayStreamReceiver( VOID )
{
	DeleteCriticalSection( &_lock );
}

VOID RelayStreamReceiver::Start( VOID )
{
	_bRun = TRUE;
}

VOID RelayStreamReceiver::Stop( VOID )
{
	_bRun = FALSE;
}

VOID RelayStreamReceiver::SetStreamInfo( CString UUID )
{
	_relayUUID = UUID;
}

VOID RelayStreamReceiver::GetTime( UINT date, UINT time, UINT& year, UINT& month, UINT& day, UINT& hour, UINT& minute, UINT& second )
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

VOID RelayStreamReceiver::OnNotifyMessage( LiveNotifyMsg* pNotify )
{
	if(pNotify->nMessage == LIVE_STREAM_CONNECT) 
	{
		if(pNotify->nError == enLIVE_NO_ERROR)
		{
			TRACE( _T("릴레이 연결 시작 \n") );
			_bConntected = TRUE;
		}
	} 
	else if(pNotify->nMessage == LIVE_STREAM_DISCONNECT) 
	{
		TRACE( _T("릴레이 연결 종료 \n") );
		_bConntected = FALSE;
	}
}

VOID RelayStreamReceiver::OnReceive( LPStreamData Data )
{
	if( !_bRun ) return;

	if( Data->Type==FRAME_XML )
	{
		RS_RELAY_INFO_T rlInfo;
		rlInfo.szID = _relayUUID;
		_service->GetRelayInfo( (VOID*)Data->pData, &rlInfo );
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
			_relayInfo.videoType = AVMEDIA_JPEG;
			break;
		case RS_VIDEO_MPEG4 :
			_relayInfo.videoType = AVMEDIA_MPEG4;
			break;
		case RS_VIDEO_H264 :
			_relayInfo.videoType = AVMEDIA_H264;
			break;
		default:
			_relayInfo.videoType = AVMEDIA_UNKNOWN;
			break;
		}

		_relayInfo.stream = pVideo->stream;
		_relayInfo.szVideoSize.cx = pVideo->width;
		_relayInfo.szVideoSize.cy = pVideo->height;
		if( pVideo->extra_size>0 )
		{
			RS_STREAM_BUFFER_T *p = new RS_STREAM_BUFFER_T( Data->pData, Data->nDataSize );
			p->dwChannel	= Data->nChannel;
			p->nLength		= Data->nDataSize;
			p->pBuffer		= (BYTE*) p->Buffer.GetPtr();
			p->nWidth		= _relayInfo.szVideoSize.cx;
			p->nHeight		= _relayInfo.szVideoSize.cy;
			p->br			= 0;
			p->sr			= 0;
			p->mediaType	=_relayInfo.videoType;
			p->tReceived	= Data->tReceived;
			p->nFrameType	= CODED_SPS;

			CChannelInfo *chInfo = gChannelManager->GetChannelInfo( _relayUUID );

			if( chInfo==NULL ) return;
			if( chInfo && chInfo->m_pCamera && chInfo->m_pCamera->m_RelayStream) 
			{
				CHitronStreamer * pStreamer = chInfo->m_pCamera->m_RelayStream;
				switch( p->mediaType )
				{
				case AVMEDIA_MPEG4 :
					pStreamer->m_videoCodec = VIDEO_MPEG4;
					break;
				case AVMEDIA_H264 :
					pStreamer->m_videoCodec = VIDEO_H264;
					break;
				case AVMEDIA_JPEG :
					pStreamer->m_videoCodec = VIDEO_JPEG;
					break;
				}

				if( pStreamer->m_extra_video == NULL )
				{
					chInfo->m_pCamera->m_relayCriSection.Lock();
					//chInfo->m_pCamera->m_pVideoData = (BYTE *)av_malloc(p->nWidth*p->nHeight*2);
					pStreamer->m_extra_video = (BYTE *)av_malloc(MAX_RESOLUTION*2);
					memcpy(pStreamer->m_extra_video,(BYTE*)(p->pBuffer+sizeof(RS_VIDEO_CODEC_T)),pVideo->extra_size);
					pStreamer->m_extra_size = pVideo->extra_size;
					chInfo->m_pCamera->m_relayCriSection.Unlock();
				}
				else
				{
					chInfo->m_pCamera->m_relayCriSection.Lock();
					if(pStreamer->m_extra_video) av_free(pStreamer->m_extra_video);	pStreamer->m_extra_video = NULL;
					pStreamer->m_extra_video = (BYTE *)av_malloc(MAX_RESOLUTION*2);
					memcpy(pStreamer->m_extra_video,(BYTE*)(p->pBuffer+sizeof(RS_VIDEO_CODEC_T)),pVideo->extra_size);
					pStreamer->m_extra_size = pVideo->extra_size;
					chInfo->m_pCamera->m_relayCriSection.Unlock();
				}
			}
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
			_relayInfo.audioType = AVMEDIA_PCM;
			break;
		case RS_AUDIO_G711U:
			_relayInfo.audioType = AVMEDIA_PCMU;
			break;
		case RS_AUDIO_G711A:
			_relayInfo.audioType = AVMEDIA_PCMA;
			break;
		case RS_AUDIO_G726:
			{
				if( pAudio->bit_rate==16000 )
					_relayInfo.audioType = AVMEDIA_G726_16;
				else if( pAudio->bit_rate==24000 )
					_relayInfo.audioType = AVMEDIA_G726_24;
				else if( pAudio->bit_rate==32000 )
					_relayInfo.audioType = AVMEDIA_G726_32;
				else if( pAudio->bit_rate==40000 )
					_relayInfo.audioType = AVMEDIA_G726_40;
				else
					_relayInfo.audioType = AVMEDIA_UNKNOWN;
			}
			break;
		case RS_AUDIO_AAC:
			_relayInfo.audioType = AVMEDIA_AAC;
			break;
		default:
			_relayInfo.audioType = AVMEDIA_UNKNOWN;
			break;
		}
	}
	else if( Data->Type==FRAME_DATA )
	{
		RS_FRAME_HEADER_T *pHeader = (RS_FRAME_HEADER_T*)Data->pData;
		const char* pData = (const char*)Data->pData + sizeof(RS_FRAME_HEADER_T);

		AVMEDIA_TYPE mediaType = AVMEDIA_UNKNOWN;
		INT frameType;

		switch( pHeader->type )
		{
		case RS_FRAME_UNKNOWN :
			return;
		case RS_FRAME_VIDEO :
			mediaType = _relayInfo.videoType;
			break;
		case RS_FRAME_AUDIO :
			mediaType = _relayInfo.audioType;
			if( _relayInfo.audioType==AVMEDIA_AAC )
			{
				//p->pExtraData = (BYTE*) m_AudioExtraData.GetPtr();
				//p->nExtraData = m_AudioExtraData.GetSize();
			}
			break;
		default :
			mediaType = (AVMEDIA_TYPE)(AVMEDIA_NRS_META + pHeader->type );
			break;
		}

		if(pHeader->keyframe)
		{
			switch( mediaType )
			{
				case AVMEDIA_H264:
					{
						BYTE btType = pData[4] & 0x1F;
						switch(btType)
						{
						case 7:
							frameType = CODED_SPS;
							break;
						case 8:
							frameType = CODED_PPS;
							break;
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
		else frameType = -100;

		CChannelInfo *chInfo = gChannelManager->GetChannelInfo( _relayUUID );

		if( chInfo && chInfo->m_pCamera && chInfo->m_pCamera->m_RelayStream) 
		{
			CHitronStreamer * pStreamer = chInfo->m_pCamera->m_RelayStream;
			switch( mediaType )
			{
			case AVMEDIA_MPEG4 :
				pStreamer->m_videoCodec = VIDEO_MPEG4;
				break;
			case AVMEDIA_H264 :
				pStreamer->m_videoCodec = VIDEO_H264;
				break;
			case AVMEDIA_JPEG :
				pStreamer->m_videoCodec = VIDEO_JPEG;
				break;
			case AVMEDIA_PCM :
				break;
			case AVMEDIA_PCMU :
				pStreamer->m_audioCodec = AUDIO_G711U;
				break;
			case AVMEDIA_PCMA :
				pStreamer->m_audioCodec = AUDIO_G711A;
				break;
			case AVMEDIA_G726_16 :
			case AVMEDIA_G726_24 :
			case AVMEDIA_G726_32 :
			case AVMEDIA_G726_40 :
				pStreamer->m_audioCodec = AUDIO_G726;
				break;
			case AVMEDIA_AAC :
				pStreamer->m_audioCodec = AUDIO_AAC;
				break;
			}

			if( pHeader->type==RS_FRAME_VIDEO )
			{
				if( g_flag_end ) return;

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
				header.size = pHeader->frame_size;
				pStreamer->m_buffer->Write(header,(BYTE*)pData);
			}
			else if( pHeader->type==RS_FRAME_AUDIO )
			{


			}
		}
	}
	else
	{
	}
}

#endif
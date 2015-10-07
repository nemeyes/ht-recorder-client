#include "stdafx.h"
#include "HTRelayStreamReceiver.h"

HTRelayStreamReceiver::HTRelayStreamReceiver(HTRecorder * service, CString strCameraUuid, CDisplayLib * pVideoView, unsigned char * key, size_t nChannel)
	: m_bConntected(FALSE)
	, m_bRun(FALSE)
	, m_HTRecorder(service)
	, m_relayUUID(strCameraUuid)
	, m_pVideoView(pVideoView)
	, m_nVideoViewChannels(nChannel)
	/*, m_pVideoExtraData(0)
	, m_nVideoExtraDataSize(0)
	, m_pAudioExtraData(0)
	, m_nAudioExtraDataSize(0)*/
	, m_bFindFirstKey(FALSE)
{
	m_Decode.SetVideoOutput(DISP_YV12);
	//m_Decode.SetEnableDecodeMode(TRUE);
	m_Decode.SetDecodeMode(TRUE);

	m_key[0] = key[0];
	m_key[1] = key[1];
	m_key[2] = key[2];
	m_key[3] = key[3];
	m_key[4] = key[4];
	m_key[5] = key[5];
	m_key[6] = key[6];
	m_key[7] = key[7];


	_snprintf(m_strKey, sizeof(m_strKey), "%02X%02X%02X%02X%02X%02X%02X%02X", m_key[0], m_key[1], m_key[2], m_key[3], m_key[4], m_key[5], m_key[6], m_key[7]);
	m_Decode.AddCallbackFunction(m_strKey, (HDISPLIB)m_pVideoView, m_nVideoViewChannels);
	//m_pVideoView->SetBackgroundColor(RGB(0x00, 0x00, 0x00));
	//m_pVideoView->SetChannelBackground(1, D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00));
	m_pVideoView->SetPreview(nChannel, L"Test");
	m_pVideoView->SetEnableAspectratio(m_nVideoViewChannels, TRUE);
	//m_pVideoView->SetEnableBackgroundImage(TRUE);
	//m_pVideoView->SetBackgroundImage(_T("images\\bg.png"), D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00));
}

HTRelayStreamReceiver::~HTRelayStreamReceiver( VOID )
{
	m_Decode.Restore();
	m_Decode.RemoveCallbackFunctionAll();
	m_Decode.Close();

	/*if (m_pVideoExtraData)
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
	m_nAudioExtraDataSize = 0;*/
}

VOID HTRelayStreamReceiver::Start( VOID )
{
	m_bRun = TRUE;
}

VOID HTRelayStreamReceiver::Stop( VOID )
{
	m_bRun = FALSE;
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
		else
		{
			m_Decode.Restore();
			m_Decode.RemoveCallbackFunctionAll();
		}
	} 
	else if(pNotify->nMessage == LIVE_STREAM_DISCONNECT) 
	{
		m_bConntected = FALSE;
		m_Decode.Restore();
		m_Decode.RemoveCallbackFunctionAll();
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
		RS_VIDEO_CODEC_T * pVideo = (RS_VIDEO_CODEC_T*)Data->pData;
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
			RS_STREAM_BUFFER_T *p = new RS_STREAM_BUFFER_T((const char*)Data->pData + sizeof(RS_VIDEO_CODEC_T), pVideo->extra_size);
			memcpy(p->mac, m_key, sizeof(p->mac));
			//p->dwChannel = Data->nChannel;
			p->nLength = Data->nDataSize;
			p->pBuffer = (BYTE*) p->Buffer.GetPtr();
			p->nWidth = m_relayInfo.szVideoSize.cx;
			p->nHeight = m_relayInfo.szVideoSize.cy;
			p->br = 0;
			p->sr = 0;
			p->mediaType = m_relayInfo.videoType;
			p->tReceived = Data->tReceived;
			p->nFrameType = CODED_SPS;
			p->bNoSkip = TRUE;
			p->streamType = INFO_STREAM_TYPE;

			m_nVideoType = p->mediaType;

			/*if (m_pVideoExtraData)
			{
				free(m_pVideoExtraData);
				m_pVideoExtraData = 0;

			}
			m_pVideoExtraData = (BYTE*)malloc(pVideo->extra_size);
			memcpy(m_pVideoExtraData, (BYTE*)(p->pBuffer + sizeof(RS_VIDEO_CODEC_T)), pVideo->extra_size);
			m_nVideoExtraDataSize = pVideo->extra_size;*/

			m_Decode.Process(p);

			delete p;
		}
	}
	else if( Data->Type==FRAME_DATA )
	{
		RS_FRAME_HEADER_T * pHeader = (RS_FRAME_HEADER_T*)Data->pData;
		const char* pData = (const char*)Data->pData + sizeof(RS_FRAME_HEADER_T);

		if (pHeader->type == RS_FRAME_VIDEO)
		{
			const char* pData = (const char*)Data->pData + sizeof(RS_FRAME_HEADER_T);
			RS_STREAM_BUFFER_T * p = new RS_STREAM_BUFFER_T(pData, pHeader->frame_size);

			p->streamType = VIDEO_STREAM_TYPE;

			memcpy(p->mac, m_key, sizeof(p->mac));
			p->nLength = pHeader->frame_size;
			p->pBuffer = (BYTE*)p->Buffer.GetPtr();
			p->nWidth = m_relayInfo.szVideoSize.cx;
			p->nHeight = m_relayInfo.szVideoSize.cy;
			p->br = 0;
			p->sr = 0;
			p->mediaType = m_relayInfo.videoType;
			p->tReceived = Data->tReceived;
			if (pHeader->keyframe)
			{
				if (m_relayInfo.videoType == AVMEDIA_H264)
					p->nFrameType = CODED_IDR;
				else if (m_relayInfo.videoType == AVMEDIA_MPEG4)
					p->nFrameType = CODING_TYPE_INTRA;
				else if (m_relayInfo.videoType == AVMEDIA_JPEG)
					p->nFrameType = 0;
				else
					p->nFrameType = -1;

				if (!m_bFindFirstKey)
					m_bFindFirstKey = TRUE;
			}

			//m_pVideoView->UpdateTime()
			if (m_bFindFirstKey)
				m_Decode.Process(p);
			delete p;
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

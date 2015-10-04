#include "stdafx.h"
#include "HTPlayBackStreamReceiver.h"
#include "ScopedLock.h"

HTPlayBackStreamReceiver::HTPlayBackStreamReceiver(HTRecorder * service)
	: m_bConntected(FALSE)
	, m_bRun(FALSE)
	, m_pVideoExtraData(0)
	, m_nVideoExtraDataSize(0)
	, m_pAudioExtraData(0)
	, m_nAudioExtraDataSize(0)
{
	m_HTReocrder = service;
}

HTPlayBackStreamReceiver::~HTPlayBackStreamReceiver(VOID)
{
	RemoveAllStreamInfo();
}

VOID HTPlayBackStreamReceiver::Start(VOID)
{
	m_bRun = TRUE;
}

VOID HTPlayBackStreamReceiver::Stop(VOID)
{
	m_bRun = FALSE;
}

VOID HTPlayBackStreamReceiver::AddStreamInfo(INT32 channel, CString uuid)
{
	CScopedLock lock(&m_lock);
	m_pbUUIDs.insert(std::make_pair(channel,uuid));
}

VOID HTPlayBackStreamReceiver::RemoveStreamInfo(INT32 channel)
{
	CScopedLock lock( &m_lock );
	std::map<INT32,CString>::iterator iter;
	iter = m_pbUUIDs.find( channel );
	if( iter!=m_pbUUIDs.end() ) 
		m_pbUUIDs.erase( iter );
}

VOID HTPlayBackStreamReceiver::RemoveStreamInfo(CString uuid)
{
	CScopedLock lock( &m_lock );
	std::map<INT32,CString>::iterator iter;
	for( iter=m_pbUUIDs.begin(); iter!=m_pbUUIDs.end(); iter++ )
	{
		if( ((*iter).second)==uuid ) 
			break;
	}
	if( iter!=m_pbUUIDs.end() ) 
		m_pbUUIDs.erase( iter );
}

VOID HTPlayBackStreamReceiver::RemoveAllStreamInfo(VOID)
{
	CScopedLock lock(&m_lock);
	m_pbUUIDs.clear();
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

VOID HTPlayBackStreamReceiver::GetTime(UINT date, UINT time, UINT& year, UINT& month, UINT& day, UINT& hour, UINT& minute, UINT& second)
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

VOID HTPlayBackStreamReceiver::InitPbInfo(VOID)
{
	for(int i=0; i<RS_MAX_PLAYBACK_CH; i++)
	{
		m_pbInfo[i].avType = AVMEDIA_UNKNOWN;
		m_pbInfo[i].stream = -1;
		m_pbInfo[i].szVideoSize.cx = 0;
		m_pbInfo[i].szVideoSize.cy = 0;
	}
}

VOID HTPlayBackStreamReceiver::InsertPbInfo(int nIdx, AVMEDIA_TYPE avt_Type, UINT32 ui32Width, UINT32 ui32Height)
{
	m_pbInfo[nIdx].avType = avt_Type;
	m_pbInfo[nIdx].stream = nIdx;
	m_pbInfo[nIdx].szVideoSize.cx = ui32Width;
	m_pbInfo[nIdx].szVideoSize.cy = ui32Height;
}

VOID HTPlayBackStreamReceiver::OnNotifyMessage( LiveNotifyMsg* pNotify )
{
	if(pNotify->nMessage == LIVE_STREAM_CONNECT) 
	{
		if(pNotify->nError == enLIVE_NO_ERROR)
		{
			TRACE( _T("플레이백 연결 성공 \n") );
			m_bConntected = TRUE;
		}
	} 
	else if(pNotify->nMessage == LIVE_STREAM_DISCONNECT) 
	{
		TRACE( _T("플레이백 연결 종료 \n") );
		m_bConntected = FALSE;
	}
}

VOID HTPlayBackStreamReceiver::OnReceive( LPStreamData Data )
{
	if( !m_bRun ) return;

	if( Data->Type==FRAME_XML )
	{
		RS_PLAYBACK_INFO_T		pbInfo = {0,};
		if (m_HTReocrder->GetPlaybackInfo((VOID*)Data->pData, &pbInfo))
		{
			m_nPlaybackID = pbInfo.playbackID;
			if( pbInfo.Enable==FALSE )
			{
				TRACE(_T("플레이백 실패 \n"), m_nPlaybackID);
				//need to add exception for playback when no data response;
			}
			else
			{
				TRACE(_T("플레이백 성공 \n"), m_nPlaybackID);
			}
			
		}
	}
	else if( Data->Type==FRAME_VIDEO_CODEC )
	{
		RS_VIDEO_CODEC_T *pVideo = (RS_VIDEO_CODEC_T*)Data->pData;
		AVMEDIA_TYPE avType;
		switch( pVideo->type )
		{
		case RS_VIDEO_MJPEG :
			avType = AVMEDIA_JPEG;
			break;
		case RS_VIDEO_MPEG4 :
			avType = AVMEDIA_MPEG4;
			break;
		case RS_VIDEO_H264 :
			avType = AVMEDIA_H264;
			break;
		default:
			avType = AVMEDIA_UNKNOWN;
			break;
		}
		//TRACE( _T("pVideo->stream : %d \n"), pVideo->stream );
		//TRACE( _T("pVideo->channel : %d \n"), pVideo->channel );
		InsertPbInfo( pVideo->stream, avType, pVideo->width, pVideo->height );

		if( pVideo->extra_size>0 )
		{
			RS_STREAM_BUFFER_T *p = new RS_STREAM_BUFFER_T( Data->pData, Data->nDataSize );
			p->dwChannel	= Data->nChannel;
			p->nLength		= Data->nDataSize;
			p->pBuffer		= (BYTE*) p->Buffer.GetPtr();
			p->nWidth		= m_pbInfo[pVideo->stream].szVideoSize.cx;
			p->nHeight		= m_pbInfo[pVideo->stream].szVideoSize.cy;
			p->br			= 0;
			p->sr			= 0;
			p->mediaType	= m_pbInfo[pVideo->stream].avType;
			p->tReceived	= Data->tReceived;
			p->nFrameType	= CODED_SPS;


			m_nVideoType	= p->mediaType;

			if (m_pVideoExtraData)
			{
				free(m_pVideoExtraData);
				m_pVideoExtraData = 0;
			}
			m_pVideoExtraData = (BYTE*)malloc(pVideo->extra_size);
			memcpy(m_pVideoExtraData, (BYTE*)(p->pBuffer + sizeof(RS_VIDEO_CODEC_T)), pVideo->extra_size);
			m_nVideoExtraDataSize = pVideo->extra_size;
			//TRACE( "IDR:%02x %02x %02x %02x %02x %02x \n", chInfo->m_pCamera->m_pVideoData[0], chInfo->m_pCamera->m_pVideoData[1], chInfo->m_pCamera->m_pVideoData[2], chInfo->m_pCamera->m_pVideoData[3], chInfo->m_pCamera->m_pVideoData[4], chInfo->m_pCamera->m_pVideoData[5] );

			delete p;
		}
	}
	else if( Data->Type==FRAME_AUDIO_CODEC )
	{
#if 1//matia_adding_20130115
		RS_AUDIO_CODEC_T *pAudio = (RS_AUDIO_CODEC_T*)Data->pData;

		if( pAudio->extra_size > 0 )
		{
			RS_STREAM_BUFFER_T * p = new RS_STREAM_BUFFER_T( Data->pData, Data->nDataSize );
			p->pBuffer = reinterpret_cast<BYTE*>( p->Buffer.GetPtr() );
			if (m_pAudioExtraData)
			{
				free(m_pAudioExtraData);
				m_pAudioExtraData = 0;
			}
			
			m_pAudioExtraData = (BYTE *)malloc(pAudio->extra_size);
			memcpy(m_pAudioExtraData, (BYTE*)(p->pBuffer + sizeof(RS_AUDIO_CODEC_T)), pAudio->extra_size);
			m_nAudioExtraDataSize = pAudio->extra_size;
		}
		//if(pStreamer->m_h_output == NULL)
		//{
		//	if((pStreamer->m_audioCodec == AUDIO_G711A) || (pStreamer->m_audioCodec == AUDIO_G711U))
		//	{
		//		pStreamer->m_wave_format.wFormatTag = WAVE_FORMAT_PCM;
		//		pStreamer->m_wave_format.nChannels = 1;//1-mono,2-stereo
		//		pStreamer->m_wave_format.nSamplesPerSec = 8000; //sampling period 16khz
		//		pStreamer->m_wave_format.wBitsPerSample = 16;//16; // G.711 sampling 16bits
		//		pStreamer->m_wave_format.nBlockAlign = 2;
		//		pStreamer->m_wave_format.nAvgBytesPerSec = 16000;
		//		pStreamer->m_wave_format.cbSize = sizeof(WAVEFORMATEX);
		//		//m_wave_format.cbSize = 0;
		//	}
		//	else if(pStreamer->m_audioCodec == AUDIO_AAC)
		//	{
		//		char audioObjectType = ((BYTE*)pStreamer->m_extra_audio)[0]>>3;
		//		char samplingFreqIndex = ( ((BYTE*)pStreamer->m_extra_audio)[0] & 0x7 )<<1 | ( ((BYTE*)pStreamer->m_extra_audio)[1]>>7 );
		//		char channelConfig = ( ((BYTE*)pStreamer->m_extra_audio)[1] & 0x7F ) >> 3;

		//		pStreamer->m_wave_format.wFormatTag = WAVE_FORMAT_PCM;
		//		pStreamer->m_wave_format.nChannels = channelConfig;
		//		pStreamer->m_wave_format.nSamplesPerSec = SamplingFrequencyTable[samplingFreqIndex]; //sampling period 16khz
		//		pStreamer->m_wave_format.wBitsPerSample = 16;//16; // G.711 sampling 16bits
		//		pStreamer->m_wave_format.nBlockAlign = (pStreamer->m_wave_format.nChannels*pStreamer->m_wave_format.wBitsPerSample/8);
		//		pStreamer->m_wave_format.nAvgBytesPerSec = pStreamer->m_wave_format.nSamplesPerSec*pStreamer->m_wave_format.nBlockAlign;
		//		pStreamer->m_wave_format.cbSize = sizeof(WAVEFORMATEX);
		//		//m_wave_format.cbSize = 0;
		//	}
		//	MMRESULT mmResult = waveOutOpen(&pStreamer->m_h_output,
		//		WAVE_MAPPER, 
		//		&pStreamer->m_wave_format,
		//		NULL/*(DWORD_PTR)waveOutProc*/,
		//		/*(DWORD)this->m_hWnd*/NULL,
		//		CALLBACK_NULL/*CALLBACK_FUNCTION*/);

		//	TRACE(" result = %d \n",mmResult);

		//	if(mmResult == MMSYSERR_NOERROR )
		//	{
		//		//f_send_message(_T("sucess"));
		//	}
		//	else if(mmResult == MMSYSERR_BADDEVICEID )
		//	{
		//		//f_send_message(_T("out of bound"));
		//	}
		//	pStreamer->m_flag_audio = TRUE;
		//}
#endif

	}
	else if( Data->Type==FRAME_DATA )
	{
		RS_FRAME_HEADER_T * pHeader = (RS_FRAME_HEADER_T*)Data->pData;
		const char* pData = (const char*)Data->pData + sizeof(RS_FRAME_HEADER_T);

		if(pHeader->type == RS_FRAME_VIDEO)
		{
			int frameType = 0;
			if(pHeader->keyframe)
			{
				switch(m_pbInfo[pHeader->stream].avType)
				{
				case AVMEDIA_H264:
					{
						BYTE btType = pData[4] & 0x1F;
						switch(btType)
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
				case AVMEDIA_PCM:
					break;
				case AVMEDIA_PCMU:
					break;
				case AVMEDIA_PCMA:
					break;
				case AVMEDIA_G726_16:
					break;
				case AVMEDIA_G726_24:
					break;
				case AVMEDIA_G726_32:
					break;
				case AVMEDIA_G726_40:
					break;
				case AVMEDIA_AAC:
					break;
				default:
					frameType = -1;
					break;
				}
			}
			else 
				frameType = -100;
		
			if( pHeader->type == RS_FRAME_VIDEO )
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

		else if(pHeader->type == RS_FRAME_AUDIO)
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

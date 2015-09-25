#include "stdafx.h"
#include "PlayBackStreamReceiver.h"
#include "ScopedLock.h"

#ifdef WITH_HITRON_RECORDER

PlayBackStreamReceiver::PlayBackStreamReceiver( Recorder * service )
	: _bConntected(FALSE)
	, _bRun(FALSE)
{
	InitializeCriticalSection( &_lock );
	_service = service;

	memset(&m_playbackTime,0,sizeof(RECORD_TIME));
}

PlayBackStreamReceiver::~PlayBackStreamReceiver( VOID )
{
	RemoveAllStreamInfo();
	DeleteCriticalSection( &_lock );
}

VOID PlayBackStreamReceiver::Start( VOID )
{
	_bRun = TRUE;
}

VOID PlayBackStreamReceiver::Stop( VOID )
{
	_bRun = FALSE;
}

VOID PlayBackStreamReceiver::AddStreamInfo( INT32 channel, CString uuid )
{
	CScopedLock lock( &_lock );
	_pbUUIDs.insert( std::make_pair(channel,uuid) );
}

VOID PlayBackStreamReceiver::RemoveStreamInfo( INT32 channel )
{
	CScopedLock lock( &_lock );
	std::map<INT32,CString>::iterator iter;
	iter = _pbUUIDs.find( channel );
	if( iter!=_pbUUIDs.end() ) _pbUUIDs.erase( iter );
}

VOID PlayBackStreamReceiver::RemoveStreamInfo( CString uuid )
{
	CScopedLock lock( &_lock );
	std::map<INT32,CString>::iterator iter;
	for( iter=_pbUUIDs.begin(); iter!=_pbUUIDs.end(); iter++ )
	{
		if( ((*iter).second)==uuid ) break;
	}
	if( iter!=_pbUUIDs.end() ) _pbUUIDs.erase( iter );
}

VOID PlayBackStreamReceiver::RemoveAllStreamInfo( VOID )
{
	CScopedLock lock( &_lock );
	_pbUUIDs.clear();
}

VOID PlayBackStreamReceiver::GetTime( UINT date, UINT time, UINT& year, UINT& month, UINT& day, UINT& hour, UINT& minute, UINT& second )
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

VOID PlayBackStreamReceiver::InitPbInfo( VOID )
{
	for(int i=0; i<RS_MAX_PLAYBACK_CH; i++)
	{
		_pbInfo[i].avType = AVMEDIA_UNKNOWN;
		_pbInfo[i].stream = -1;
		_pbInfo[i].szVideoSize.cx = 0;
		_pbInfo[i].szVideoSize.cy = 0;
	}
}

VOID PlayBackStreamReceiver::InsertPbInfo( int nIdx, AVMEDIA_TYPE avt_Type, UINT32 ui32Width, UINT32 ui32Height )
{
	_pbInfo[nIdx].avType = avt_Type;
	_pbInfo[nIdx].stream = nIdx;
	_pbInfo[nIdx].szVideoSize.cx = ui32Width;
	_pbInfo[nIdx].szVideoSize.cy = ui32Height;
}

VOID PlayBackStreamReceiver::OnNotifyMessage( LiveNotifyMsg* pNotify )
{
	if(pNotify->nMessage == LIVE_STREAM_CONNECT) 
	{
		if(pNotify->nError == enLIVE_NO_ERROR)
		{
			TRACE( _T("플레이백 연결 성공 \n") );
			_bConntected = TRUE;
		}
	} 
	else if(pNotify->nMessage == LIVE_STREAM_DISCONNECT) 
	{
		TRACE( _T("플레이백 연결 종료 \n") );
		_bConntected = FALSE;
	}
}

VOID PlayBackStreamReceiver::OnReceive( LPStreamData Data )
{
	if( !_bRun ) return;

	if( Data->Type==FRAME_XML )
	{
		RS_PLAYBACK_INFO_T		pbInfo = {0,};
		if( _service->GetPlaybackInfo((VOID*)Data->pData, &pbInfo) )
		{
			_playbackID = pbInfo.playbackID;
			if( pbInfo.Enable==FALSE )
			{
				TRACE( _T("플레이백 실패 \n"), _playbackID );
				//need to add exception for playback when no data response;
			}
			else
			{
				TRACE( _T("플레이백 성공 \n"), _playbackID );
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
			p->nWidth		= _pbInfo[pVideo->stream].szVideoSize.cx;
			p->nHeight		= _pbInfo[pVideo->stream].szVideoSize.cy;
			p->br			= 0;
			p->sr			= 0;
			p->mediaType	= _pbInfo[pVideo->stream].avType;
			p->tReceived	= Data->tReceived;
			p->nFrameType	= CODED_SPS;

			CString UUID;
			{
				CScopedLock lock( &_lock );
				std::map<INT32,CString>::iterator iter;
				iter = _pbUUIDs.find( pVideo->stream );
				if( iter==_pbUUIDs.end() ) return;
				UUID = (*iter).second;
			}

			CChannelInfo *chInfo = gChannelManager->GetChannelInfo( UUID );

			if( chInfo==NULL ) return;
			if( chInfo && chInfo->m_pCamera && chInfo->m_pCamera->m_PlaybackStream) 
			{
				CHitronStreamer * pStreamer = chInfo->m_pCamera->m_PlaybackStream;
				switch( p->mediaType )
				{
				case AVMEDIA_MPEG4 :
					//TRACE( _T("AVMEDIA_MPEG4 \n") );
					pStreamer->m_videoCodec = VIDEO_MPEG4;
					break;
				case AVMEDIA_H264 :
					//TRACE( _T("AVMEDIA_H264 \n") );
					pStreamer->m_videoCodec = VIDEO_H264;
					break;
				case AVMEDIA_JPEG :
					//TRACE( _T("AVMEDIA_JPEG \n") );
					pStreamer->m_videoCodec = VIDEO_JPEG;
					break;
				}

				if( pStreamer->m_extra_video==NULL )
				{
					pStreamer->m_extra_video = (BYTE *)av_malloc(MAX_RESOLUTION*2);
					memcpy(pStreamer->m_extra_video,(BYTE*)(p->pBuffer+sizeof(RS_VIDEO_CODEC_T)),pVideo->extra_size);
					pStreamer->m_extra_size = pVideo->extra_size;
					//TRACE( "IDR:%02x %02x %02x %02x %02x %02x \n", chInfo->m_pCamera->m_pVideoData[0], chInfo->m_pCamera->m_pVideoData[1], chInfo->m_pCamera->m_pVideoData[2], chInfo->m_pCamera->m_pVideoData[3], chInfo->m_pCamera->m_pVideoData[4], chInfo->m_pCamera->m_pVideoData[5] );
				}
			}
			delete p;
		}
	}
	else if( Data->Type==FRAME_AUDIO_CODEC )
	{
#if 1//matia_adding_20130115
		RS_AUDIO_CODEC_T *pAudio = (RS_AUDIO_CODEC_T*)Data->pData;

		CString UUID;
		{
			CScopedLock lock( &_lock );
			std::map<INT32,CString>::iterator iter;
			iter = _pbUUIDs.find( pAudio->stream );
			if( iter==_pbUUIDs.end() ) return;
			UUID = (*iter).second;
		}

		CChannelInfo *chInfo = gChannelManager->GetChannelInfo( UUID );
		CHitronStreamer * pStreamer = NULL;
		if( chInfo==NULL ) return;
		if( chInfo && chInfo->m_pCamera && chInfo->m_pCamera->m_PlaybackStream) 
		{
			pStreamer = chInfo->m_pCamera->m_PlaybackStream;
			if( pAudio->type==RS_AUDIO_AAC )		pStreamer->m_audioCodec = AUDIO_AAC;
			else if( pAudio->type==RS_AUDIO_G711A ) pStreamer->m_audioCodec = AUDIO_G711A;
			else if( pAudio->type==RS_AUDIO_G711U )	pStreamer->m_audioCodec = AUDIO_G711U;
			else									pStreamer->m_audioCodec = AUDIO_NULL;
		}
		else return;

		if( pAudio->extra_size > 0 )
		{
			RS_STREAM_BUFFER_T *p = new RS_STREAM_BUFFER_T( Data->pData, Data->nDataSize );
			p->pBuffer = reinterpret_cast<BYTE*>( p->Buffer.GetPtr() );

			if(pStreamer->m_extra_audio) av_free(pStreamer->m_extra_audio);

			pStreamer->m_extra_audio = (BYTE *)av_malloc(pAudio->extra_size*sizeof(uint8_t));

			memcpy( pStreamer->m_extra_audio, (BYTE*)(p->pBuffer+sizeof(RS_AUDIO_CODEC_T)), pAudio->extra_size );
			pStreamer->m_extra_audio_size = pAudio->extra_size;
		}

		if(pStreamer->m_h_output == NULL)
		{
			//chInfo->m_pCamera->m_audioPlaybackDecoder->m_samplingFrequencyIndex = frequency;
			//chInfo->m_pCamera->m_audioPlaybackDecoder->m_ChannelConfiguration = channels;
			//m_AudioBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE*3;
			//if(m_pAudioData == NULL) m_pAudioData = (BYTE *)av_malloc(m_AudioBufSize);
			if((pStreamer->m_audioCodec == AUDIO_G711A) || (pStreamer->m_audioCodec == AUDIO_G711U))
			{
				pStreamer->m_wave_format.wFormatTag = WAVE_FORMAT_PCM;
				pStreamer->m_wave_format.nChannels = 1;//1-mono,2-stereo
				pStreamer->m_wave_format.nSamplesPerSec = 8000; //sampling period 16khz
				pStreamer->m_wave_format.wBitsPerSample = 16;//16; // G.711 sampling 16bits
				pStreamer->m_wave_format.nBlockAlign = 2;
				pStreamer->m_wave_format.nAvgBytesPerSec = 16000;
				pStreamer->m_wave_format.cbSize = sizeof(WAVEFORMATEX);
				//m_wave_format.cbSize = 0;
			}
			else if(pStreamer->m_audioCodec == AUDIO_AAC)
			{
				char audioObjectType = ((BYTE*)pStreamer->m_extra_audio)[0]>>3;
				char samplingFreqIndex = ( ((BYTE*)pStreamer->m_extra_audio)[0] & 0x7 )<<1 | ( ((BYTE*)pStreamer->m_extra_audio)[1]>>7 );
				char channelConfig = ( ((BYTE*)pStreamer->m_extra_audio)[1] & 0x7F ) >> 3;

				pStreamer->m_wave_format.wFormatTag = WAVE_FORMAT_PCM;
				pStreamer->m_wave_format.nChannels = channelConfig;
				pStreamer->m_wave_format.nSamplesPerSec = SamplingFrequencyTable[samplingFreqIndex]; //sampling period 16khz
				pStreamer->m_wave_format.wBitsPerSample = 16;//16; // G.711 sampling 16bits
				pStreamer->m_wave_format.nBlockAlign = (pStreamer->m_wave_format.nChannels*pStreamer->m_wave_format.wBitsPerSample/8);
				pStreamer->m_wave_format.nAvgBytesPerSec = pStreamer->m_wave_format.nSamplesPerSec*pStreamer->m_wave_format.nBlockAlign;
				pStreamer->m_wave_format.cbSize = sizeof(WAVEFORMATEX);
				//m_wave_format.cbSize = 0;
			}
			MMRESULT mmResult = waveOutOpen(&pStreamer->m_h_output,
				WAVE_MAPPER, 
				&pStreamer->m_wave_format,
				NULL/*(DWORD_PTR)waveOutProc*/,
				/*(DWORD)this->m_hWnd*/NULL,
				CALLBACK_NULL/*CALLBACK_FUNCTION*/);

			TRACE(" result = %d \n",mmResult);

			if(mmResult == MMSYSERR_NOERROR )
			{
				//f_send_message(_T("sucess"));
			}
			else if(mmResult == MMSYSERR_BADDEVICEID )
			{
				//f_send_message(_T("out of bound"));
			}

			pStreamer->m_flag_audio = TRUE;
		}
	
#endif

	}
	else if( Data->Type==FRAME_DATA )
	{
		RS_FRAME_HEADER_T *pHeader = (RS_FRAME_HEADER_T*)Data->pData;
		const char* pData = (const char*)Data->pData + sizeof(RS_FRAME_HEADER_T);

		if(pHeader->type == RS_FRAME_VIDEO)
		{
			int frameType = 0;
			if(pHeader->keyframe)
			{
				switch(_pbInfo[pHeader->stream].avType)
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
			else frameType = -100;
			

			CString UUID;
			{
				CScopedLock lock( &_lock );
				std::map<INT32,CString>::iterator iter;
				iter = _pbUUIDs.find( pHeader->stream );
				if( iter==_pbUUIDs.end() ) 
				{

					TRACE( _T("존재하지않음\n") );
					return;
				}
				UUID = (*iter).second;
			}
			CChannelInfo *chInfo = gChannelManager->GetChannelInfo( UUID );

			if( chInfo && chInfo->m_pCamera && chInfo->m_pCamera->m_PlaybackStream) 
			{
				CHitronStreamer * pStreamer = chInfo->m_pCamera->m_PlaybackStream;
				switch( _pbInfo[pHeader->stream].avType )
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

				if( pHeader->type == RS_FRAME_VIDEO )
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

					//TRACE( "%02x %02x %02x %02x %02x %02x \n",pData[0],pData[1], pData[2], pData[3], pData[4], pData[5] );

					CTime tTime( year, month, day, hour, minute, second );

					if(chInfo->m_pCamera->m_flag_single_playback && !chInfo->m_pCamera->m_flag_event_popup)
					{
						if(chInfo->m_pCamera->m_pCameraDlg)
						{
							if(chInfo->m_pCamera->m_pCameraDlg->m_pCameraToolDlg->m_stopTime > tTime)
							{
								chInfo->m_pCamera->m_pCameraDlg->m_pCameraToolDlg->DrawTrack(tTime);
							}
							else
							{
								VMS_MSG msg;
								msg.uuid = chInfo->GetUUID();
								msg.msg = PLAYBACK_SINGLE_STOP;
								gQueueManager->AddMessage(msg);
								TRACE("PLAYBACK_SINGLE_STOP \n");
							}
						}
					}
					else if(chInfo->m_pCamera->m_flag_event_popup)
					{
						if(chInfo->m_pCamera->m_pEventPopUpDlg)
						{
							if(chInfo->m_pCamera->m_pEventPopUpDlg->m_pCameraToolDlg->m_stopTime > tTime)
							{
								chInfo->m_pCamera->m_pEventPopUpDlg->m_pCameraToolDlg->DrawTrack(tTime);
							}
							else
							{
								VMS_MSG msg;
								msg.uuid = chInfo->GetUUID();
								msg.msg = PLAYBACK_SINGLE_STOP;
								gQueueManager->AddMessage(msg);
								TRACE("PLAYBACK_SINGLE_STOP \n");
							}
						}
					}
					else
					{
						g_mainDlg->SetTimeLineBar(tTime);
					}
				}
			}
		}
		else if(pHeader->type == RS_FRAME_AUDIO)
		{
			CString UUID;
			{
				CScopedLock lock( &_lock );
				std::map<INT32,CString>::iterator iter;
				iter = _pbUUIDs.find( pHeader->stream );
				if( iter==_pbUUIDs.end() ) 
				{
					return;    
				}
				UUID = (*iter).second;
			}
			CChannelInfo *chInfo = gChannelManager->GetChannelInfo( UUID );

			if( chInfo && chInfo->m_pCamera && chInfo->m_pCamera->m_PlaybackStream) 
			{
				DATA_HEADER header;
				header.type = HEADER_TYPE_AUDIO;
				header.size = pHeader->frame_size;
				chInfo->m_pCamera->m_PlaybackStream->m_buffer->Write(header,(BYTE*)pData);
			}
		}
	}
	else
	{
	}
}

#endif
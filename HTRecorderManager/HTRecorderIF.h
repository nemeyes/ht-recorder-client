#pragma once

#include <HTRecorderDLL.h>
#include <LiveSession5.h>
#include <LiveSessionDLL.h>

#include "DisplayLib.h"

enum _exportRecordFile
{
	RECORDER_EXPORT_OK = 0,
	RECORDER_CONNECTION_FAIL,
	RECORDER_NOT_EXIST_EXPORT_FILE,
};

enum _vms_exportRecordFile
{
	VMS_RESULT_DEFAULT = 0,
	VMS_RESULT_SUCCESS,
	VMS_RESULT_FAIL_FILE,
	VMS_RESULT_FAIL_CONNECT,
	VMS_RESULT_NO_RECORDER,
};

class HTNotificationReceiver;

typedef struct _HTRECORD_TIME_T
{
	UINT year;
	UINT month;
	UINT day;
	UINT hour;
	UINT minute;
	UINT second;
	_HTRECORD_TIME_T(VOID)
	{
		year = 0;
		month = 0;
		day = 0;
		hour = 0;
		minute = 0;
		second = 0;
	}
	_HTRECORD_TIME_T(const _HTRECORD_TIME_T & clone)
	{
		year = clone.year;
		month = clone.month;
		day = clone.day;
		hour = clone.hour;
		minute = clone.minute;
		second = clone.second;
	}
	_HTRECORD_TIME_T & operator = (const _HTRECORD_TIME_T & clone)
	{
		year = clone.year;
		month = clone.month;
		day = clone.day;
		hour = clone.hour;
		minute = clone.minute;
		second = clone.second;
		return (*this);
	}
} HTRECORD_TIME_T;

typedef struct _HTRECORD_TIME_INFO_T
{
	time_t startTime;
	time_t endTime;
} HTRECORD_TIME_INFO_T;


typedef std::vector<CString> VCamUUIDList;
typedef std::map<CString,IStreamReceiver5*> PBStreamReceiverList; //CString -> group UUID or vcam UUID, IStreamReceiver5 -> playback streamer
typedef std::multimap<CString,IStreamReceiver5*> RLStreamReceiverList; //CString -> group UUID or vcam UUID, IStreamReceiver5 -> relay streamer


class HTRecorderIF
{
public:
	HTRecorderIF(BOOL bRunAsRecorder = FALSE);
	~HTRecorderIF(VOID);

	HTRecorder * GetRecorder(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, UINT nRetryCount=0);
	BOOL IsRecording(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid);

	VOID KillRelayStream(VOID);
	VOID KillPlayBackStream(VOID);

	BOOL StartRelay(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid, CDisplayLib * pVideoView, unsigned char * key, size_t nChannel);
	BOOL StopRelay(CString strRecorderUuid, CString strRecorderAddress, CString strRecorderUsername, CString strRecorderPassword, CString strCameraUuid);

private:
	HTRecorder * GetRecorder(RS_SERVER_INFO_T * serverInfo, UINT nRetryCount=0);
	BOOL MakeDeviceInfo(CString strCameraUuid, RS_DEVICE_INFO_T * rsDeviceInfo);


	/*
	BOOL	StartPlayback( RecStreamRequstInfo *vcamInfo );
	BOOL	StopPlayback( RecStreamRequstInfo *vcamInfo );

	BOOL	ControlPause( RecStreamRequstInfo *vcamInfo );
	BOOL	ControlResume( RecStreamRequstInfo *vcamInfo );

	//BOOL	ControlForwardPlay( RecStreamRequstInfo *vcamInfo, UINT speed );
	//BOOL	ControlBackwardPlay( RecStreamRequstInfo *vcamInfo, UINT speed );
	BOOL	ControlPlaySpeed( RecStreamRequstInfo *vcamInfo, VideoSpeedInfo *jumpInfo );

	BOOL	GetRecordingRetentionTime( RS_SERVER_INFO_T *serverInfo, BOOL *enable, UINT *year, UINT *month, UINT *week, UINT *day );

	time_t		m_endTime;
	CPtrArray	*m_pRecTime;
	BOOL	GetSelectedTimeline(RecStreamRequstInfo *vcamInfo, time_t startTime, time_t endTime);
	void	AddRecTime(tm *startTime,tm *endTime);
	BOOL	GetTimelineTrack( RecStreamRequstInfo *vcamInfo, int type, SYSTEMTIME startTime);
	BOOL	LoadTimeline(CString path);
	void	DeleteRecTimeArray();
	void	DeleteRecTimeArray(CTime curtime,int retentionTime);
	void	UpdateLastTime();
	BOOL	SaveTimeline(CString path);
	BOOL	SendMessage(RecStreamRequstInfo *vcamInfo, UINT msg);

	BOOL	GetRecordingStatus(RecStreamRequstInfo *vcamInfo);

	void	SendTimelineData(CString vcamUUID, SYSTEMTIME startTime);

	//단일 카메라의 저장영상의 playback 시작위치를 정해진 시간의 위치로 이동한다.
	BOOL	JumpPlaybackByInterval(RecStreamRequstInfo *vcamInfo, VideoJumpInfo *info);

	//단일 카메라의 저장영상을 AVI 파일로 추출하여 저장한다.
	int		StartExport( RecStreamRequstInfo *chInfo, ExportInfo *exportInfo, LPEXPORTMSGHANDLER pHandler = NULL, LPVOID pParam = NULL);
	BOOL	StopExport( RecStreamRequstInfo *chInfo );
	void	ResetExportList();
	*/

private:

	std::map<CString,HTRecorder*>	m_mapRecorderList;
	std::map<CString,CString>		m_mapCameraList;
	HTNotificationReceiver	* m_notifier;

	CCriticalSection	m_lockRecorder;
	CCriticalSection	m_lockRelayReceiver;
	CCriticalSection	m_lockPlaybackReceiver;
	CCriticalSection	m_lockTimeline;
	CCriticalSection	m_lockExportReceiver;
	//ExportStreamReceiver								*_exportReceiver;
	//std::map<RecStreamRequstInfo*, ExportStreamReceiver *> m_exportList;
	
	CCriticalSection	m_lockCamera;
	std::map<CString,PBStreamReceiverList>	m_playbackUUID;
	std::map<CString,RLStreamReceiverList>	m_relayUUID;

	BOOL	m_bRunAsRecorder;
};
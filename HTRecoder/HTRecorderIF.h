#pragma once

#include <RecorderDLL.h>
#include <LiveSession5.h>
#include <LiveSessionDLL.h>

enum _exportRecordFile
{
	RECORDER_EXPORT_OK = 0,
	RECORDER_CONNECTION_FAIL,
	RECORDER_NOT_EXIST_EXPORT_FILE,
};


class CGroupInfo;
class CChannelInfo;
class CChannelManager;
class NotificationReceiver;
struct VCAM_STREAM_URL;
typedef struct _RECORD_TIME
{
	UINT year;
	UINT month;
	UINT day;
	UINT hour;
	UINT minute;
	UINT second;
	_RECORD_TIME( VOID )
	{
		year = 0;
		month = 0;
		day = 0;
		hour = 0;
		minute = 0;
		second = 0;
	}
	_RECORD_TIME( const _RECORD_TIME& clone )
	{
		year = clone.year;
		month = clone.month;
		day = clone.day;
		hour = clone.hour;
		minute = clone.minute;
		second = clone.second;
	}
	_RECORD_TIME& operator = ( const _RECORD_TIME& clone )
	{
		year = clone.year;
		month = clone.month;
		day = clone.day;
		hour = clone.hour;
		minute = clone.minute;
		second = clone.second;
		return (*this);
	}
} RECORD_TIME;

typedef struct _RECORD_TIME_INFO
{
	RECORD_TIME startTime;
	RECORD_TIME endTime;
	_RECORD_TIME_INFO( VOID ) {}
	_RECORD_TIME_INFO( const _RECORD_TIME_INFO& clone )
	{
		startTime = clone.startTime;
		endTime = clone.endTime;
	}
	_RECORD_TIME_INFO& operator = ( const _RECORD_TIME_INFO& clone )
	{
		startTime = clone.startTime;
		endTime = clone.endTime;
		return (*this);
	}
} RECORD_TIME_INFO;

typedef std::vector<CString> VCamUUIDList;
typedef std::map<CString,IStreamReceiver5*> PBStreamReceiverList; //CString -> group UUID or vcam UUID, IStreamReceiver5 -> playback streamer
typedef std::multimap<CString,IStreamReceiver5*> RLStreamReceiverList; //CString -> group UUID or vcam UUID, IStreamReceiver5 -> relay streamer


class RecorderIF
{
public:
	RecorderIF( BOOL bRunAsRecorder=FALSE );
	~RecorderIF( VOID );

	//BOOL	IsConnected( RS_SERVER_INFO_T *serverInfo );
	//BOOL	Reconnect( RS_SERVER_INFO_T *serverInfo );

	VOID	KillRelayStream( VOID );
	VOID	KillPlayBackStream( VOID );

/////////////////////////////////// VMS_RECORDER 관리기능 ///////////////////////////////////////////////
	///////////////////////  DEVICE  /////////////////////////
	//특정 레코더로 부터 등록된 카메라 목록를 조회한다.
	BOOL	GetDeviceList( RS_SERVER_INFO_T *serverInfo, RS_DEVICE_INFO_SET_T *deviceInfoList );
	//서버가 가지고 있는 카메라 목록과 현재 Client가 가지고 있는 group의 카메라 갯수를 비교하여 틀리면 FALSE를 리턴한다.
	BOOL	CheckDeviceList( RS_SERVER_INFO_T *serverInfo, CGroupInfo *group );

	//그룹내에 속해있는 카메라를 연관된 Recorder에 일괄 등록한다.
	BOOL	AddDevice( CGroupInfo *group );
	//채널목록내에 속해있는 카메라를 연관된 Recorder에 일괄 등록한다.
	BOOL	AddDevice( std::vector<CChannelInfo*> *chennelInfos );
	//단일 카메라를 연관된 Recorder에 등록한다.
	BOOL	AddDevice( CChannelInfo *chInfo );

	//그룹내에 속해있는 카메라를 연관된 Recorder로부터 일괄 삭제한다.
	BOOL	RemoveDevice( CGroupInfo *group );
	//채널목록내에 속해있는 카메라를 연관된 Recorder로부터 일괄 삭제한다.
	BOOL	RemoveDevice( std::vector<CChannelInfo*> *chennelInfos );
	//단일 카메라를 연관된 Recorder로부터 삭제한다.
	BOOL	RemoveDevice( CChannelInfo *chInfo );

	//그룹내에 속해있는 카메라를 연관된 Recorder로부터 일괄 삭제한다.
	BOOL	RemoveDeviceEx( CGroupInfo *group );
	//채널목록내에 속해있는 카메라를 연관된 Recorder로부터 일괄 삭제한다.
	BOOL	RemoveDeviceEx( std::vector<CChannelInfo*> *chennelInfos );
	//단일 카메라를 연관된 Recorder로부터 삭제한다.
	BOOL	RemoveDeviceEx( CChannelInfo *chInfo );



	//그룹내에 속해있는 카메라의 상태정보를 조회한다.
	BOOL	CheckDeviceStatus( CGroupInfo *group, RS_DEVICE_STATUS_SET_T *deviceStatusList );
	//채널목록내에 속해있는 카메라의 상태정보를 조회한다.
	BOOL	CheckDeviceStatus( std::vector<CChannelInfo*> *chennelInfos, RS_DEVICE_STATUS_SET_T *deviceStatusList );
	//단일 카메라의 상태정보를 조회한다.
	BOOL	CheckDeviceStatus( CChannelInfo *chInfo, RS_DEVICE_STATUS_SET_T *deviceStatusList );

	///////////////// RECORDING PRE-SETUP
	//카메라 그룹에 대한 저장 스케쥴 설정을 가져온다.
	BOOL	GetRecordingScheduleList( CGroupInfo *group, RS_RECORD_SCHEDULE_SET_T *recordShcedList );
	//채널목록에 대한 저장 스케쥴 설정을 가져온다.
	BOOL	GetRecordingScheduleList( std::vector<CChannelInfo*> *chInfos, RS_RECORD_SCHEDULE_SET_T *recordShcedList );
	//단일 카메라에 대한 저장 스케쥴 설정을 가져온다.
	BOOL	GetRecordingScheduleList( CChannelInfo *chInfo, RS_RECORD_SCHEDULE_SET_T *recordShcedList );

	//그룹내에 속해있는 카메라에 대한 저장 스케쥴 설정을 수행한다.
	/*
	schedType : 0-NONE 
	            1-항상 레코딩
		        2-이벤트 레코딩
				3-ADAPTIVE 레코딩 (이벤트 발생 시에는 pre, post 시간을 포함하여 모든 프레임을 저장하고 그 외에는 키프레임만 저장)
	audio : 오디오 저장여부
	*/
	BOOL	UpdateRecordingSchedule( CGroupInfo *group, UINT schedType, BOOL audio, 
									 UINT bitSun, UINT bitMon, UINT bitTue, UINT bitWed, UINT bitThu, UINT bitFri, UINT bitSat, 		
									 UINT preRecordingTime, UINT postRecordingTime );
	//채널 목록내에 속해있는 카메라에 대한 저장 스케쥴 설정을 수행한다.
	BOOL	UpdateRecordingSchedule( std::vector<CChannelInfo*> *chInfos, UINT schedType, BOOL audio, 
									 UINT bitSun, UINT bitMon, UINT bitTue, UINT bitWed, UINT bitThu, UINT bitFri, UINT bitSat, 		
									 UINT preRecordingTime, UINT postRecordingTime );
	//단일 카메라에 대한 저장 스케쥴 설정을 수행한다.
	BOOL	UpdateRecordingSchedule( CChannelInfo *chInfo, UINT schedType, BOOL audio, 
									 UINT bitSun, UINT bitMon, UINT bitTue, UINT bitWed, UINT bitThu, UINT bitFri, UINT bitSat, 
									 UINT preRecordingTime, UINT postRecordingTime );

	//그룹내 카메라와 연관된 다수의 레코더의 저장 overwrite 설정을 enable/disable 한다.
	BOOL	SetRecordingOverwrite( CGroupInfo *group, BOOL onoff );
	//채널목록내 카메라와 연관된 다수의 레코더의 저장 overwrite 설정을 enable/disable 한다.
	BOOL	SetRecordingOverwrite( std::vector<CChannelInfo*> *chInfos, BOOL onoff );
	//특정 레코더의 저장 overwrite 설정을 enable/disable 한다.
	BOOL	SetRecordingOverwrite( RS_SERVER_INFO_T *serverInfo, BOOL onoff );

	//특정 레코더의 저장 overwrite 설정을 가져온다.
	BOOL	GetRecordingOverwrite( RS_SERVER_INFO_T *serverInfo, BOOL *onoff );

	//그룹내 카메라와 연관된 다수의 레코더의 retentiontime을 enable/disable하고, enable시에 해당 값대로 설정한다.
	BOOL	SetRecordingRetentionTime( CGroupInfo *group, BOOL enable, UINT year, UINT month, UINT week, UINT day );
	//채널목록내 카메라와 연관된 다수의 레코더의 retentiontime을 enable/disable하고, enable시에 해당 값대로 설정한다.
	BOOL	SetRecordingRetentionTime( std::vector<CChannelInfo*> *chInfos, BOOL enable, UINT year, UINT month, UINT week, UINT day );
	//특정 레코더의 retentiontime을 enable/disable하고, enable시에 해당 값대로 설정한다.
	BOOL	SetRecordingRetentionTime( RS_SERVER_INFO_T *serverInfo, BOOL enable, UINT year, UINT month, UINT week, UINT day );


	//특정 레코더의 retentiontime 값을 가져온다.
	BOOL	GetRecordingRetentionTime( RS_SERVER_INFO_T *serverInfo, BOOL *enable, UINT *year, UINT *month, UINT *week, UINT *day );


	BOOL	GetDiskInfo( RS_SERVER_INFO_T *serverInfo, RS_DISK_INFO_SET_T *diskInfoList );
	
	//그룹내 카메라와 연관된 다수의 레코더에 속해있는 특정 디스크(strVolumeSerial)에 recordingSize만큼의 저장공간을 할당한다.
	//BOOL	ReserveDiskSpace( CGroupInfo *group, CString strVolumeSerial, UINT64 recordingSize );
	//채널목록내 카메라와 연관된 다수의 레코더에 속해있는 특정 디스크(strVolumeSerial)에 recordingSize만큼의 저장공간을 할당한다.
	//BOOL	ReserveDiskSpace( std::vector<CChannelInfo*> *chInfos, CString strVolumeSerial, UINT64 recordingSize );
	//특정 레코더에 속해있는 특정 디스크(strVolumeSerial)에 recordingSize만큼의 저장공간을 할당한다.
	BOOL	ReserveDiskSpace( RS_SERVER_INFO_T *serverInfo, CString strVolumeSerial, UINT64 recordingSize );

	//특정 레코더에 할당된 카메라의 저장 위치(디스크위치)를 질의 한다. 
	//두번째 파라미터[std::map<CString,VCamUUIDList> *vcamUUIDList]의 
	//CString은 특정 디스크의 volumeSerial이며, VCamUUIDList를 카메라에 대한 UUID vector이다.
	BOOL	GetDiskPolicy( RS_SERVER_INFO_T *serverInfo, std::map<CString,VCamUUIDList> *vcamUUIDList );

	//카메라에 대한 디스크 할당은 하나의 레코더만을 대상으로 한다. - CGroupInfo 또는 std::vector<CChannelInfo*>의 카메라는 모두 동일한 레코더임
	//그룹에 속한 카메라를 특정 디스크에 할당 - 디스크 할당에 실패하거나 strVolumeSerial과 연관된 디스크를 찾지 못할 경우 FALSE 리턴
	BOOL	UpdateDiskPolicy( CGroupInfo *group, CString strVolumeSerial );

	//채널에 속한 카메라를 특정 디스크에 할당 - 디스크 할당에 실패하거나 strVolumeSerial과 연관된 디스크를 찾지 못할 경우 FALSE 리턴
	BOOL	UpdateDiskPolicy( std::vector<CChannelInfo*> *chInfos, CString strVolumeSerial );

	//단일 카메라를 특정 디스크에 할당 - 디스크 할당에 실패하거나 strVolumeSerial과 연관된 디스크를 찾지 못할 경우 FALSE 리턴
	BOOL	UpdateDiskPolicy( CChannelInfo *chInfo, CString strVolumeSerial );

	//등록된 모든 카메라를 특정 디스크에 할당 - 디스크 할당에 실패하거나 strVolumeSerial과 연관된 디스크를 찾지 못할 경우 FALSE 리턴
	BOOL	UpdateDiskPolicy( CChannelManager *chMgr, CString strVolumeSerial );

/////////////////////////////////// VMS_RECORDER 및 VMS_VIEWER 공통기능 ///////////////////////////////////////////////
	///////////////// RELAY
	//BOOL	GetRelayInfo( VOID *xmlData );
	BOOL	StartRelay( CGroupInfo *group );
	BOOL	StartRelay( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	StartRelay( CChannelInfo *chInfo );
	
	BOOL	UpdateRelay( CGroupInfo *group );
	BOOL	UpdateRelay( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	UpdateRelay( CChannelInfo *chInfo );

	BOOL	StopRelay( CGroupInfo *group );
	BOOL	StopRelay( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	StopRelay( CChannelInfo *chInfo );

	///////////////// RECORDING
	//그룹에 속한 카메라의 레코딩이 스케쥴에 따라 작동할 준비가 되었는지 확인
	BOOL	IsRecording( CGroupInfo *group, RS_RECORDING_STATUS_SET_T *recordingStatusList );
	//채널 목록에 속한 카메라의 레코딩이 스케쥴에 따라 작동할 준비가 되었는지 확인
	BOOL	IsRecording( std::vector<CChannelInfo*> *chInfos, RS_RECORDING_STATUS_SET_T *recordingStatusList );
	//단일 카메라에 대한 레코딩이 스케쥴에 따라 작동할 준비가 되었는지 확인
	BOOL	IsRecording( CChannelInfo *chInfo, RS_RECORDING_STATUS_SET_T *recordingStatusList );

	//그룹에 속한 카메라의 레코딩 시작
	BOOL	StartRecordingRequest( CGroupInfo *group );
	//채널 목록에 속한 카메라의 레코딩 시작
	BOOL	StartRecordingRequest( std::vector<CChannelInfo*> *chInfos );
	//단일 카메라에 대한 레코딩 시작(저장명령이 실패하거나 레코딩중이면 FALSE 리턴)
	BOOL	StartRecordingRequest( CChannelInfo *chInfo );

	//그룹에 속한 카메라의 레코딩 종료
	BOOL	StopRecordingRequest( CGroupInfo *group );
	//채널 목록에 속한 카메라의 레코딩 종료
	BOOL	StopRecordingRequest( std::vector<CChannelInfo*> *chInfos );
	//단일 카메라에 대한 레코딩 종료(정지명령이 실패하거나 레코딩 중이 아니면 FALSE 리턴)
	BOOL	StopRecordingRequest( CChannelInfo *chInfo );

	//그룹에 속한 카메라연관된 레코드 대한 일괄 레코딩 시작(레코드에 등록된 모든 카메라가 레코딩을 시작함)
	BOOL	StartRecordingAll( CGroupInfo *group );
	//채널목록에 속한 카메라연관된 레코드 대한 일괄 레코딩 시작(레코드에 등록된 모든 카메라가 레코딩을 시작함)
	BOOL	StartRecordingAll( std::vector<CChannelInfo*> *chInfos );
	//특정 레코드에 등록된 카메라에 대한 일괄 레코딩 시작
	BOOL	StartRecordingAll( RS_SERVER_INFO_T *serverInfo );

	//그룹에 속한 카메라연관된 레코드 대한 일괄 레코딩 종료(레코드에 등록된 모든 카메라가 레코딩을 종료함)
	BOOL	StopRecordingAll( CGroupInfo *group );
	//그룹에 속한 카메라연관된 레코드 대한 일괄 레코딩 종료(레코드에 등록된 모든 카메라가 레코딩을 종료함)
	BOOL	StopRecordingAll( std::vector<CChannelInfo*> *chInfos );
	//특정 레코드에 등록된 카메라에 대한 일괄 레코딩 종료
	BOOL	StopRecordingAll( RS_SERVER_INFO_T *serverInfo );

	//여러 레코드에서 레코딩되고 있는 카메라(그룹)의 레코딩 정보 삭제
	BOOL	DeleteRecordingData( CGroupInfo *group );
	//여러 레코드에서 레코딩되고 있는 카메라(채널)의 레코딩 정보 삭제
	BOOL	DeleteRecordingData( std::vector<CChannelInfo*> *chInfos );
	//여러 레코드에서 레코딩되고 있는 단일 카메라의 레코딩 정보 삭제
	BOOL	DeleteRecordingData( CChannelInfo *chInfo );

	///////////////// PLAYBACK STREAM
	//CALENDAR SEARCH
	//그룹내에 속해있는 카메라의 년/월/일/시/분의 시간정보를 가져온다.
	BOOL	GetTimeIndex( CGroupInfo *group, UINT year, std::map<CChannelInfo*,CPtrArray> *time );
	//채널 목록내에 속해있는 카메라의 년/월/일/시/분의 시간정보를 가져온다.
	BOOL	GetTimeIndex( std::vector<CChannelInfo*> *chInfos, UINT year, std::map<CChannelInfo*,CPtrArray> *time );

	//그룹내에 속해있는 카메라와 연관된 저장(Recording) 시간정보를 가져온다.
	BOOL	GetDayIndex( CGroupInfo *group, UINT year, UINT month, UINT day );
	//채널목록내에 속해있는 카메라와 연관된 저장(Recording) 시간정보를 가져온다.
	BOOL	GetDayIndex( std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day );
	//단일 카메라와 연관된 저장(Recording) 시간정보를 가져온다.
	BOOL	GetDayIndex( CChannelInfo *chInfo, UINT year, UINT month, UINT day );
	//단일 카메라와 연관된 저장(Recording) 시간정보를 가져온다.-현재날짜로부터 원하는 기간(일단위)을 가지고 온다.
	BOOL	GetPeriodIndex( CChannelInfo *chInfo, int period );
	//단일 카메라와 연관된 저장(Recording) 시간정보를 가져온다.-현재시간으로부터 원하는 기간(분단위)을 가지고 온다.
	BOOL RecorderIF::GetRecentMinIndex( CChannelInfo *chInfo, CTime curTime, UINT min );
	//단일 카메라와 연관된 저장(Recording) 시간정보를 가져온다.-현재시간으로부터 원하는 기간(분단위)을 가지고 온다.
	int RecorderIF::GetRecordedTimeExport( CChannelInfo *chInfo, CTime startTime, CTime endTime);

/*
	//그룹내에 속해있는 카메라의 월정보를 가져온다.
	BOOL	GetYearIndex( CGroupInfo *group, UINT year, std::map<CChannelInfo*,PlayBackTimeInfo> *month );
	//채널목록내에 속해있는 카메라의 월정보를 가져온다.
	BOOL	GetYearIndex( std::vector<CChannelInfo*> *chInfos, UINT year, std::map<CChannelInfo*,PlayBackTimeInfo> *month );

	//그룹내에 속해있는 카메라의 일정보를 가져온다.
	BOOL	GetMonthIndex( CGroupInfo *group, UINT year, UINT month, std::map<CChannelInfo*,PlayBackMonthInfo> *day );
	//채널목록내에 속해있는 카메라의 일정보를 가져온다.
	BOOL	GetMonthIndex( std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, std::map<CChannelInfo*,PlayBackMonthInfo> *day );

	//그룹내에 속해있는 카메라의 시간정보를 가져온다.
	BOOL	GetDayIndex( CGroupInfo *group, UINT year, UINT month, UINT day, std::map<CChannelInfo*,PlayBackDayInfo> *hour );
	//채널목록내에 속해있는 카메라의 시간정보를 가져온다.
	BOOL	GetDayIndex( std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day, std::map<CChannelInfo*,PlayBackDayInfo> *hour );

	//그룹내에 속해있는 카메라의 분정보를 가져온다.
	BOOL	GetHourIndex( CGroupInfo *group, UINT year, UINT month, UINT day, UINT hour, std::map<CChannelInfo*,PlayBackHourInfo> *minute );
	//채널목록내에 속해있는 카메라의 분정보를 가져온다.
	BOOL	GetHourIndex( std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day, UINT hour, std::map<CChannelInfo*,PlayBackHourInfo> *minute );
*/


	//PLAYBACK
	//그룹내에 속해있는 카메라의 PlayBack을 요청한다.
	BOOL	StartPlayback( CGroupInfo *group, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second=0 );
	//채널목록내에 속해있는 카메라의 PlayBack을 요청한다.
	BOOL	StartPlayback( CString groupUUID, std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second=0 );
	//단일카메라에 대한 PlayBack을 요청한다.
	BOOL	StartPlayback( CChannelInfo *chInfo, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second=0 );

	//그룹내에 속해있는 카메라의 PlayBack을 중지한다.
	BOOL	StopPlayback( CGroupInfo *group );
	//채널목록내에 속해있는 카메라의 PlayBack을 중지한다.
	BOOL	StopPlayback( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	//단일 카메라에 대한 PlayBack을 중지한다.
	BOOL	StopPlayback( CChannelInfo *chInfo );

	//PLAYBACK CONTROL
	//그룹내에 속해있는 카메라의 Play을 재시작한다.(pause상태 또는 forward/backward replay 상태에서)
	BOOL	ControlPlay( CGroupInfo *group );
	//채널목록내에 속해있는 카메라의 Play을 재시작한다.(pause상태 또는 forward/backward replay 상태에서)
	BOOL	ControlPlay( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	//단일 카메라의 Play을 재시작한다.(pause상태 또는 forward/backward replay 상태에서)
	BOOL	ControlPlay( CChannelInfo *chInfo );

	//그룹내에 속해있는 카메라의 Play를 중지한다.
	BOOL	ControlStop( CGroupInfo *group );
	//채널목록내에 속해있는 카메라의 Play를 중지한다.
	BOOL	ControlStop( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	//단일 카메라의 Play를 중지한다.
	BOOL	ControlStop( CChannelInfo *chInfo );

	//그룹내에 속해있는 카메라의 Play를 임시중지한다.
	BOOL	ControlPause( CGroupInfo *group );
	//채널목록내에 속해있는 카메라의 Play를 임시중지한다.
	BOOL	ControlPause( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	//단일 카메라의 Play를 임시중지한다.
	BOOL	ControlPause( CChannelInfo *chInfo );

	//그룹내에 속해있는 카메라의 임시중지를 해제한다.
	BOOL	ControlResume( CGroupInfo *group );
	//채널목록내에 속해있는 카메라의 임시중지를 해제한다.
	BOOL	ControlResume( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	//단일 카메라의 Play를 임시중지를 해제한다.
	BOOL	ControlResume( CChannelInfo *chInfo );

	//그룹내에 속해있는 카메라의 영상을 배속으로 forward play한다. 기본적으로 key frame 단위로 수신한다.
	BOOL	ControlFowardPlay( CGroupInfo *group, UINT speed );
	//채널목록내에 속해있는 카메라의 영상을 배속으로 forward play한다. 기본적으로 key frame 단위로 수신한다.
	BOOL	ControlFowardPlay( CString groupUUID, std::vector<CChannelInfo*> *chInfos, UINT speed );
	//단일 카메라의 영상을 배속으로 forward play한다. 기본적으로 key frame 단위로 수신한다.
	BOOL	ControlFowardPlay( CChannelInfo *chInfo, UINT speed );

	//그룹내에 속해있는 카메라의 영상을 배속으로 backward play한다. 기본적으로 key frame 단위로 수신한다.
	BOOL	ControlBackwardPlay( CGroupInfo *group, UINT speed );
	//채널목록내에 속해있는 카메라의 영상을 배속으로 backward play한다. 기본적으로 key frame 단위로 수신한다.
	BOOL	ControlBackwardPlay( CString groupUUID, std::vector<CChannelInfo*> *chInfos, UINT speed );
	//단일 카메라의 영상을 배속으로 backward play한다. 기본적으로 key frame 단위로 수신한다.
	BOOL	ControlBackwardPlay( CChannelInfo *chInfo, UINT speed );


	//그룹내에 속해있는 카메라의 저장영상의 playback 시작위치를 정해진 시간의 위치로 이동한다.
	BOOL	ControlJump( CGroupInfo *group, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second );
	//채널목록내에 속해있는 카메라의 저장영상의 playback 시작위치를 정해진 시간의 위치로 이동한다.
	BOOL	ControlJump( CString groupUUID, std::vector<CChannelInfo*> *chInfos, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second );
	//단일 카메라의 저장영상의 playback 시작위치를 정해진 시간의 위치로 이동한다.
	BOOL	ControlJump( CChannelInfo *chInfo, UINT year, UINT month, UINT day, UINT hour, UINT minute, UINT second );

	//그룹내에 속해있는 카메라의 저장영상의 playback 첫 시작위치의 시간으로 이동한다.
	BOOL	ControlGoToFirst( CGroupInfo *group );//, UINT *year, UINT *month, UINT*day, UINT *hour, UINT *minute, UINT *second );
	//채널목록내에 속해있는 카메라의 저장영상의 playback 첫 시작위치의 시간으로 이동한다.
	BOOL	ControlGoToFirst( CString groupUUID, std::vector<CChannelInfo*> *chInfos);//, UINT *year, UINT *month, UINT *day, UINT *hour, UINT *minute, UINT *second );
	//단일 카메라의 저장영상의 playback 첫 시작위치의 시간으로 이동한다.
	BOOL	ControlGoToFirst( CChannelInfo *chInfo );

	//그룹내에 속해있는 카메라의 저장영상의 playback 마지막 종료위치의 시간으로 이동한다.
	BOOL	ControlGoToLast( CGroupInfo *group );//, UINT *year, UINT *month, UINT*day, UINT *hour, UINT *minute, UINT *second );
	//채널목록내에 속해있는 카메라의 저장영상의 playback 마지막 종료위치의 시간으로 이동한다.
	BOOL	ControlGoToLast( CString groupUUID, std::vector<CChannelInfo*> *chInfos );//, UINT *year, UINT *month, UINT *day, UINT *hour, UINT *minute, UINT *second );
	//단일 카메라의 저장영상의 playback 마지막 종료위치의 시간으로 이동한다.
	BOOL	ControlGoToLast( CChannelInfo *chInfo );

	BOOL	ControlForwardStep( CGroupInfo *group );
	BOOL	ControlForwardStep( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	ControlForwardStep( CChannelInfo *chInfo );

	BOOL	ControlBackwardStep( CGroupInfo *group );
	BOOL	ControlBackwardStep( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	ControlBackwardStep( CChannelInfo *chInfo );

	//EXPORT
	//BOOL	StartExport( CGroupInfo *group );
	//BOOL	StartExport( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	StartExport( CChannelInfo *chInfo, CString szFilePath, 
		UINT sYear, UINT sMonth, UINT sDay, UINT sHour, UINT sMinute, 
		UINT eYear, UINT eMonth, UINT eDay, UINT eHour, UINT eMinute,
		LPEXPORTMSGHANDLER pHandler = NULL, LPVOID pParam = NULL);

	//BOOL	StopExport( CGroupInfo *group );
	//BOOL	StopExport( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	StopExport( CChannelInfo *chInfo );

	/*
	BOOL	PauseExport( CGroupInfo *group );
	BOOL	PauseExport( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	PauseExport( CChannelInfo *chInfo );

	BOOL	ResumeExport( CGroupInfo *group );
	BOOL	ResumeExport( CString groupUUID, std::vector<CChannelInfo*> *chInfos );
	BOOL	ResumeExport( CChannelInfo *chInfo );
	*/

	//Set Permission
	void	SetRunAsRecorder( BOOL bRunAsRecorder ){ _bRunAsRecorder=bRunAsRecorder; };
	BOOL	GetRunAsRecorder(){ return _bRunAsRecorder; };


	Recorder * GetRecorder( RS_SERVER_INFO_T *serverInfo, UINT nRetryCount=0 );

private:
	Recorder * GetRecorder( CChannelInfo *chInfo, UINT nRetryCount=0 );
//	Recorder * GetRecorder( RS_SERVER_INFO_T *serverInfo, UINT nRetryCount=0 );

	static VOID GenerateQuasiMac( CString *vcamMacAddress );
	static UINT16 GetIndexOfMaxResolution( VCAM_STREAM_URL *vcamStreamUrl );
	static VOID GetDeviceUrls( CString *szVcamStreamUrl, CString *szIpAddress, CString *szProfileName, UINT16 &nRtspPort );
	static BOOL MakeDeviceInfo( CChannelInfo *chInfo, RS_DEVICE_INFO_T *devInfo );
	static BOOL MakeSchedInfo( CChannelInfo *chInfo, RS_RECORD_SCHEDULE_INFO_T *schedInfo, UINT schedType, BOOL audio,
							   UINT bitSun, UINT bitMon, UINT bitTue, UINT bitWed, UINT bitThu, UINT bitFri, UINT bitSat, 		
							   UINT preRecordingTime, UINT postRecordingTime );

	std::map<CString,Recorder*>							_recorderList;
	std::map<CString,CString>							_cameraList;

	CRITICAL_SECTION									_lockOfRecorder;
	CRITICAL_SECTION									_lockOfCamera;

	CRITICAL_SECTION									_lockOfPbReceiver;
	CRITICAL_SECTION									_lockOfRlReceiver;
	std::map<CString,PBStreamReceiverList>				_pbUUID;
	std::map<CString,RLStreamReceiverList>				_rlUUID;




	BOOL												_bRunAsRecorder;
	NotificationReceiver								*_notifier;

	CRITICAL_SECTION									_lockOfExpReceiver;
	ExportStreamReceiver								*_exportReceiver;
};
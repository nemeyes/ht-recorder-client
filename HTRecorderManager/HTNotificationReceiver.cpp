#include "stdafx.h"
#include "HTNotificationReceiver.h"
#include "OutputWnd.h"
#include "StatusLogDAO.h"
#include "HTNotificationReceiverFactory.h"

HTNotificationReceiver::HTNotificationReceiver(void)
	: m_wndEventListWindow(NULL)
{
	unsigned int thrdAddr;
	m_pollThread = (HANDLE)(::_beginthreadex(NULL, 0, &HTNotificationReceiver::PollProcess, this, 0, &thrdAddr));
}

HTNotificationReceiver::~HTNotificationReceiver(void)
{
	m_bPoll = FALSE;
	::WaitForSingleObject(m_pollThread, INFINITE);
	::CloseHandle(m_pollThread);
}

void HTNotificationReceiver::SetHTRecorder(HTRecorder * recorder)
{
	m_pHTRecorder = recorder;
}

void HTNotificationReceiver::SetRecorderAddress(wchar_t * address)
{
	memset(m_strRecorderAddress, 0x00, sizeof(m_strRecorderAddress));
	wcscpy(m_strRecorderAddress, address);
}

wchar_t * HTNotificationReceiver::GetRecorderAddress(void)
{
	return m_strRecorderAddress;
}

VOID HTNotificationReceiver::SetEventListWindow(CWnd * wnd)
{
	m_wndEventListWindow = wnd;
}

CWnd* HTNotificationReceiver::GetEventListWindow(void)
{
	return m_wndEventListWindow;
}

unsigned __stdcall HTNotificationReceiver::PollProcess(VOID * param)
{
	HTNotificationReceiver * self = static_cast<HTNotificationReceiver*>(param);
	self->Poll();
	return 0;
}

void HTNotificationReceiver::Poll(void)
{
	m_bPoll = TRUE;
	while (m_bPoll)
	{
		if (wcslen(m_strRecorderAddress) > 0)
		{
			RS_DEVICE_INFO_SET_T devices;
			RS_DEVICE_STATUS_SET_T status;
			BOOL result = m_pHTRecorder->GetDeviceList(&devices);
			if (result)
			{
				result = m_pHTRecorder->CheckDeviceStatus(&devices, &status);
				if (result)
				{
					//status.validDeviceCount
					for (int index = 0; index < status.validDeviceCount; index++)
					{
						if (status.deviceStatusInfo[index].nIsConnected)
						{
							CString strMsg = TEXT("Device");
							strMsg += TEXT("[");
							strMsg += devices.deviceInfo[index].GetURL();
							strMsg += TEXT("] Associated with ");
							strMsg += GetRecorderAddress();
							strMsg += TEXT(" Recorder Server is Connected.");
							if (m_wndEventListWindow)
								((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);
						}
						else
						{
							CString strMsg = TEXT("Device");
							strMsg += TEXT("[");
							strMsg += devices.deviceInfo[index].GetURL();
							strMsg += TEXT("] Associated with ");
							strMsg += GetRecorderAddress();
							strMsg += TEXT(" Recorder Server is Disconnected.");
							if (m_wndEventListWindow)
								((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);
						}

						if (status.deviceStatusInfo[index].nIsRecordingError)
						{
							CString strMsg = TEXT("Device");
							strMsg += TEXT("[");
							strMsg += devices.deviceInfo[index].GetURL();
							strMsg += TEXT("] Associated with ");
							strMsg += GetRecorderAddress();
							strMsg += TEXT(" Recorder Server is Under Recording Error.");
							if (m_wndEventListWindow)
								((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);
						}
					}
				}
			}
		}
		::Sleep(3000);
	}
}

void HTNotificationReceiver::OnConnectionStop(RS_CONNECTION_STOP_NOTIFICATION_T * notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버에 다른 관리자가 로그인하였습니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_INFO;
	log.category = CATEGORY_SYSTEM;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}

void HTNotificationReceiver::OnRecordingStorageFull(RS_STORAGE_FULL_NOTIFICATION_T *notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버의 저장 공간이 부족합니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_CIRITICAL;
	log.category = CATEGORY_STORAGE;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}

void HTNotificationReceiver::OnReservedStorageFull(RS_STORAGE_FULL_NOTIFICATION_T *notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버의 예약 저장 공간이 부족합니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_CIRITICAL;
	log.category = CATEGORY_STORAGE;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}

void HTNotificationReceiver::OnOverwritingError(RS_OVERWRITE_ERROR_NOTIFICATION_T *notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버가 덮어쓰기에 실패하였습니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_CIRITICAL;
	log.category = CATEGORY_STORAGE;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}

void HTNotificationReceiver::OnConfigurationChanged(RS_CONFIGURATION_CHANGED_NOTIFICATION_T *notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버의 환경 설정이 변경되었습니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_INFO;
	log.category = CATEGORY_SYSTEM;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}

void HTNotificationReceiver::OnPlaybackError(RS_PLAYBACK_ERROR_NOTIFICATION_T *notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버의 재생에 실패하였습니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_WARNING;
	log.category = CATEGORY_PLAYBACK;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}

void HTNotificationReceiver::OnDiskError(RS_DISK_ERROR_NOTIFICATION_T * notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버의 저장매체(디스크)에 오류가 발견되었습니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_WARNING;
	log.category = CATEGORY_STORAGE;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}

void HTNotificationReceiver::OnKeyFrameMode(RS_KEY_FRAME_MODE_NOTIFICATION_T * notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버의 저장매체(디스크) 속도저하로 인해, 저장모드가 키프레임이 저장모드로 변경되었습니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_WARNING;
	log.category = CATEGORY_STORAGE;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}

void HTNotificationReceiver::OnBufferClean(RS_BUFFER_CLEAN_NOTIFICATION_T * notification)
{
	CString strMsg;
	strMsg = GetRecorderAddress();
	strMsg += TEXT(" 레코더 서버의 저장매체(디스크) 응답시간 지연으로 인해, 저장을 위한 버퍼를 비우고 새로운 저장을 시도합니다.");
	if (m_wndEventListWindow)
		((COutputWnd*)(m_wndEventListWindow))->AddString2OutputEvent(strMsg);

	STATUS_LOG_T log;
	wcscpy(log.uuid, m_strRecorderAddress);
	log.level = LEVEL_WARNING;
	log.category = CATEGORY_STORAGE;
	log.resid = 0;
	wcscpy(log.contents, (LPWSTR)(LPCWSTR)strMsg);

	StatusLogDAO dao;
	dao.CreateStatusLog(&log);
}
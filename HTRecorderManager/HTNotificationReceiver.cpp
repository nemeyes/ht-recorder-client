#include "stdafx.h"
#include "HTNotificationReceiver.h"
#include "OutputWnd.h"
#include "StatusLogDAO.h"

HTNotificationReceiver::HTNotificationReceiver(void)
	: m_wndEventListWindow(NULL)
{

}

HTNotificationReceiver::~HTNotificationReceiver(void)
{


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
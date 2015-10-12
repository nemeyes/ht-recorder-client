#pragma once

#include <HTRecorderDLL.h>
#include <LiveSessionDLL.h>
#include <LiveSession5.h>

class HTNotificationReceiver : public INotificationReceiver
{
public:
	HTNotificationReceiver(void);
	~HTNotificationReceiver(void);

	void SetRecorderAddress(wchar_t * address);
	wchar_t * GetRecorderAddress(void);

	void SetEventListWindow(CWnd * wnd);
	CWnd* GetEventListWindow(void);

	void OnConnectionStop(RS_CONNECTION_STOP_NOTIFICATION_T * notification);
	void OnRecordingStorageFull(RS_STORAGE_FULL_NOTIFICATION_T * notification);
	void OnReservedStorageFull(RS_STORAGE_FULL_NOTIFICATION_T * notification);
	void OnOverwritingError(RS_OVERWRITE_ERROR_NOTIFICATION_T * notification);
	void OnConfigurationChanged(RS_CONFIGURATION_CHANGED_NOTIFICATION_T * notification);
	void OnPlaybackError(RS_PLAYBACK_ERROR_NOTIFICATION_T * notification);
	void OnDiskError(RS_DISK_ERROR_NOTIFICATION_T * notification);
	void OnKeyFrameMode(RS_KEY_FRAME_MODE_NOTIFICATION_T * notification);
	void OnBufferClean(RS_BUFFER_CLEAN_NOTIFICATION_T * notification);

private:
	wchar_t m_strRecorderAddress[100];
	CWnd * m_wndEventListWindow;
};
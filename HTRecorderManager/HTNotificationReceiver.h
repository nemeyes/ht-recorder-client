#pragma once

#include <HTRecorderDLL.h>
#include <LiveSessionDLL.h>
#include <LiveSession5.h>

class HTNotificationReceiver : public INotificationReceiver
{
public:
	HTNotificationReceiver(VOID);
	~HTNotificationReceiver(VOID);

	CString GetRecorderIP(CString strRecorderUuid);
	VOID OnConnectionStop(RS_CONNECTION_STOP_NOTIFICATION_T * notification);
	VOID OnRecordingStorageFull(RS_STORAGE_FULL_NOTIFICATION_T * notification);
	VOID OnReservedStorageFull(RS_STORAGE_FULL_NOTIFICATION_T * notification);
	VOID OnOverwritingError(RS_OVERWRITE_ERROR_NOTIFICATION_T * notification);
	VOID OnConfigurationChanged(RS_CONFIGURATION_CHANGED_NOTIFICATION_T * notification);
	VOID OnPlaybackError(RS_PLAYBACK_ERROR_NOTIFICATION_T * notification);
	VOID OnDiskError(RS_DISK_ERROR_NOTIFICATION_T * notification);
	VOID OnKeyFrameMode(RS_KEY_FRAME_MODE_NOTIFICATION_T * notification);
	VOID OnBufferClean(RS_BUFFER_CLEAN_NOTIFICATION_T * notification);
};
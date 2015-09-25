#pragma once

#include <RecorderDLL.h>
#include <LiveSessionDLL.h>
#include <LiveSession5.h>

 /*!
	\brief Receiver Class for Hitron Recorder Server
*/
class NotificationReceiver : public INotificationReceiver
{
public:
	NotificationReceiver( VOID );
	~NotificationReceiver( VOID );

	/*!
		\brief Get Recorder IP Address using UUID.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	CString getRecorderIP(CString recUuid);

	/*!
		\brief Callback function When Another Recorder Client moudle connected to Recorder Server as Administrator previliges.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID OnConnectionStop( RS_CONNECTION_STOP_NOTIFICATION_T *notification );

	/*!
		\brief Callback function When Recording Storage of Recorder Server is fulled.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID OnRecordingStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification );

	/*!
		\brief Callback function When Reserved Storage of Recorder Server is fulled.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID OnReservedStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification );

	/*!
		\brief Callback function When Overwriting is failed in Recorder Server.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID OnOverwritingError( RS_OVERWRITE_ERROR_NOTIFICATION_T *notification );

	/*!
		\brief Callback function When Configuration of Recorder Server is changed.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID OnConfigurationChanged( RS_CONFIGURATION_CHANGED_NOTIFICATION_T *notification );

	/*!
		\brief Callback function When Abnormal Error is occurred during being PlayBack in Recorder Server.
		\param notification : Notification Structure.
		@msc
			CIOCPHandler, win32;
			CIOCPHandler=>win32 [label="CreateIOCompletionPort()"];
		@endmsc
	*/
	VOID OnPlaybackError( RS_PLAYBACK_ERROR_NOTIFICATION_T *notification );
};
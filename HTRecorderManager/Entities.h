#pragma once

#include <vector>

typedef struct _TREE_T
{
	int tree_level;
} TREE_T;

typedef struct _RECORDER_T RECORDER_T;
typedef struct _SITE_T : public _TREE_T
{
	wchar_t uuid[200];
	wchar_t name[200];

	RECORDER_T ** recorders;
	int recordersCount;
} SITE_T;

typedef struct _CAMERA_T CAMERA_T;
typedef struct _RECORDER_T : public _TREE_T
{
	SITE_T * parent;
	wchar_t uuid[200];
	wchar_t name[200];
	wchar_t address[200];
	wchar_t username[200];
	wchar_t pwd[200];

	CAMERA_T ** cameras;
	int camerasCount;
} RECORDER_T;

typedef struct _CAMERA_T : public _TREE_T
{
	RECORDER_T * parent;
	wchar_t uuid[200];
	unsigned char key[8];
	wchar_t address[200];
	wchar_t username[200];
	wchar_t pwd[200];
} CAMERA_T;

/*
void OnConnectionStop(RS_CONNECTION_STOP_NOTIFICATION_T * notification);
void OnRecordingStorageFull(RS_STORAGE_FULL_NOTIFICATION_T * notification);
void OnReservedStorageFull(RS_STORAGE_FULL_NOTIFICATION_T * notification);
void OnOverwritingError(RS_OVERWRITE_ERROR_NOTIFICATION_T * notification);
void OnConfigurationChanged(RS_CONFIGURATION_CHANGED_NOTIFICATION_T * notification);
void OnPlaybackError(RS_PLAYBACK_ERROR_NOTIFICATION_T * notification);
void OnDiskError(RS_DISK_ERROR_NOTIFICATION_T * notification);
void OnKeyFrameMode(RS_KEY_FRAME_MODE_NOTIFICATION_T * notification);
void OnBufferClean(RS_BUFFER_CLEAN_NOTIFICATION_T * notification);
*/

#define CATEGORY_SYSTEM 0
#define CATEGORY_STORAGE 1
#define CATEGORY_PLAYBACK 2

#define LEVEL_INFO 0
#define LEVEL_WARNING 1
#define LEVEL_CIRITICAL 2

typedef struct _STATUS_LOG_T
{
	wchar_t uuid[200];
	int seq;
	int dt;
	int level;
	int category;
	int resid;
	wchar_t contents[500];
} STATUS_LOG_T;
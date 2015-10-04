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
	wchar_t address[200];
	wchar_t username[200];
	wchar_t pwd[200];
} CAMERA_T;

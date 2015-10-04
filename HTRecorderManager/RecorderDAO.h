#pragma once

#include "Entities.h"
#include "DataAccessObject.h"

class RecorderDAO : public DataAccessObject
{
public:
	explicit RecorderDAO(void);
	virtual ~RecorderDAO(void);

	int RetrieveRecorder(SITE_T * site, RECORDER_T *** recorders, int & count, sqlite3 * connection = 0);
	int CreateRecorder(SITE_T * site, RECORDER_T * recorder, sqlite3 * connection = 0);
	int DeleteRecorder(SITE_T * site, RECORDER_T * recorder, sqlite3 * connection = 0);


private:
	int RetrieveRecorderCount(SITE_T * site, sqlite3 * connection);
};

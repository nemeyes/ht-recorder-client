#pragma once

#include "Entities.h"
#include "DataAccessObject.h"

class StatusLogDAO : public DataAccessObject
{
public:
	explicit StatusLogDAO(void);
	virtual ~StatusLogDAO(void);

	int CreateStatusLog(STATUS_LOG_T * log, sqlite3 * connection = 0);

private:
};


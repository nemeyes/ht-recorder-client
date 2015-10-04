#pragma once

#define DAO_SUCCESS		0x00
#define DAO_FAIL		0x01

#include "sqlite3.h"

class DataAccessObject
{
public:
	DataAccessObject(void);
	virtual ~DataAccessObject(void);
	void begin(void);
	void commit(void);
	void rollback(void);
	void begin(sqlite3 * connection);
	void commit(sqlite3 * connection);
	void rollback(sqlite3 * connection);

	sqlite3 * get_connection(void);

protected:
	sqlite3 *_db;
};
#include "StatusLogDAO.h"
#include <ctime>
#include <stdlib.h>
#include <stdio.h>
#include <string>


StatusLogDAO::StatusLogDAO(void)
{

}

StatusLogDAO::~StatusLogDAO(void)
{

}

int StatusLogDAO::CreateStatusLog(STATUS_LOG_T * log, sqlite3 * connection)
{
	int value = DAO_FAIL;
	sqlite3_stmt *stmt;

	sqlite3 *conn;
	if (connection == 0)
		conn = _db;
	else
		conn = connection;

	if (!log)
		return value;

	std::string sql = "INSERT INTO tb_status_log (recorder_uuid, dt, level, category, resid, contents) VALUES (?, ?, ?, ?, ?, ?);";
	int db_error = SQLITE_OK;
	if ((db_error = sqlite3_prepare(conn, sql.c_str(), -1, &stmt, 0)) == SQLITE_OK)
	{
		int index = 0;
		std::time_t t = std::time(0);

		sqlite3_bind_text16(stmt, ++index, log->uuid, -1, 0);
		sqlite3_bind_int(stmt, ++index, t);
		sqlite3_bind_int(stmt, ++index, log->level);
		sqlite3_bind_int(stmt, ++index, log->category);
		sqlite3_bind_int(stmt, ++index, log->resid);
		sqlite3_bind_text16(stmt, ++index, log->contents, -1, 0);

		int result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			value = DAO_SUCCESS;
		}
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
	return value;
}
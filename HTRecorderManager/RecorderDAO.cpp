#include "RecorderDAO.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>

RecorderDAO::RecorderDAO(void)
{

}

RecorderDAO::~RecorderDAO(void)
{

}

int RecorderDAO::RetrieveRecorder(SITE_T * site, RECORDER_T *** recorders, int & count, sqlite3 * connection)
{
	int value = DAO_FAIL;
	sqlite3_stmt *stmt;

	sqlite3 *conn;
	if (connection == 0)
		conn = _db;
	else
		conn = connection;

	if (!site)
		return value;

	count = RetrieveRecorderCount(site, conn);

	*recorders = static_cast<RECORDER_T**>(malloc(sizeof(RECORDER_T*)*count));
	std::string sql = "SELECT uuid, name, address, username, pwd FROM tb_recorder WHERE uuid in (SELECT recorder_uuid FROM tb_site_recorder WHERE site_uuid=?) ORDER BY name ASC;";
	if (sqlite3_prepare(conn, sql.c_str(), -1, &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text16(stmt, 1, site->uuid, -1, 0);
		int result = SQLITE_ERROR;
		int index = 0;
		while (true)
		{
			result = sqlite3_step(stmt);
			if (result == SQLITE_ROW)
			{
				(*recorders)[index] = static_cast<RECORDER_T*>(malloc(sizeof(RECORDER_T)));
				wchar_t * uuid = (wchar_t*)sqlite3_column_text16(stmt, 0);
				wchar_t * name = (wchar_t*)sqlite3_column_text16(stmt, 1);
				wchar_t * address = (wchar_t*)sqlite3_column_text16(stmt, 2);
				wchar_t * username = (wchar_t*)sqlite3_column_text16(stmt, 3);
				wchar_t * pwd = (wchar_t*)sqlite3_column_text16(stmt, 4);

				wcscpy((*recorders)[index]->uuid, uuid);
				wcscpy((*recorders)[index]->name, name);
				wcscpy((*recorders)[index]->address, address);
				wcscpy((*recorders)[index]->username, username);
				wcscpy((*recorders)[index]->pwd, pwd);
				index++;
				value = DAO_SUCCESS;
			}
			else
			{
				if (result == SQLITE_DONE)
					value = DAO_SUCCESS;
				break;
			}
		}
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
	return value;
}

int RecorderDAO::CreateRecorder(SITE_T * site, RECORDER_T * recorder, sqlite3 * connection)
{
	int value = DAO_FAIL;
	sqlite3_stmt * stmt;

	sqlite3 *conn;
	if (connection == 0)
		conn = _db;
	else
		conn = connection;

	if (!site || !recorder)
		return value;

	begin(conn);

	std::string sql = "INSERT INTO tb_recorder (uuid, name, address, username, pwd) VALUES (?,?,?,?,?);";
	int db_error = SQLITE_OK;
	if ((db_error = sqlite3_prepare(conn, sql.c_str(), -1, &stmt, 0)) == SQLITE_OK)
	{
		int index = 0;
		sqlite3_bind_text16(stmt, ++index, recorder->uuid, -1, 0);
		sqlite3_bind_text16(stmt, ++index, recorder->name, -1, 0);
		sqlite3_bind_text16(stmt, ++index, recorder->address, -1, 0);
		sqlite3_bind_text16(stmt, ++index, recorder->username, -1, 0);
		sqlite3_bind_text16(stmt, ++index, recorder->pwd, -1, 0);
		int result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			value = DAO_SUCCESS;
		}
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
	if (value != DAO_SUCCESS)
	{
		rollback(conn);
		return value;
	}

	sql = "INSERT INTO tb_site_recorder (site_uuid, recorder_uuid) VALUES (?,?);";
	if ((db_error = sqlite3_prepare(conn, sql.c_str(), -1, &stmt, 0)) == SQLITE_OK)
	{
		int index = 0;
		sqlite3_bind_text16(stmt, ++index, site->uuid, -1, 0);
		sqlite3_bind_text16(stmt, ++index, recorder->uuid, -1, 0);
		int result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			value = DAO_SUCCESS;
		}
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
	if (value != DAO_SUCCESS)
	{
		rollback(conn);
		return value;
	}
	
	commit(conn);
	return value;
}

int RecorderDAO::DeleteRecorder(SITE_T * site, RECORDER_T * recorder, sqlite3 * connection)
{
	int value = DAO_FAIL;
	sqlite3_stmt *stmt;

	sqlite3 *conn;
	if (connection == 0)
		conn = _db;
	else
		conn = connection;

	if (!site || !recorder)
		return value;

	begin(conn);

	if (sqlite3_prepare(conn, "DELETE FROM tb_site_recorder WHERE site_uuid=? and recorder_uuid=?;", -1, &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text16(stmt, 1, site->uuid, -1, 0);
		sqlite3_bind_text16(stmt, 2, recorder->uuid, -1, 0);
		int result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			value = DAO_SUCCESS;
		}
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
	if (value != DAO_SUCCESS)
	{
		rollback(conn);
		return value;
	}

	if (sqlite3_prepare(conn, "DELETE FROM tb_recorder WHERE uuid=?;", -1, &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text16(stmt, 1, recorder->uuid, -1, 0);
		int result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			value = DAO_SUCCESS;
		}
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
	if (value != DAO_SUCCESS)
	{
		rollback(conn);
		return value;
	}

	commit(conn);
	return value;
}

int RecorderDAO::RetrieveRecorderCount(SITE_T * site, sqlite3 * connection)
{
	int count = 0;
	sqlite3_stmt *stmt;

	sqlite3 *conn;
	if (connection == 0)
		conn = _db;
	else
		conn = connection;

	if (!site)
		return count;

	if (sqlite3_prepare(conn, "SELECT count(*) FROM tb_recorder WHERE uuid in (SELECT recorder_uuid FROM tb_site_recorder WHERE site_uuid=?);", -1, &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text16(stmt, 1, site->uuid, -1, 0);
		int result = SQLITE_ERROR;
		while (true)
		{
			result = sqlite3_step(stmt);
			if (result == SQLITE_ROW)
			{
				count = sqlite3_column_int(stmt, 0);
			}
			else
				break;
		}
	}
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
	return count;
}
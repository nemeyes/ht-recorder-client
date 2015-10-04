#include "SiteDAO.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>

SiteDAO::SiteDAO(void)
{

}

SiteDAO::~SiteDAO(void)
{

}

int SiteDAO::RetrieveSites(SITE_T *** sites, int & count, sqlite3 * connection)
{
	int value = DAO_FAIL;
	sqlite3_stmt *stmt;

	sqlite3 *conn;
	if (connection == 0)
		conn = _db;
	else
		conn = connection;

	count = RetrieveSitesCount(conn);

	*sites = static_cast<SITE_T**>(malloc(sizeof(SITE_T*)*count));
	std::string sql = "SELECT uuid, name FROM tb_site ORDER BY name ASC;";
	if (sqlite3_prepare(conn, sql.c_str(), -1, &stmt, 0) == SQLITE_OK)
	{
		int result = SQLITE_ERROR;
		int index = 0;
		while (true)
		{
			result = sqlite3_step(stmt);
			if (result == SQLITE_ROW)
			{
				(*sites)[index] = static_cast<SITE_T*>(malloc(sizeof(SITE_T)));
				wchar_t * uuid = (wchar_t*)sqlite3_column_text16(stmt, 0);
				wchar_t * name = (wchar_t*)sqlite3_column_text16(stmt, 1);
				wcscpy((*sites)[index]->uuid, uuid);
				wcscpy((*sites)[index]->name, name);
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

int SiteDAO::CreateSite(SITE_T * site, sqlite3 * connection)
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

	std::string sql = "INSERT INTO tb_site (uuid, name) VALUES (?,?);";
	int db_error = SQLITE_OK;
	if ((db_error = sqlite3_prepare(conn, sql.c_str(), -1, &stmt, 0)) == SQLITE_OK)
	{
		int index = 0;
		sqlite3_bind_text16(stmt, ++index, site->uuid, -1, 0);
		sqlite3_bind_text16(stmt, ++index, site->name, -1, 0);

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

int SiteDAO::DeleteSite(SITE_T * site, sqlite3 * connection)
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

	begin(conn);
	if (sqlite3_prepare(conn, "DELETE FROM tb_recorder WHERE uuid IN (SELECT recorder_uuid FROM tb_site_recorder WHERE site_uuid=?);", -1, &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text16(stmt, 1, site->uuid, -1, 0);
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

	if (sqlite3_prepare(conn, "DELETE FROM tb_site_recorder WHERE site_uuid=?;", -1, &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text16(stmt, 1, site->uuid, -1, 0);
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

	if (sqlite3_prepare(conn, "DELETE FROM tb_site WHERE uuid=?;", -1, &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text16(stmt, 1, site->uuid, -1, 0);
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

int SiteDAO::RetrieveSitesCount(sqlite3 * connection)
{
	int count = 0;
	sqlite3_stmt *stmt;

	sqlite3 *conn;
	if (connection == 0)
		conn = _db;
	else
		conn = connection;

	if (sqlite3_prepare(conn, "SELECT count(*) FROM tb_site;", -1, &stmt, 0) == SQLITE_OK)
	{
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
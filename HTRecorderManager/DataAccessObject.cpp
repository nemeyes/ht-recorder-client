#include "DataAccessObject.h"
#include <stdlib.h>
#include <stdio.h>

DataAccessObject::DataAccessObject( void )
{
	sqlite3_enable_shared_cache( true );
	if( sqlite3_open("db/db.sqlite", &_db)!=SQLITE_OK )
	{
		printf( "db connection error\n" );
	}
}

DataAccessObject::~DataAccessObject( void )
{
	sqlite3_close( _db );
	printf( "db is closed\n" );
}

sqlite3* DataAccessObject::get_connection( void )
{
	return _db;
}

void DataAccessObject::begin( void )
{
	sqlite3_exec( _db, "BEGIN TRANSACTION;", 0, 0, 0 );
}

void DataAccessObject::commit( void )
{
	sqlite3_exec( _db, "COMMIT TRANSACTION;", 0, 0, 0 );
}

void DataAccessObject::rollback( void )
{
	sqlite3_exec( _db, "ROLLBACK TRANSACTION;", 0, 0, 0 );
}

void DataAccessObject::begin( sqlite3 *connection )
{
	sqlite3_exec( connection, "BEGIN TRANSACTION;", 0, 0, 0 );
}

void DataAccessObject::commit( sqlite3 *connection )
{
	sqlite3_exec( connection, "COMMIT TRANSACTION;", 0, 0, 0 );
}

void DataAccessObject::rollback( sqlite3 *connection )
{
	sqlite3_exec( connection, "ROLLBACK TRANSACTION;", 0, 0, 0 );
}

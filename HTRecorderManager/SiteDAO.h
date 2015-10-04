#pragma once

#include "Entities.h"
#include "DataAccessObject.h"

class SiteDAO : public DataAccessObject
{
public:
	explicit SiteDAO(void);
	virtual ~SiteDAO(void);

	int RetrieveSites(SITE_T *** sites, int & count, sqlite3 * connection = 0);
	int CreateSite(SITE_T * site, sqlite3 * connection = 0);
	int DeleteSite(SITE_T * site, sqlite3 * connection = 0);


private:
	int RetrieveSitesCount(sqlite3 * connection);
};


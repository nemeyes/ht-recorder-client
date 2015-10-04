#pragma once

#pragma region "CHTDateTime Definition"


class CHTDateTimeSpan : public CTimeSpan
{
public:
	CHTDateTimeSpan() throw();
	CHTDateTimeSpan( __time64_t time ) throw();
	CHTDateTimeSpan( __time64_t time, int nMSecs ) throw();
	CHTDateTimeSpan( LONG lDays, int nHours, int nMins, int nSecs ) throw();
	CHTDateTimeSpan( LONG lDays, int nHours, int nMins, int nSecs, int nMSecs ) throw();

	LONGLONG GetTotalMiliseconds() const throw();
	LONG GetMiliseconds() { return m_nMillisecond; };

	CHTDateTimeSpan operator+( CHTDateTimeSpan span ) const throw();
	CHTDateTimeSpan operator-( CHTDateTimeSpan span ) const throw();
	CHTDateTimeSpan& operator+=( CHTDateTimeSpan span ) throw();
	CHTDateTimeSpan& operator-=( CHTDateTimeSpan span ) throw();
	bool operator==( CHTDateTimeSpan span ) const throw();
	bool operator!=( CHTDateTimeSpan span ) const throw();
	bool operator<( CHTDateTimeSpan span ) const throw();
	bool operator>( CHTDateTimeSpan span ) const throw();
	bool operator<=( CHTDateTimeSpan span ) const throw();
	bool operator>=( CHTDateTimeSpan span ) const throw();

protected:
	int m_nMillisecond;

	void Initialize(int nMSecs = 0);
};

#pragma endregion

class CHTDateTime : public CTime
{
public:
	typedef struct _ntp64
	{
		UINT int_part;
		UINT fraction;

	} ntp64;

	CHTDateTime() throw();
	CHTDateTime( __time64_t time ) throw();
	CHTDateTime( __time64_t time, int nMSec ) throw();
	CHTDateTime( ntp64 ntptime );
	CHTDateTime( int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1 );
	CHTDateTime( int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nMSec, int nDST = -1 );
	CHTDateTime( WORD wDosDate, WORD wDosTime, int nDST = -1 );
	CHTDateTime( const SYSTEMTIME& st, int nDST = -1 );
	CHTDateTime( const FILETIME& ft, int nDST = -1 );

#ifdef __oledb_h__
	CHTDateTime( const DBTIMESTAMP& dbts, int nDST = -1 ) throw();
#endif

	CHTDateTime(UINT nDate, UINT nTime);
	CHTDateTime(CString strDate, CString strTime);

	CHTDateTime& operator=( __time64_t time ) throw();
	CHTDateTime& operator=( CTime time ) throw();

	CHTDateTime& operator+=( CHTDateTimeSpan span ) throw();
	CHTDateTime& operator-=( CHTDateTimeSpan span ) throw();

	CHTDateTimeSpan operator-( CHTDateTime time ) const throw();
	CHTDateTime operator-( CHTDateTimeSpan span ) const throw();
	CHTDateTime operator+( CHTDateTimeSpan span ) const throw();

	bool operator==( CHTDateTime time ) const throw();
	bool operator!=( CHTDateTime time ) const throw();
	bool operator<( CHTDateTime time ) const throw();
	bool operator>( CHTDateTime time ) const throw();
	bool operator<=( CHTDateTime time ) const throw();
	bool operator>=( CHTDateTime time ) const throw();

	// Converts a CTime object into a formatted string - based on the local time zone(Include miliseconds)
	// formatting code is '%s' for miliseconds
	CString FormatEx( LPCTSTR pszFormat ) const;
	// Converts a CTime object into a formatted string - based UTC(Include miliseconds)
	// formatting code is '%s' for miliseconds
	CString FormatGmtEx( LPCTSTR pszFormat ) const;
	CString FormatEx( UINT nFormatID ) const 
	{ 
		CString strFormat;
		strFormat.LoadString(nFormatID);
		return FormatEx(CString(strFormat)); 
	};

	CString FormatGmtEx( UINT nFormatID ) const 
	{ 
		CString strFormat;
		strFormat.LoadString(nFormatID);
		return FormatGmtEx(strFormat); 
	};

	UINT GetHTDate();
	UINT GetHTTime();
	unsigned __int64	GetHTDateTime();

	int GetMilisecond() { return m_nMillisecond; };

	bool GetAsSystemTime( SYSTEMTIME& st ) const throw();

	static CHTDateTime WINAPI GetCurrentTime() throw();

	void ntp64_time(UINT* ntp_sec, UINT* ntp_frac);
protected:
	int	m_nMillisecond;

	void Initialize(int nMilisecond = 0);

	UINT ConvertToHTDate(__time64_t tTime);
	UINT ConvertToHTTime(__time64_t tTime, int nMSec = 0);
	__time64_t ConvertToDateTime(UINT nHTDate, UINT nHTTime, int* pMSec);
};

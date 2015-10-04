#include "Common.h"
#include "HTDateTime.h"

#define ADJUST_TIME(x, y)				((x) * 1000 + (y)) / 1000
#define ADJUST_MSEC(x)			(x) >= 0 ? (x) % 1000 : 1000 + ((x) % 1000)

static const unsigned __int64 EPOCH				= 2208988800ULL;
static const unsigned __int64 NTP_SCALE_FRAC	= 4294967295ULL;
#define FMAXINT	(4294967296.0)        /* floating point rep. of MAXINT */


#pragma region "CHTDateTimeSpan Implementation"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// CHTDateTimeSpan
CHTDateTimeSpan::CHTDateTimeSpan() throw()
: CTimeSpan()
{
	Initialize();
}

CHTDateTimeSpan::CHTDateTimeSpan( __time64_t time ) throw()
: CTimeSpan(time)
{
	Initialize();
}

CHTDateTimeSpan::CHTDateTimeSpan( __time64_t time, int nMSecs ) throw()
: CTimeSpan(time)
{
	Initialize(nMSecs);
}

CHTDateTimeSpan::CHTDateTimeSpan( LONG lDays, int nHours, int nMins, int nSecs ) throw()
: CTimeSpan(lDays, nHours, nMins, nSecs)
{
	Initialize();
}

CHTDateTimeSpan::CHTDateTimeSpan( LONG lDays, int nHours, int nMins, int nSecs, int nMSecs ) throw()
: CTimeSpan(lDays, nHours, nMins, nSecs)
{
	Initialize(nMSecs);
}

void CHTDateTimeSpan::Initialize(int nMSecs/* = 0*/)
{
	__time64_t tNewSpan = ADJUST_TIME(GetTimeSpan(), nMSecs);
	CopyMemory(this, &tNewSpan, sizeof(tNewSpan));
	m_nMillisecond = ADJUST_MSEC(nMSecs);
}

LONGLONG CHTDateTimeSpan::GetTotalMiliseconds() const throw()
{
	return (GetTimeSpan() * 1000) + m_nMillisecond;
}



CHTDateTimeSpan CHTDateTimeSpan::operator+( CHTDateTimeSpan span ) const throw()
{
	int nMSecs = m_nMillisecond + span.GetMiliseconds();
	__time64_t tNewTime = GetTimeSpan() + span.GetTimeSpan();
	return CHTDateTimeSpan(ADJUST_TIME(tNewTime, nMSecs), ADJUST_MSEC(nMSecs));
}

CHTDateTimeSpan CHTDateTimeSpan::operator-( CHTDateTimeSpan span ) const throw()
{
	int nMSecs = m_nMillisecond - span.GetMiliseconds();
	return CHTDateTimeSpan(ADJUST_TIME(GetTimeSpan() - span.GetTimeSpan(), nMSecs), ADJUST_MSEC(nMSecs));
}

CHTDateTimeSpan& CHTDateTimeSpan::operator+=( CHTDateTimeSpan span ) throw()
{
	__time64_t tNewSpan = GetTimeSpan() + span.GetTimeSpan();
	int nNewMSecs = m_nMillisecond + span.GetMiliseconds();
	tNewSpan = ADJUST_TIME(tNewSpan, nNewMSecs);

	CopyMemory(this, &tNewSpan, sizeof(tNewSpan));
	m_nMillisecond = ADJUST_MSEC(nNewMSecs);
	return (*this);

}

CHTDateTimeSpan& CHTDateTimeSpan::operator-=( CHTDateTimeSpan span ) throw()
{
	__time64_t tNewSpan = GetTimeSpan() - span.GetTimeSpan();
	int nNewMSecs = m_nMillisecond - span.GetMiliseconds();
	tNewSpan = ADJUST_TIME(tNewSpan, nNewMSecs);

	CopyMemory(this, &tNewSpan, sizeof(tNewSpan));
	m_nMillisecond = ADJUST_MSEC(nNewMSecs);
	return (*this);
}

bool CHTDateTimeSpan::operator==( CHTDateTimeSpan span ) const throw()
{
	return ((GetTimeSpan() == span.GetTimeSpan()) && 
		(m_nMillisecond == span.GetMiliseconds()));
}

bool CHTDateTimeSpan::operator!=( CHTDateTimeSpan span ) const throw()
{
	return ((GetTimeSpan() != span.GetTimeSpan()) || 
		(m_nMillisecond != span.GetMiliseconds()));
}

bool CHTDateTimeSpan::operator<( CHTDateTimeSpan span ) const throw()
{
	return (GetTimeSpan() < span.GetTimeSpan()) ||
		((GetTimeSpan() == span.GetTimeSpan()) && (m_nMillisecond < span.GetMiliseconds()));
}

bool CHTDateTimeSpan::operator>( CHTDateTimeSpan span ) const throw()
{
	return (GetTimeSpan() > span.GetTimeSpan()) ||
		((GetTimeSpan() == span.GetTimeSpan()) && (m_nMillisecond > span.GetMiliseconds()));
}

bool CHTDateTimeSpan::operator<=( CHTDateTimeSpan span ) const throw()
{
	return operator<(span) || operator==(span);
}

bool CHTDateTimeSpan::operator>=( CHTDateTimeSpan span ) const throw()
{
	return operator>(span) || operator==(span);
}


#pragma endregion


///////////////////////////////////////////////////////////////////////////////////////////////////////
// CHTDateTime
CHTDateTime::CHTDateTime() throw()
: CTime()
{
	Initialize();
}

CHTDateTime::CHTDateTime( __time64_t time ) throw()
: CTime(time)
{
	Initialize();
}

CHTDateTime::CHTDateTime( __time64_t time, int nMSec ) throw()
: CTime(time)
{
	Initialize(nMSec);
}

CHTDateTime::CHTDateTime( ntp64 ntptime )
: CTime(__time64_t(ntptime.int_part - EPOCH))
{
	float ff = ntptime.fraction;
	if(ff < 0)
		ff += FMAXINT;
	ff = ff / FMAXINT;

	Initialize(UINT(ff * 1000000.0));
}

CHTDateTime::CHTDateTime( int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST/* = -1 */)
: CTime(nYear, nMonth, nDay, nHour, nMin, nSec, nDST)
{
	Initialize();
}

CHTDateTime::CHTDateTime( int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nMSec, int nDST/* = -1 */)
: CTime(nYear, nMonth, nDay, nHour, nMin, nSec, nDST)
{
	Initialize(nMSec);
}


CHTDateTime::CHTDateTime( WORD wDosDate, WORD wDosTime, int nDST/* = -1 */)
: CTime(wDosDate, wDosTime, nDST)
{
	Initialize();
}

CHTDateTime::CHTDateTime( const SYSTEMTIME& st, int nDST/* = -1 */)
: CTime(st, nDST)
{
	Initialize(st.wMilliseconds);
}

CHTDateTime::CHTDateTime( const FILETIME& ft, int nDST/* = -1 */)
: CTime(ft, nDST)
{
	Initialize();
}

CHTDateTime::CHTDateTime(UINT nDate, UINT nTime)
{
	int nMSec = 0;
	__time64_t tNewValue = ConvertToDateTime(nDate, nTime, &nMSec);
	CopyMemory(this, &tNewValue, sizeof(tNewValue));
	Initialize(nMSec);
}

CHTDateTime::CHTDateTime(CString strDate, CString strTime)
{
	int nMSec = 0;
	__time64_t tNewValue = ConvertToDateTime(_ttoi(strDate), _ttoi(strTime), &nMSec);
	CopyMemory(this, &tNewValue, sizeof(tNewValue));
	Initialize(nMSec);
}

void CHTDateTime::Initialize(int nMilisecond/* = 0*/)
{
	__time64_t tNewValue = ADJUST_TIME(GetTime(), nMilisecond);
	CopyMemory(this, &tNewValue, sizeof(tNewValue));
	m_nMillisecond = ADJUST_MSEC(nMilisecond);
}


UINT CHTDateTime::ConvertToHTDate(__time64_t tTime)
{
	CTime clsTime = CTime(tTime);
	UINT uResult = 0;
	uResult += (clsTime.GetYear() * 10000);
	uResult += (clsTime.GetMonth() * 100);
	uResult += clsTime.GetDay();
	return uResult;
}

UINT CHTDateTime::ConvertToHTTime(__time64_t tTime, int nMSec/* = 0*/)
{
	CTime clsTime = CTime(tTime);
	UINT uResult = 0;
	uResult += (clsTime.GetHour() * 10000000);
	uResult += (clsTime.GetMinute() * 100000);
	uResult += (clsTime.GetSecond() * 1000);
	uResult += nMSec;
	return uResult;
}

__time64_t CHTDateTime::ConvertToDateTime(UINT nHTDate, UINT nHTTime, int* pMSec)
{
	UINT nTemp;
	int nYear = nHTDate / 10000;
	nTemp = nHTDate % 10000;

	int nMonth = nTemp / 100;
	nTemp = nTemp % 100;

	int nDay = nTemp;

	int nHour = nHTTime / 10000000;
	nTemp = nHTTime % 10000000;

	int nMinute = nTemp / 100000;
	nTemp = nTemp % 100000;

	int nSecond = nTemp / 1000;
	nTemp = nTemp % 1000;

	*pMSec = nTemp;

	CTime clsTime(nYear, nMonth, nDay, nHour, nMinute, nSecond);
	return clsTime.GetTime();
}

UINT CHTDateTime::GetHTDate()
{
	return ConvertToHTDate(GetTime());
}

UINT CHTDateTime::GetHTTime()
{
	return ConvertToHTTime(GetTime(), m_nMillisecond);
}

unsigned __int64 CHTDateTime::GetHTDateTime()
{
	unsigned __int64 nDate64 = GetHTDate();

	return (nDate64 * 1000000000) + GetHTTime();
}


CHTDateTime WINAPI CHTDateTime::GetCurrentTime() throw()
{
	SYSTEMTIME stTime = {0};
	GetLocalTime(&stTime);
	return CHTDateTime(stTime);
}


CHTDateTime& CHTDateTime::operator=( __time64_t time ) throw()
{
	CopyMemory(this, &time, sizeof(time));
	return (*this);
}

CHTDateTime& CHTDateTime::operator=( CTime time ) throw()
{
	CopyMemory(this, &time, sizeof(time));
	return (*this);
}


CHTDateTime& CHTDateTime::operator+=( CHTDateTimeSpan span ) throw()
{
	__time64_t tNewValue = GetTime() + span.GetTimeSpan();
	int nNewMSec = m_nMillisecond + span.GetMiliseconds();
	tNewValue = ADJUST_TIME(tNewValue, nNewMSec);
	
	CopyMemory(this, &tNewValue, sizeof(tNewValue));
	m_nMillisecond = ADJUST_MSEC(nNewMSec);
	return (*this);
}

CHTDateTime& CHTDateTime::operator-=( CHTDateTimeSpan span ) throw()
{
	__time64_t tNewValue = GetTime() - span.GetTimeSpan();
	int nNewMSec = m_nMillisecond - span.GetMiliseconds();
	tNewValue = ADJUST_TIME(tNewValue, nNewMSec);

	CopyMemory(this, &tNewValue, sizeof(tNewValue));
	m_nMillisecond = ADJUST_MSEC(nNewMSec);
	return (*this);
}

CHTDateTimeSpan CHTDateTime::operator-( CHTDateTime time ) const throw()
{
	int nMSecs = m_nMillisecond - time.GetMilisecond();
	__time64_t tNewValue = ADJUST_TIME(GetTime() - time.GetTime(), nMSecs);
	int nNewMSec = ADJUST_MSEC(nMSecs);
	return CHTDateTimeSpan(tNewValue, nNewMSec);
}

CHTDateTime CHTDateTime::operator-( CHTDateTimeSpan span ) const throw()
{
	int nMSecs = m_nMillisecond - span.GetMiliseconds();
	return CHTDateTime(ADJUST_TIME(GetTime() - span.GetTimeSpan(), nMSecs), ADJUST_MSEC(nMSecs));
}

CHTDateTime CHTDateTime::operator+( CHTDateTimeSpan span ) const throw()
{
	int nMSecs = m_nMillisecond + span.GetMiliseconds();
	return CHTDateTime(ADJUST_TIME(GetTime() + span.GetTimeSpan(), nMSecs), ADJUST_MSEC(nMSecs));
}

bool CHTDateTime::operator==( CHTDateTime time ) const throw()
{
	return ((GetTime() == time.GetTime()) &&
		(m_nMillisecond == time.GetMilisecond()));
}

bool CHTDateTime::operator!=( CHTDateTime time ) const throw()
{
	return ((GetTime() != time.GetTime()) ||
		(m_nMillisecond != time.GetMilisecond()));
}

bool CHTDateTime::operator<( CHTDateTime time ) const throw()
{
	return (GetTime() < time.GetTime()) ||
		((GetTime() == time.GetTime()) && (m_nMillisecond < time.GetMilisecond()));
}

bool CHTDateTime::operator>( CHTDateTime time ) const throw()
{
	return (GetTime() > time.GetTime()) ||
		((GetTime() == time.GetTime()) && (m_nMillisecond > time.GetMilisecond()));
}

bool CHTDateTime::operator<=( CHTDateTime time ) const throw()
{
	return operator<(time) || operator==(time);
}

bool CHTDateTime::operator>=( CHTDateTime time ) const throw()
{
	return operator>(time) || operator==(time);
}


CString CHTDateTime::FormatEx( LPCTSTR pszFormat ) const
{
	CString strResult = pszFormat;
	CString strMSec;

	strMSec.Format(L"%.3d", m_nMillisecond);
	strResult.Replace(L"%s", strMSec);
	strResult = Format(strResult);
	return strResult;
}

CString CHTDateTime::FormatGmtEx( LPCTSTR pszFormat ) const
{
	CString strResult = pszFormat;
	CString strMSec;

	strMSec.Format(L"%.3d", m_nMillisecond);
	strResult.Replace(L"%s", strMSec);
	strResult = FormatGmt(strResult);
	return strResult;
}



bool CHTDateTime::GetAsSystemTime( SYSTEMTIME& st ) const throw()
{
	bool bResult = CTime::GetAsSystemTime(st);

	if (bResult)
	{
		st.wMilliseconds = m_nMillisecond;
	}

	return bResult;
}

void CHTDateTime::ntp64_time(UINT* ntp_sec, UINT* ntp_frac)
{
	struct tm cur;
	UINT tmp;
	GetLocalTm(&cur);
	
	time_t tTime = mktime(&cur);

	*ntp_sec = tTime + EPOCH;

	tmp = m_nMillisecond;
	*ntp_frac = (tmp << 12) + (tmp << 8) - ((tmp * 3650) >> 6);
}

//unsigned __int64 CHTDateTime::ntp64_time()
//{
//	unsigned __int64 tv_ntp, tv_usecs;
//
//	struct tm cur;
//	GetLocalTm(&cur);
//	
//	time_t tTime = mktime(&cur);
//	
//	tv_ntp = tTime + EPOCH;
//	tv_usecs = (NTP_SCALE_FRAC * m_nMillisecond) / 1000000ULL;
//
//	return ((tv_ntp << 32) | tv_usecs);
//}
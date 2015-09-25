#pragma once
#include "ExceptionBase.h"

class COperationFailException : public CExceptionBase
{
public:
	COperationFailException()
	{
	};

	COperationFailException(const LPCTSTR lpMessage)
	{
	};

	COperationFailException(enum EXCEPTION_LEVEL_TYPE eType, LPCTSTR lpMessage)
	{
	};

	COperationFailException(enum EXCEPTION_LEVEL_TYPE eType, LPCTSTR lpCategory, LPCTSTR lpMessage)
	{
	};

	COperationFailException(const CExceptionBase& e)
	{
	};
};
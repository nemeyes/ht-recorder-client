#pragma once
#include "HTRecorderIF.h"

class HTRecorderFactory
{
public:
	static HTRecorderIF & GetInstance()
	{
		static HTRecorderIF instance(FALSE);
		return instance;
	}

private:
	HTRecorderFactory();
	HTRecorderFactory(const HTRecorderFactory & clone);
	~HTRecorderFactory();


};
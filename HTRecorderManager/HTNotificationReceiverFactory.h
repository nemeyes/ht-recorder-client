#pragma once
#include "HTNotificationReceiver.h"

class HTNotificationReceiverFactory
{
public:
	static HTNotificationReceiver & GetInstance()
	{
		static HTNotificationReceiver instance;
		return instance;
	}

private:
	HTNotificationReceiverFactory();
	HTNotificationReceiverFactory(const HTNotificationReceiverFactory & clone);
	~HTNotificationReceiverFactory();
};
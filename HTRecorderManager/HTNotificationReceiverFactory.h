#pragma once
#include "HTNotificationReceiver.h"
#include <map>


class HTNotificationReceiverFactory
{
public:
	static HTNotificationReceiverFactory & GetInstance()
	{
		static HTNotificationReceiverFactory instance;
		return instance;
	}

	HTNotificationReceiver * GetNotifier(wchar_t * recorder_uuid)
	{
		HTNotificationReceiver * receiver = nullptr;
		std::map<std::wstring, HTNotificationReceiver*>::iterator iter = _notifiers.find(recorder_uuid);
		if (iter != _notifiers.end())
		{
			receiver = (*iter).second;
		}
		else
		{
			receiver = new HTNotificationReceiver();
			receiver->SetEventListWindow(_eventListWindows);
			_notifiers.insert(std::make_pair(recorder_uuid, receiver));
		}
		return receiver;
	}

	void SetEventListWindow(CWnd * wnd)
	{
		HTNotificationReceiver * receiver = nullptr;
		std::map<std::wstring, HTNotificationReceiver*>::iterator iter;
		for (iter = _notifiers.begin(); iter != _notifiers.end(); iter++)
		{
			receiver = (*iter).second;
			if (receiver != nullptr)
			{
				receiver->SetEventListWindow(wnd);
			}
		}
		_eventListWindows = wnd;
	}
private:
	CWnd * _eventListWindows;
	std::map<std::wstring, HTNotificationReceiver*> _notifiers;

private:
	HTNotificationReceiverFactory() {}
	HTNotificationReceiverFactory(const HTNotificationReceiverFactory & clone);
	~HTNotificationReceiverFactory() 
	{
		HTNotificationReceiver * receiver = nullptr;
		std::map<std::wstring, HTNotificationReceiver*>::iterator iter;
		for (iter = _notifiers.begin(); iter != _notifiers.end(); iter++)
		{
			receiver = (*iter).second;
			if (receiver != nullptr)
			{
				delete receiver;
				receiver = nullptr;
			}
		}
	}
};

// 180.71.14.39
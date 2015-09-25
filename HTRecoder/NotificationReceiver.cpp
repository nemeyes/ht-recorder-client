#include "stdafx.h"
#include "NotificationReceiver.h"

#ifdef WITH_HITRON_RECORDER

NotificationReceiver::NotificationReceiver( VOID )
{

}

NotificationReceiver::~NotificationReceiver( VOID )
{


}

CString NotificationReceiver::getRecorderIP(CString recUuid)
{
	CString ipAddress;
	for(int i=0;i<gChannelManager->GetCount();i++)
	{
		CChannelInfo *chInfo = gChannelManager->GetByIndex( i );
		if(StrCmpW(chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdUuid, recUuid)==0)
		{
			ipAddress.Format(TEXT("Recorder(%s)"), chInfo->m_vamInfoEx->m_vcamInfo.vcamRcrdIP);
			return ipAddress;
		}
	}
}

VOID NotificationReceiver::OnConnectionStop( RS_CONNECTION_STOP_NOTIFICATION_T *notification )
{
	CString strMsg;
#ifdef LANGUAGEPACK
	strMsg=g_LanguageMapClass->Get(g_nLanguageType,L"S220")+getRecorderIP(notification->strUUID);
	CAlertMessage alertDlg(NULL, strMsg, g_LanguageMapClass->Get(g_nLanguageType,L"S221"), VMS_OK);
#else
	strMsg=getRecorderIP(notification->strUUID)+TEXT("에 다른 관리자가");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("로그인 하였습니다. 프로그램을 종료합니다."), VMS_OK);
#endif
	if(alertDlg.DoModal() == IDOK)
	{
		//VMS_MSG msg;
		//msg.msg = SHUTDOWN_PROCESS;
		//gQueueManager->AddMessage(msg);
	//	g_mainDlg->shutdownProcess();
		g_mainDlg->OnClickedButtonClose();
	}
}

VOID NotificationReceiver::OnRecordingStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification )
{
	CString strMsg;
#ifdef LANGUAGEPACK
	strMsg=g_LanguageMapClass->Get(g_nLanguageType,L"S222")+getRecorderIP(notification->strUUID);
	CAlertMessage alertDlg(NULL, strMsg, g_LanguageMapClass->Get(g_nLanguageType,L"S223"), VMS_OK);
#else
	strMsg=getRecorderIP(notification->strUUID)+TEXT("의 저장 공간이 부족합니다.");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("남은 저장 공간을 확인하여 주십시오."), VMS_OK);
#endif
	if(alertDlg.DoModal() == IDOK)
	{
		if(g_2DGroupInfo!=NULL)
		{
			g_mainDlg->m_recordSetupDlg->m_disk->LoadDiskInfo();
			g_mainDlg->m_recordSetupDlg->m_schedule->LoadScheduleInfo();
			g_mainDlg->m_recordSetupDlg->m_flag_show = TRUE;
			g_mainDlg->m_recordSetupDlg->ShowWindow(SW_SHOW);
		}
	}
}

VOID NotificationReceiver::OnReservedStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification )
{
	CString strMsg;
#ifdef LANGUAGEPACK
	strMsg=g_LanguageMapClass->Get(g_nLanguageType,L"S222")+getRecorderIP(notification->strUUID);
	CAlertMessage alertDlg(NULL, strMsg, g_LanguageMapClass->Get(g_nLanguageType,L"S223"), VMS_OK);
#else
	strMsg=getRecorderIP(notification->strUUID)+TEXT("의 저장 공간이 부족합니다.");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("남은 저장 공간을 확인하여 주십시오."), VMS_OK);
#endif
	if(alertDlg.DoModal() == IDOK)
	{
		if(g_2DGroupInfo!=NULL)
		{
			g_mainDlg->m_recordSetupDlg->m_disk->LoadDiskInfo();
			g_mainDlg->m_recordSetupDlg->m_schedule->LoadScheduleInfo();
			g_mainDlg->m_recordSetupDlg->m_flag_show = TRUE;
			g_mainDlg->m_recordSetupDlg->ShowWindow(SW_SHOW);
		}
	}
}

VOID NotificationReceiver::OnOverwritingError( RS_OVERWRITE_ERROR_NOTIFICATION_T *notification )
{
	CString strMsg;
#ifdef LANGUAGEPACK
	strMsg=g_LanguageMapClass->Get(g_nLanguageType, L"S224")+getRecorderIP(notification->strUUID);
	CAlertMessage alertDlg(NULL, strMsg, g_LanguageMapClass->Get(g_nLanguageType, L"S225"), VMS_OK);
#else
	strMsg=getRecorderIP(notification->strUUID)+TEXT(" 덮어쓰기에 실패하였습니다.");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("Recorder의 상태를 확인하여 주십시오."), VMS_OK);
#endif
	if(alertDlg.DoModal() == IDOK)
	{
		return;
	}
}

VOID NotificationReceiver::OnConfigurationChanged( RS_CONFIGURATION_CHANGED_NOTIFICATION_T *notification )
{
#ifdef LANGUAGEPACK
	g_mainDlg->SendLog(g_LanguageMapClass->Get(g_nLanguageType,L"S233"));
#else
	g_mainDlg->SendLog(L"레코더의 환경 설정이 변경되었습니다.");
#endif
	
/*
	CString strMsg;
	strMsg=getRecorderIP(notification->strUUID)+TEXT("다른 관리자에 의하여 환경 설정이 변경되었습니다.");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("Recorder의 상태를 확인하여 주십시오."), VMS_OK);
	if(alertDlg.DoModal() == IDOK)
	{
		return;
	}
*/
}

VOID NotificationReceiver::OnPlaybackError( RS_PLAYBACK_ERROR_NOTIFICATION_T *notification )
{
	CString strMsg;
#ifdef LANGUAGEPACK
	strMsg=g_LanguageMapClass->Get(g_nLanguageType,L"S226")+getRecorderIP(notification->strUUID);
	CAlertMessage alertDlg(NULL, strMsg, g_LanguageMapClass->Get(g_nLanguageType,L"S225"), VMS_OK);
#else
	strMsg=getRecorderIP(notification->strUUID)+TEXT(" 재생에 실패하였습니다.");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("Recorder의 상태를 확인하여 주십시오."), VMS_OK);
#endif
	if(alertDlg.DoModal() == IDOK)
	{
		return;
	}
}

#endif
#include "stdafx.h"
#include "HTNotificationReceiver.h"


HTNotificationReceiver::HTNotificationReceiver( VOID )
{

}

HTNotificationReceiver::~HTNotificationReceiver( VOID )
{


}

CString HTNotificationReceiver::GetRecorderIP(CString strRecorderUuid)
{
	/*
	CString ipAddress;

	if(gChannelManager)
	{
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
*/
	return NULL;
}

VOID HTNotificationReceiver::OnConnectionStop( RS_CONNECTION_STOP_NOTIFICATION_T *notification )
{
	CString strMsg;
	strMsg=GetRecorderIP(notification->strUUID)+TEXT("에 다른 관리자가 로그인하였습니다.");
	//SendLog(strMsg, LOG_OPERATION);
	return;
/*
	CString strMsg;
#ifdef LANGUAGEPACK
	strMsg=g_LanguageMapClass->Get(g_nLanguageType,L"S220")+getRecorderIP(notification->strUUID);
	CAlertMessage alertDlg(NULL, strMsg, g_LanguageMapClass->Get(g_nLanguageType,L"S221"), VMS_OK);
#else
	strMsg=getRecorderIP(notification->strUUID)+TEXT("에 다른 관리자가");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("로그인 하였습니다. 프로그램을 종료합니다."), VMS_OK);
#endif
	if(alertDlg.DoModal() == IDOK && g_mainDlg)
	{
		//VMS_MSG msg;
		//msg.msg = SHUTDOWN_PROCESS;
		//gQueueManager->AddMessage(msg);
	//	g_mainDlg->shutdownProcess();
		g_mainDlg->OnClickedButtonClose();
	}
*/
}

VOID HTNotificationReceiver::OnRecordingStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification )
{
	CString strMsg;
	strMsg = GetRecorderIP(notification->strUUID) + TEXT("의 저장 공간이 부족합니다.");
	//SendLog(strMsg, LOG_OPERATION);
	return;
/*
#ifdef LANGUAGEPACK
	strMsg=g_LanguageMapClass->Get(g_nLanguageType,L"S222")+getRecorderIP(notification->strUUID);
	CAlertMessage alertDlg(NULL, strMsg, g_LanguageMapClass->Get(g_nLanguageType,L"S223"), VMS_OK);
#else
	strMsg=getRecorderIP(notification->strUUID)+TEXT("의 저장 공간이 부족합니다.");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("남은 저장 공간을 확인하여 주십시오."), VMS_OK);
#endif
	if(alertDlg.DoModal() == IDOK)
	{
		if(g_2DGroupInfo!=NULL && g_mainDlg)
		{
			g_mainDlg->m_recordSetupDlg->m_disk->LoadDiskInfo();
			g_mainDlg->m_recordSetupDlg->m_schedule->LoadScheduleInfo();
			g_mainDlg->m_recordSetupDlg->m_flag_show = TRUE;
			g_mainDlg->m_recordSetupDlg->ShowWindow(SW_SHOW);
		}
	}
*/
}

VOID HTNotificationReceiver::OnReservedStorageFull( RS_STORAGE_FULL_NOTIFICATION_T *notification )
{
	CString strMsg;
	strMsg = GetRecorderIP(notification->strUUID) + TEXT("의 저장 공간이 부족합니다.");
	//SendLog(strMsg, LOG_OPERATION);
	return;
/*
#ifdef LANGUAGEPACK
	strMsg=g_LanguageMapClass->Get(g_nLanguageType,L"S222")+getRecorderIP(notification->strUUID);
	CAlertMessage alertDlg(NULL, strMsg, g_LanguageMapClass->Get(g_nLanguageType,L"S223"), VMS_OK);
#else
	strMsg=getRecorderIP(notification->strUUID)+TEXT("의 저장 공간이 부족합니다.");
	CAlertMessage alertDlg(NULL, strMsg, TEXT("남은 저장 공간을 확인하여 주십시오."), VMS_OK);
#endif
	if(alertDlg.DoModal() == IDOK)
	{
		if(g_2DGroupInfo!=NULL && g_mainDlg)
		{
			g_mainDlg->m_recordSetupDlg->m_disk->LoadDiskInfo();
			g_mainDlg->m_recordSetupDlg->m_schedule->LoadScheduleInfo();
			g_mainDlg->m_recordSetupDlg->m_flag_show = TRUE;
			g_mainDlg->m_recordSetupDlg->ShowWindow(SW_SHOW);
		}
	}
*/
}

VOID HTNotificationReceiver::OnOverwritingError( RS_OVERWRITE_ERROR_NOTIFICATION_T *notification )
{
	CString strMsg;
	strMsg = GetRecorderIP(notification->strUUID) + TEXT(" 덮어쓰기에 실패하였습니다.");
	MessageBox(NULL, strMsg, TEXT("WARNING!!!"), MB_OK);

	//SendLog(strMsg, LOG_OPERATION);
	return;
/*
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
*/
}

VOID HTNotificationReceiver::OnConfigurationChanged( RS_CONFIGURATION_CHANGED_NOTIFICATION_T *notification )
{
	//SendLog(L"레코더의 환경 설정이 변경되었습니다.",LOG_OPERATION);
	return;
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

VOID HTNotificationReceiver::OnPlaybackError( RS_PLAYBACK_ERROR_NOTIFICATION_T *notification )
{
	CString strMsg;
	strMsg = GetRecorderIP(notification->strUUID) + TEXT(" 재생에 실패하였습니다.");
	//SendLog(strMsg, LOG_OPERATION);
	return;
/*
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
	*/
}

VOID HTNotificationReceiver::OnDiskError(RS_DISK_ERROR_NOTIFICATION_T * notification)
{

}

VOID HTNotificationReceiver::OnKeyFrameMode(RS_KEY_FRAME_MODE_NOTIFICATION_T * notification)
{

}

VOID HTNotificationReceiver::OnBufferClean(RS_BUFFER_CLEAN_NOTIFICATION_T * notification)
{

}
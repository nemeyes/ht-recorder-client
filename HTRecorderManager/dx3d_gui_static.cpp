#include "StdAfx.h"
#include "dx3d_gui_static.h"

using namespace DX3DUTIL;

CDX3DStatic::CDX3DStatic(CDX3DPannel* pDxPannel)
: CDX3DBaseControl(pDxPannel)
{
	m_ControlType = CONTROL_STATIC;
}

CDX3DStatic::~CDX3DStatic()
{
}

void CDX3DStatic::SetText(LPCTSTR lpText)
{
	m_strText = lpText;
}

CString	CDX3DStatic::GetText() const
{
	return m_strText;
}

void CDX3DStatic::SetToolTipText(LPCTSTR lpText)
{
	m_strToolTip = lpText;
}

CString	CDX3DStatic::GetToolTipText() const
{
	return m_strToolTip;
}

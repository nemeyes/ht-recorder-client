#pragma once
#include "dx3d_gui.h"

namespace DX3DUTIL
{
	class CDX3DStatic : public CDX3DBaseControl
	{
	public:
		CDX3DStatic(CDX3DPannel* pDxPannel);
		virtual ~CDX3DStatic();

		void	SetText(LPCTSTR lpText);
		CString	GetText() const;

		void	SetToolTipText(LPCTSTR lpText);
		CString	GetToolTipText() const;

	protected:
		CString	m_strText;
		CString m_strToolTip;
	};
};
#pragma once
#include "dx3d_gui.h"
#include "dx3d_gui_static.h"

namespace DX3DUTIL
{
	class CDX3DButton : public CDX3DStatic
	{
	public:
		enum BUTTON_STATE
		{
			BUTTON_STATE_NORMAL = 0,
			BUTTON_STATE_OVER,
			BUTTON_STATE_DOWN,
			BUTTON_STATE_DISABLE,
			BUTTON_STATE_MAX,
		};

		CDX3DButton(CDX3DPannel* pDxPannel);
		virtual ~CDX3DButton();

		virtual void	Refresh();
		virtual BOOL	HandleMouse(UINT nMsg, POINT pt, WPARAM wParam, LPARAM lParam);
		virtual BOOL	GetHasFocus() const { return TRUE; }
		virtual void	OnMouseEnter();
		virtual void	OnMouseLeave();
		void			SetEnabled(BOOL bEnabled, BOOL bRedraw = TRUE);

		void			SetButtonTextureRect(BUTTON_STATE State, const RECT& rect, BOOL bRedraw = FALSE);
		void			SetEnableCheckButton(BOOL bCheckButton);
		BOOL			GetEnableCheckButton() const;
		void			SetCheck(BOOL bCheck = TRUE);
		BOOL			GetCheck() const;
		void			SetButtonState(BUTTON_STATE State = BUTTON_STATE_NORMAL, BOOL bRedraw = FALSE);

	protected:
		BOOL	m_bPressed;
		CRect	m_rtButtonTexture[BUTTON_STATE_MAX];

		BUTTON_STATE	m_ButtonState;
		BOOL	m_bCheckButton;
		BOOL	m_bChecked;
	};
};
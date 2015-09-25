#include "StdAfx.h"
#include "dx3d_gui_button.h"

using namespace DX3DUTIL;

CDX3DButton::CDX3DButton(CDX3DPannel* pDxPannel)
: CDX3DStatic(pDxPannel)
, m_bPressed(FALSE)
, m_bCheckButton(FALSE)
, m_bChecked(FALSE)
, m_ButtonState(BUTTON_STATE_NORMAL)
{
	m_bHasFocus = TRUE;
	m_ControlType = CONTROL_BUTTON;
}

CDX3DButton::~CDX3DButton()
{
}

void CDX3DButton::Refresh()
{
	m_rtTexture = m_rtButtonTexture[m_ButtonState];

	__super::Refresh();
}

BOOL CDX3DButton::HandleMouse(UINT nMsg, POINT pt, WPARAM wParam, LPARAM lParam)
{
	if(!m_bEnabled || !m_bVisible)
		return FALSE;

	switch(nMsg)
	{
	case WM_LBUTTONDOWN:
		{
			if(ContainsPoint(pt))
			{
				if(m_bEnabled && m_bVisible)
				{
					m_bPressed = TRUE;
					SetCapture( GetDxPannel()->GetParentHwnd() );

					m_ButtonState	= BUTTON_STATE_DOWN;
					m_rtTexture = m_rtButtonTexture[m_ButtonState];
					

					GetDxPannel()->SendEvent(DXGUI_EVENT_LBUTTONDOWN, this);
					if(m_bHasFocus)
						GetDxPannel()->SetFocusControl( this );
					

					return TRUE;
				}
			}
		}
		break;

	case WM_LBUTTONDBLCLK:
		if(ContainsPoint(pt))
		{
			return TRUE;
		}
		break;

	case WM_LBUTTONUP:
	case WM_MOUSELEAVE:
		{
			if(m_bPressed)
			{
				m_bPressed = FALSE;
				ReleaseCapture();

				if(ContainsPoint(pt))
				{
					if(m_bCheckButton)
						SetCheck(!m_bChecked);
					else
					{
						m_ButtonState = BUTTON_STATE_OVER;
						m_rtTexture = m_rtButtonTexture[m_ButtonState];
					}
				}				
				else
				{
					if(m_bCheckButton && GetCheck())
						m_ButtonState = BUTTON_STATE_OVER;
					else
						m_ButtonState = BUTTON_STATE_NORMAL;

					m_rtTexture = m_rtButtonTexture[m_ButtonState];
				}

				if(ContainsPoint(pt))
					GetDxPannel()->SendEvent(DXGUI_EVENT_BUTTON_CLICKED, this);
				else
					GetDxPannel()->SendEvent(DXGUI_EVENT_LBUTTONUP, this);

				GetDxPannel()->SetFocusControl(NULL);

				return TRUE;
			}
		}
		break;
	}

	return FALSE;
}

void CDX3DButton::SetButtonTextureRect(BUTTON_STATE State, const RECT& rect, BOOL bRedraw /*= FALSE*/)
{
	m_rtButtonTexture[State] = rect;
	if(bRedraw)
		GetDxPannel()->SendEvent(DXGUI_EVENT_REDRAWCONTROL, this);
}

void CDX3DButton::OnMouseEnter()
{
	if(!m_bVisible)
		return;

	__super::OnMouseEnter();

	if(m_bEnabled)
	{
		if(m_bPressed)
			m_ButtonState = BUTTON_STATE_DOWN;
		else
			m_ButtonState = BUTTON_STATE_OVER;

		m_rtTexture = m_rtButtonTexture[m_ButtonState];		
	}

	m_pDxPannel->SendEvent(DXGUI_EVENT_MOUSEENTER, this);
}

void CDX3DButton::OnMouseLeave()
{
	if(!m_bVisible)
		return;

	__super::OnMouseLeave();

	if(m_bEnabled)
	{
		if(m_bCheckButton && m_bChecked)
			m_ButtonState = BUTTON_STATE_OVER;
		else
			m_ButtonState = BUTTON_STATE_NORMAL;
		
		m_rtTexture = m_rtButtonTexture[m_ButtonState];
	}

	m_pDxPannel->SendEvent(DXGUI_EVENT_MOUSELEAVE, this);
}

void CDX3DButton::SetEnableCheckButton(BOOL bCheckButton)
{
	m_bCheckButton = bCheckButton;
}

BOOL CDX3DButton::GetEnableCheckButton() const
{
	return m_bCheckButton;
}

void CDX3DButton::SetCheck(BOOL bCheck /*= TRUE*/)
{
	m_bChecked = bCheck;
	if(m_bCheckButton)
	{
		if(m_bChecked)
			m_ButtonState = BUTTON_STATE_OVER;
		else
			m_ButtonState = BUTTON_STATE_NORMAL;

		m_rtTexture = m_rtButtonTexture[m_ButtonState];
	}
}

BOOL CDX3DButton::GetCheck() const
{
	return m_bChecked;
}

void CDX3DButton::SetEnabled(BOOL bEnabled, BOOL bRedraw /*= TRUE*/)
{
	__super::SetEnabled(bEnabled);

	if(bEnabled)
		m_ButtonState = BUTTON_STATE_NORMAL;
	else
	        m_ButtonState = BUTTON_STATE_DISABLE;
	
	if(bRedraw)
		Refresh();	
	else
		m_rtTexture = m_rtButtonTexture[m_ButtonState];
}

void CDX3DButton::SetButtonState(BUTTON_STATE State /*= BUTTON_STATE_NORMAL*/, BOOL bRedraw /*= FALSE*/)
{
	m_ButtonState = State;
	
	if(bRedraw)
		Refresh();
	else
		m_rtTexture = m_rtButtonTexture[m_ButtonState];
}

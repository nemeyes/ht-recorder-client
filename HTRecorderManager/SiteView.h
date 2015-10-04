
#pragma once

#include "ViewTree.h"
#include "Entities.h"
#include <map>

class CSiteViewToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};

class CSiteView : public CDockablePane
{
// Construction
public:
	CSiteView();

	void AdjustLayout();
	void OnChangeVisualStyle();

// Attributes
protected:

	CViewTree m_wndSiteView;
	CImageList m_FileViewImages;
	CSiteViewToolBar m_wndToolBar;

	SITE_T ** m_sites;
	int m_sitesCount;

protected:
	void FillSiteView();
	void RemoveAllSites();

// Implementation
public:
	virtual ~CSiteView();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnProperties();
	afx_msg void OnAddSite();
	afx_msg void OnRemoveSite();
	afx_msg void OnAddRecorder();
	afx_msg void OnRemoveRecorder();
	afx_msg void OnAddCamera();
	afx_msg void OnRemoveCamera();
	afx_msg void OnStartRecording();
	afx_msg void OnStopRecording();
	afx_msg void OnPlayRelay();
	afx_msg void OnPlayPlayback();


	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	DECLARE_MESSAGE_MAP()
};


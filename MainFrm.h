#pragma once

#include "DemoDlg.h"


class CMainFrame : public CFrameWnd,
	public TabCtrl::Notify
{
	DECLARE_DYNAMIC(CMainFrame)

private: // TabCtrlNotify.
	void OnButtonCloseClicked(TabCtrl *ctrl, CRect const *rect, CPoint ptScr) override;
	void OnButtonMenuClicked(TabCtrl *ctrl, CRect const *rect, CPoint ptScr) override;

protected:   // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
		// 
	TabCtrlEx<TabCtrlStyle_VS2003_client_custom1> m_TabCtrl;
	CListCtrl m_List1,m_List2,m_List3,m_List4,m_List5;
		// 
	DemoDlg m_DemoDlg;

protected:
	DECLARE_MESSAGE_MAP()
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnUpdateTestdlg(CCmdUI *pCmdUI);
	afx_msg void OnTestdlg();
		// 
	afx_msg void OnUpdateLayoutTop(CCmdUI *pCmdUI);
	afx_msg void OnLayoutTop();
	afx_msg void OnUpdateLayoutBottom(CCmdUI *pCmdUI);
	afx_msg void OnLayoutBottom();
	afx_msg void OnUpdateBehaviorScaling(CCmdUI *pCmdUI);
	afx_msg void OnBehaviorScaling();
	afx_msg void OnUpdateBehaviorScrolling(CCmdUI *pCmdUI);
	afx_msg void OnBehaviorScrolling();
	afx_msg void OnUpdateEqualsize(CCmdUI *pCmdUI);
	afx_msg void OnEqualsize();
	afx_msg void OnUpdateTabsRemove(CCmdUI *pCmdUI);
	afx_msg void OnTabsRemove();
	afx_msg void OnUpdateShowCloseButton(CCmdUI *pCmdUI);
	afx_msg void OnShowCloseButton();
	afx_msg void OnUpdateShowMenuButton(CCmdUI *pCmdUI);
	afx_msg void OnShowMenuButton();
	afx_msg void OnUpdateShowScrollButtons(CCmdUI *pCmdUI);
	afx_msg void OnShowScrollButtons();
	afx_msg void OnUpdateShowClientEdgeBorder(CCmdUI *pCmdUI);
	afx_msg void OnShowClientEdgeBorder();
};


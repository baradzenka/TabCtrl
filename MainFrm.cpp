#include "stdafx.h"
#include "MainFrm.h"


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_TESTDLG, OnUpdateTestdlg)
	ON_COMMAND(ID_TESTDLG, OnTestdlg)
	ON_UPDATE_COMMAND_UI(ID_LAYOUT_TOP, OnUpdateLayoutTop)
	ON_COMMAND(ID_LAYOUT_TOP, OnLayoutTop)
	ON_UPDATE_COMMAND_UI(ID_LAYOUT_BOTTOM, OnUpdateLayoutBottom)
	ON_COMMAND(ID_LAYOUT_BOTTOM, OnLayoutBottom)
	ON_UPDATE_COMMAND_UI(ID_BEHAVIOR_SCALING, OnUpdateBehaviorScaling)
	ON_COMMAND(ID_BEHAVIOR_SCALING, OnBehaviorScaling)
	ON_UPDATE_COMMAND_UI(ID_BEHAVIOR_SCROLLING, OnUpdateBehaviorScrolling)
	ON_COMMAND(ID_BEHAVIOR_SCROLLING, OnBehaviorScrolling)
	ON_UPDATE_COMMAND_UI(ID_EQUALSIZE, OnUpdateEqualsize)
	ON_COMMAND(ID_EQUALSIZE, OnEqualsize)
	ON_UPDATE_COMMAND_UI(ID_TABSREMOVE, OnUpdateTabsRemove)
	ON_COMMAND(ID_TABSREMOVE, OnTabsRemove)
	ON_UPDATE_COMMAND_UI(ID_SHOWCLOSEBUTTON, OnUpdateShowCloseButton)
	ON_COMMAND(ID_SHOWCLOSEBUTTON, OnShowCloseButton)
	ON_UPDATE_COMMAND_UI(ID_SHOWMENUBUTTON, OnUpdateShowMenuButton)
	ON_COMMAND(ID_SHOWMENUBUTTON, OnShowMenuButton)
	ON_UPDATE_COMMAND_UI(ID_SHOWSCROLLBUTTONS, OnUpdateShowScrollButtons)
	ON_COMMAND(ID_SHOWSCROLLBUTTONS, OnShowScrollButtons)
	ON_UPDATE_COMMAND_UI(ID_SHOWCLIENTEDGEBORDER, OnUpdateShowClientEdgeBorder)
	ON_COMMAND(ID_SHOWCLIENTEDGEBORDER, OnShowClientEdgeBorder)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// 
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
		// 
		// 
		// 
		// Creation and initialization of TabCtrl.
	if( !m_TabCtrl.Create(this,WS_CHILD | WS_VISIBLE,CRect(0,0,0,0),AFX_IDW_PANE_FIRST) )
		return -1;
	m_TabCtrl.SetNotifyManager(this);
	m_TabCtrl.SetBehavior(TabCtrl::BehaviorScroll);
		// 
	if( !m_TabCtrl.CreateSystemImage(NULL,IDB_IMAGES_SYSTEM,true,14) ||
		!m_TabCtrl.CreateImage(NULL,IDB_IMAGES_TAB_NORMAL,IDB_IMAGES_TAB_DISABLE,true,16) )
		return -1;
		// 
		// Creation of child windows.
	if( !m_List1.Create(WS_CHILD | WS_CLIPCHILDREN | LVS_SHOWSELALWAYS | LVS_REPORT,CRect(0,0,0,0),&m_TabCtrl,2001) ||
		!m_List2.Create(WS_CHILD | WS_CLIPCHILDREN | LVS_SHOWSELALWAYS | LVS_REPORT,CRect(0,0,0,0),&m_TabCtrl,2002) ||
		!m_List3.Create(WS_CHILD | WS_CLIPCHILDREN | LVS_SHOWSELALWAYS | LVS_REPORT,CRect(0,0,0,0),&m_TabCtrl,2003) ||
		!m_List4.Create(WS_CHILD | WS_CLIPCHILDREN | LVS_SHOWSELALWAYS | LVS_REPORT,CRect(0,0,0,0),&m_TabCtrl,2004) ||
		!m_List5.Create(WS_CHILD | WS_CLIPCHILDREN | LVS_SHOWSELALWAYS | LVS_REPORT,CRect(0,0,0,0),&m_TabCtrl,2005) )
		return -1;
		// 
		// Initialization of child windows.
	m_List1.InsertColumn(0,_T("Mail"),LVCFMT_LEFT,100);
	m_List2.InsertColumn(0,_T("Calendar"),LVCFMT_LEFT,100);
	m_List3.InsertColumn(0,_T("Contacts"),LVCFMT_LEFT,100);
	m_List4.InsertColumn(0,_T("Tasks"),LVCFMT_LEFT,100);
	m_List5.InsertColumn(0,_T("Business Affairs"),LVCFMT_LEFT,100);
		// 
	m_List1.InsertItem(0,_T("Mail 1"));
	m_List1.InsertItem(1,_T("Mail 2"));
	m_List2.InsertItem(0,_T("Calendar 1"));
	m_List2.InsertItem(1,_T("Calendar 2"));
	m_List3.InsertItem(0,_T("Contact 1"));
	m_List3.InsertItem(1,_T("Contact 2"));
	m_List4.InsertItem(0,_T("Task 1"));
	m_List4.InsertItem(1,_T("Task 2"));
	m_List5.InsertItem(0,_T("Business Affair 1"));
	m_List5.InsertItem(1,_T("Business Affair 2"));
		// 
		// Attaching of child windows to the TabCtrl.
	if( !m_TabCtrl.AddTab(m_List1,_T("1.Mail"),0) ||
		!m_TabCtrl.AddTab(m_List2,_T("2.Calendar"),1) ||
		!m_TabCtrl.AddTab(m_List3,_T("3.Contacts"),2) ||
		!m_TabCtrl.AddTab(m_List4,_T("4.Tasks"),-1) ||
		!m_TabCtrl.AddTab(m_List5,_T("5.Business Affairs"),3) )
		return -1;
	m_TabCtrl.SetTabTooltipText( m_TabCtrl.GetTabHandleByIndex(1), _T("Tooltip for Calendar"));
	m_TabCtrl.SetTabTooltipText( m_TabCtrl.GetTabHandleByIndex(2), _T("Tooltip for Contacts"));
	m_TabCtrl.SetTabTooltipText( m_TabCtrl.GetTabHandleByIndex(3), _T("Tooltip for Tasks"));
		// 
	m_TabCtrl.DisableTab(m_TabCtrl.GetTabHandleByIndex(2),true);   // simply for example.
		// 
		// Load state from registry and update.
	m_TabCtrl.LoadState(AfxGetApp(),_T("TabCtrl"),_T("State"));
	m_TabCtrl.Update();
		// 
		// 
		// 
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	static UINT indicators[] =
	{	ID_SEPARATOR,           // status line indicator
		ID_INDICATOR_CAPS,
		ID_INDICATOR_NUM,
		ID_INDICATOR_SCRL,
	};
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
		// 
		// 
	OnTestdlg();
	m_DemoDlg.RedrawWindow(0,0,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		// 
		// 
	return 0;
}

void CMainFrame::OnDestroy()
{	m_TabCtrl.SaveState(AfxGetApp(),_T("TabCtrl"),_T("State"));
		// 
	CFrameWnd::OnDestroy();
}




void CMainFrame::OnUpdateTestdlg(CCmdUI *pCmdUI)
{	pCmdUI->Enable(!::IsWindow(m_DemoDlg.m_hWnd) || !::IsWindowVisible(m_DemoDlg.m_hWnd));
}
// 
void CMainFrame::OnTestdlg()
{	if(!m_DemoDlg.m_hWnd)
	{	if( !m_DemoDlg.Create(IDD_TESTDLG,this) )
			return;
		m_DemoDlg.CenterWindow(this);
	}
	m_DemoDlg.ShowWindow(SW_SHOWNORMAL);
}





// Layout - Top.
void CMainFrame::OnUpdateLayoutTop(CCmdUI *pCmdUI)
{	pCmdUI->SetRadio(m_TabCtrl.GetLayout()==TabCtrl::LayoutTop);
}
void CMainFrame::OnLayoutTop()
{	m_TabCtrl.SetLayout(TabCtrl::LayoutTop);
	m_TabCtrl.Update();
}
// 
// Layout - Bottom.
void CMainFrame::OnUpdateLayoutBottom(CCmdUI *pCmdUI)
{	pCmdUI->SetRadio(m_TabCtrl.GetLayout()==TabCtrl::LayoutBottom);
}
void CMainFrame::OnLayoutBottom()
{	m_TabCtrl.SetLayout(TabCtrl::LayoutBottom);
	m_TabCtrl.Update();
}
// 
// Behavior - Scaling.
void CMainFrame::OnUpdateBehaviorScaling(CCmdUI *pCmdUI)
{	pCmdUI->SetRadio(m_TabCtrl.GetBehavior()==TabCtrl::BehaviorScale);
}
void CMainFrame::OnBehaviorScaling()
{	m_TabCtrl.SetBehavior(TabCtrl::BehaviorScale);
	m_TabCtrl.Update();
}
// 
// Behavior - Scrolling.
void CMainFrame::OnUpdateBehaviorScrolling(CCmdUI *pCmdUI)
{	pCmdUI->SetRadio(m_TabCtrl.GetBehavior()==TabCtrl::BehaviorScroll);
}
void CMainFrame::OnBehaviorScrolling()
{	m_TabCtrl.SetBehavior(TabCtrl::BehaviorScroll);
	m_TabCtrl.Update();
}
// 
// Equal size.
void CMainFrame::OnUpdateEqualsize(CCmdUI *pCmdUI)
{	pCmdUI->SetCheck( m_TabCtrl.IsEqualTabsSize() );
}
void CMainFrame::OnEqualsize()
{	m_TabCtrl.EqualTabsSize( !m_TabCtrl.IsEqualTabsSize() );
	m_TabCtrl.Update();
}
// 
// Remove tabs.
void CMainFrame::OnUpdateTabsRemove(CCmdUI *pCmdUI)
{	pCmdUI->SetCheck( m_TabCtrl.IsTabRemoveEnable() );
}
void CMainFrame::OnTabsRemove()
{	m_TabCtrl.EnableTabRemove( !m_TabCtrl.IsTabRemoveEnable() );
	m_TabCtrl.Update();
}
// 
// Show close button.
void CMainFrame::OnUpdateShowCloseButton(CCmdUI *pCmdUI)
{	pCmdUI->SetCheck( m_TabCtrl.IsButtonCloseVisible() );
}
void CMainFrame::OnShowCloseButton()
{	m_TabCtrl.ShowButtonClose( !m_TabCtrl.IsButtonCloseVisible() );
	m_TabCtrl.Update();
}
// 
// Show menu button.
void CMainFrame::OnUpdateShowMenuButton(CCmdUI *pCmdUI)
{	pCmdUI->SetCheck( m_TabCtrl.IsButtonMenuVisible() );
}
void CMainFrame::OnShowMenuButton()
{	m_TabCtrl.ShowButtonMenu( !m_TabCtrl.IsButtonMenuVisible() );
	m_TabCtrl.Update();
}
// 
// Show scroll buttons.
void CMainFrame::OnUpdateShowScrollButtons(CCmdUI *pCmdUI)
{	pCmdUI->SetCheck( m_TabCtrl.IsButtonsScrollVisible() );
}
void CMainFrame::OnShowScrollButtons()
{	m_TabCtrl.ShowButtonsScroll( !m_TabCtrl.IsButtonsScrollVisible() );
	m_TabCtrl.Update();
}
// 
// Show ClientEdge border.
void CMainFrame::OnUpdateShowClientEdgeBorder(CCmdUI *pCmdUI)
{	const LONG styleEx = ::GetWindowLong(m_TabCtrl.m_hWnd,GWL_EXSTYLE);
	pCmdUI->SetCheck((styleEx & WS_EX_CLIENTEDGE)!=0);
}
void CMainFrame::OnShowClientEdgeBorder()
{	const LONG styleEx = ::GetWindowLong(m_TabCtrl.m_hWnd,GWL_EXSTYLE);
		// 
	if((styleEx & WS_EX_CLIENTEDGE)!=0)
	{	m_TabCtrl.ModifyStyleEx(WS_EX_CLIENTEDGE,0);
		m_TabCtrl.ShowBorder(true);
	}
	else
	{	m_TabCtrl.ModifyStyleEx(0,WS_EX_CLIENTEDGE);
		m_TabCtrl.ShowBorder(false);
	}
	m_TabCtrl.Update();
	m_TabCtrl.SetWindowPos(NULL, 0,0,0,0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);	// border update.
}




/////////////////////////////////////////////////////////////////////////////
// TabCtrlNotify.
// 
void CMainFrame::OnCloseButtonClicked(TabCtrl * /*ctrl*/, CRect const * /*rect*/, CPoint /*ptScr*/)
{	::MessageBox(m_hWnd,_T("CMainFrame::OnCloseButtonClicked"),_T("CMainFrame"),MB_OK);
}
void CMainFrame::OnMenuButtonClicked(TabCtrl * /*ctrl*/, CRect const * /*rect*/, CPoint /*ptScr*/)
{	::MessageBox(m_hWnd,_T("CMainFrame::OnMenuButtonClicked"),_T("CMainFrame"),MB_OK);
}


















BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	m_TabCtrl.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_TabCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

















/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "DemoDlg.h"
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabDialog.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(TabDialog, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// 
void TabDialog::OnBnClickedButton1()
{	::MessageBox(m_hWnd,_T("TabDialog::OnBnClickedButton1"),_T("TabDialog"),MB_OK);
}
// 
void TabDialog::OnCancel()
{	//CDialog::OnCancel();
}
// 
void TabDialog::OnOK()
{	//CDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// DemoDlg.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(DemoDlg, CDialog)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUT11, OnBnClickedBut11)
	ON_BN_CLICKED(IDC_BUT12, OnBnClickedBut12)
	ON_BN_CLICKED(IDC_BUT13, OnBnClickedBut13)
	ON_BN_CLICKED(IDC_BUT14, OnBnClickedBut14)
	ON_BN_CLICKED(IDC_BUT21, OnBnClickedBut21)
	ON_BN_CLICKED(IDC_BUT22, OnBnClickedBut22)
	ON_BN_CLICKED(IDC_BUT23, OnBnClickedBut23)
	ON_BN_CLICKED(IDC_BUT24, OnBnClickedBut24)
	ON_BN_CLICKED(IDC_BUT25, OnBnClickedBut25)
	ON_BN_CLICKED(IDC_BUT31, OnBnClickedBut31)
	ON_BN_CLICKED(IDC_BUT32, OnBnClickedBut32)
	ON_BN_CLICKED(IDC_BUT33, OnBnClickedBut33)
	ON_BN_CLICKED(IDC_BUT41, OnBnClickedBut41)
	ON_BN_CLICKED(IDC_BUT42, OnBnClickedBut42)
	ON_BN_CLICKED(IDC_BUT51, OnBnClickedBut51)
	ON_BN_CLICKED(IDC_BUT52, OnBnClickedBut52)
	ON_BN_CLICKED(IDC_BUT53, OnBnClickedBut53)
	ON_BN_CLICKED(IDC_BUT61, OnBnClickedBut61)
	ON_BN_CLICKED(IDC_BUT62, OnBnClickedBut62)
	ON_BN_CLICKED(IDC_BUT63, OnBnClickedBut63)
	ON_BN_CLICKED(IDC_BUT64, OnBnClickedBut64)
	ON_BN_CLICKED(IDC_BUT65, OnBnClickedBut65)
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// 
DemoDlg::DemoDlg()
{	m_bDelDragOutside = false;
}
// 
int DemoDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{	if(CDialog::OnCreate(lpCreateStruct)==-1)
		return -1;
	ModifyStyle(0,WS_CLIPCHILDREN);   // to avoid flicks of dialog child controls.
		// 
	if( !m_TabCtrl.Create(this,WS_CHILD | WS_VISIBLE,CRect(0,0,0,0),3001) )
		return -1;
		// 
		// 
	if( !m_List1.Create(WS_CLIPCHILDREN | LVS_REPORT,CRect(0,0,0,0),&m_TabCtrl,3002) ||
		!m_List2.Create(WS_CLIPCHILDREN | LVS_REPORT,CRect(0,0,0,0),&m_TabCtrl,3003) ||
		!m_List3.Create(WS_CLIPCHILDREN | LVS_REPORT,CRect(0,0,0,0),&m_TabCtrl,3004) )
		return -1;
		// 
	if( !m_Tree1.Create(WS_CHILD | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES,CRect(0,0,0,0),&m_TabCtrl,3005) ||
		!m_Tree2.Create(WS_CHILD | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES,CRect(0,0,0,0),&m_TabCtrl,3006) ||
		!m_Tree3.Create(WS_CHILD | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES,CRect(0,0,0,0),&m_TabCtrl,3007) )
		return -1;
		// 
	if( !m_Edit1.Create(ES_MULTILINE | WS_CHILD,CRect(0,0,0,0),&m_TabCtrl,3008) ||
		!m_Edit2.Create(ES_MULTILINE | WS_CHILD,CRect(0,0,0,0),&m_TabCtrl,3009) )
		return -1;
		// 
	if( !m_Dlg1.Create(IDD_DIALOG,&m_TabCtrl) )   // create as modeless dialog box.
		return -1;
	m_Dlg1.SetDlgCtrlID(3010);   // set unique id - important for dialog box.
		// 
	return 0;
}
/////////////////////////////////////////////////////////////////////////////
// 
BOOL DemoDlg::OnInitDialog()
{	CDialog::OnInitDialog();
		// 
	GetDlgItem(IDC_TABCTRL_BASE)->ShowWindow(SW_HIDE);
	SetTabCtrlPos();
		// 
		// 
	for(int i=0; i<6; ++i)
		OnBnClickedBut63();   // add tabs.
		// 
	m_List1.InsertColumn(0,_T("Calendar"),LVCFMT_LEFT,100);
	m_List2.InsertColumn(0,_T("Business Affairs"),LVCFMT_LEFT,100);
	m_List3.InsertColumn(0,_T("Folder List"),LVCFMT_LEFT,100);
		// 
	m_Tree1.InsertItem(_T("Mail"));
	m_Tree2.InsertItem(_T("Tasks"));
	m_Tree3.InsertItem(_T("Shortcuts"));
		// 
	m_Edit1.SetWindowText(_T("Contacts"));
	m_Edit2.SetWindowText(_T("Notes"));
		// 
		// 
	m_TabCtrl.CreateSystemImages(nullptr,IDB_IMAGES_SYSTEM,true,14);
	m_TabCtrl.CreateImages(nullptr,IDB_IMAGES_TAB_NORMAL,IDB_IMAGES_TAB_DISABLE,true,16);
		// 
	CFont font;
	font.CreatePointFont(85,_T("Tahoma"));
	m_TabCtrl.SetFontNormal(&font);
	m_TabCtrl.SetFontSelect(&font);
		// 
	m_TabCtrl.SetCursor(IDC_CURSOR1);
	m_TabCtrl.EnableTabRemove(true);
		// 
	m_TabCtrl.SetNotifyManager(this);
		// 
	m_TabCtrl.styleBase.Install(&m_TabCtrl);   // install style.
	m_TabCtrl.Update();
		// 
		// 
		// 
	CComboBox *pCombo = static_cast<CComboBox*>( GetDlgItem(IDC_COMBO1) );
	pCombo->AddString(_T("1. Base style"));
	pCombo->AddString(_T("2. Like client area VS2003"));
	pCombo->AddString(_T("3. Kind of 2"));
	pCombo->AddString(_T("4. Like bars VS2003"));
	pCombo->AddString(_T("5. Kind of 4"));
	pCombo->AddString(_T("6. Like client area VS2008 classic"));
	pCombo->AddString(_T("7. Like client area VS2008 blue"));
	pCombo->AddString(_T("8. Like client area VS2008 silver"));
	pCombo->AddString(_T("9. Like client area VS2008 olive"));
	pCombo->AddString(_T("10. Like bars VS2008 classic"));
	pCombo->AddString(_T("11. Kind of 10"));
	pCombo->AddString(_T("12. Like bars VS2008 blue"));
	pCombo->AddString(_T("13. Kind of 12"));
	pCombo->AddString(_T("14. Like bars VS2008 silver"));
	pCombo->AddString(_T("15. Kind of 14"));
	pCombo->AddString(_T("16. Like bars VS2008 olive"));
	pCombo->AddString(_T("17. Kind of 16"));
	pCombo->AddString(_T("18. Like client area VS2010"));
	pCombo->AddString(_T("19. Kind of 18"));
	pCombo->AddString(_T("20. Kind of 18"));
	pCombo->AddString(_T("21. Like bars VS2010"));
	pCombo->AddString(_T("22. Like client area VS2019 light"));
	pCombo->AddString(_T("23. Like client area VS2019 dark"));
	pCombo->AddString(_T("24. Like client area VS2019 blue"));
	pCombo->AddString(_T("25. Like bars VS2019 light"));
	pCombo->AddString(_T("26. Like bars VS2019 dark"));
	pCombo->AddString(_T("27. Like bars VS2019 blue"));
	pCombo->SetCurSel(0);
		// 
	SetButtonCheck(IDC_BUT11, m_TabCtrl.GetLayout()==TabCtrl::LayoutTop );
	SetButtonCheck(IDC_BUT12, m_TabCtrl.GetLayout()==TabCtrl::LayoutBottom );
	SetButtonCheck(IDC_BUT13, m_TabCtrl.GetBehavior()==TabCtrl::BehaviorScale );
	SetButtonCheck(IDC_BUT14, m_TabCtrl.GetBehavior()==TabCtrl::BehaviorScroll );
	SetButtonCheck(IDC_BUT21, m_TabCtrl.GetCursor()!=nullptr );
	SetButtonCheck(IDC_BUT22, m_TabCtrl.IsEqualTabsSize() );
	SetButtonCheck(IDC_BUT23, m_TabCtrl.IsHideSingleTab() );
	SetButtonCheck(IDC_BUT24, m_TabCtrl.IsTabRemoveEnable() );
	EnableControl(IDC_BUT25, m_TabCtrl.IsTabRemoveEnable() );
	SetButtonCheck(IDC_BUT25,m_bDelDragOutside);
	SetButtonCheck(IDC_BUT31, m_TabCtrl.IsBorderVisible() );
		// 
	Gdiplus::Bitmap *imageNormal, *imageDisable;
	m_TabCtrl.GetImages(&imageNormal/*out*/,&imageDisable/*out*/);
	SetButtonCheck(IDC_BUT33,imageNormal || imageDisable);
		// 
	SetButtonCheck(IDC_BUT41, m_TabCtrl.IsWatchCtrlActivity() );
		// 
	LOGFONT logfont;
	m_TabCtrl.GetFontSelect()->GetLogFont(&logfont);
	SetButtonCheck(IDC_BUT42, (logfont.lfWeight==FW_BOLD) );
		// 
	SetButtonCheck(IDC_BUT51, m_TabCtrl.IsButtonCloseVisible() );
	SetButtonCheck(IDC_BUT52, m_TabCtrl.IsButtonMenuVisible() );
	SetButtonCheck(IDC_BUT53, m_TabCtrl.IsButtonsScrollVisible() );
		// 
		// 
	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// 
void DemoDlg::OnSize(UINT nType, int cx, int cy)
{	CDialog::OnSize(nType, cx, cy);
		// 
	SetTabCtrlPos();
}
/////////////////////////////////////////////////////////////////////////////
// Style.
void DemoDlg::OnCbnSelchangeCombo1()
{	CComboBox *pCombo = static_cast<CComboBox*>( GetDlgItem(IDC_COMBO1) );
		// 
	switch( pCombo->GetCurSel() )
	{	case 0: m_TabCtrl.styleBase.Install(&m_TabCtrl); break;
		case 1: m_TabCtrl.styleVS2003_client.Install(&m_TabCtrl); break;
		case 2: m_TabCtrl.styleVS2003_client_custom1.Install(&m_TabCtrl); break;
		case 3: m_TabCtrl.styleVS2003_bars.Install(&m_TabCtrl); break;
		case 4: m_TabCtrl.styleVS2003_bars_custom1.Install(&m_TabCtrl); break;
		case 5: m_TabCtrl.styleVS2008_client_classic.Install(&m_TabCtrl); break;
		case 6: m_TabCtrl.styleVS2008_client_blue.Install(&m_TabCtrl); break;
		case 7: m_TabCtrl.styleVS2008_client_silver.Install(&m_TabCtrl); break;
		case 8: m_TabCtrl.styleVS2008_client_olive.Install(&m_TabCtrl); break;
		case 9: m_TabCtrl.styleVS2008_bars_classic.Install(&m_TabCtrl); break;
		case 10: m_TabCtrl.styleVS2008_bars_classic_custom1.Install(&m_TabCtrl); break;
		case 11: m_TabCtrl.styleVS2008_bars_blue.Install(&m_TabCtrl); break;
		case 12: m_TabCtrl.styleVS2008_bars_blue_custom1.Install(&m_TabCtrl); break;
		case 13: m_TabCtrl.styleVS2008_bars_silver.Install(&m_TabCtrl); break;
		case 14: m_TabCtrl.styleVS2008_bars_silver_custom1.Install(&m_TabCtrl); break;
		case 15: m_TabCtrl.styleVS2008_bars_olive.Install(&m_TabCtrl); break;
		case 16: m_TabCtrl.styleVS2008_bars_olive_custom1.Install(&m_TabCtrl); break;
		case 17: m_TabCtrl.styleVS2010_client.Install(&m_TabCtrl); break;
		case 18: m_TabCtrl.styleVS2010_client_custom1.Install(&m_TabCtrl); break;
		case 19: m_TabCtrl.styleVS2010_client_custom2.Install(&m_TabCtrl); break;
		case 20: m_TabCtrl.styleVS2010_bars.Install(&m_TabCtrl); break;
		case 21: m_TabCtrl.styleVS2019_client_light.Install(&m_TabCtrl); break;
		case 22: m_TabCtrl.styleVS2019_client_dark.Install(&m_TabCtrl); break;
		case 23: m_TabCtrl.styleVS2019_client_blue.Install(&m_TabCtrl); break;
		case 24: m_TabCtrl.styleVS2019_bars_light.Install(&m_TabCtrl); break;
		case 25: m_TabCtrl.styleVS2019_bars_dark.Install(&m_TabCtrl); break;
		case 26: m_TabCtrl.styleVS2019_bars_blue.Install(&m_TabCtrl); break;
	}
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Layout - top.
void DemoDlg::OnBnClickedBut11()
{	m_TabCtrl.SetLayout(TabCtrl::LayoutTop);
	m_TabCtrl.Update();
}
// 
// Layout - bottom.
void DemoDlg::OnBnClickedBut12()
{	m_TabCtrl.SetLayout(TabCtrl::LayoutBottom);
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Behavior - scaling.
void DemoDlg::OnBnClickedBut13()
{	m_TabCtrl.SetBehavior(TabCtrl::BehaviorScale);
	m_TabCtrl.Update();
}
// 
// Behavior - scrolling.
void DemoDlg::OnBnClickedBut14()
{	m_TabCtrl.SetBehavior(TabCtrl::BehaviorScroll);
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Custom cursor.
void DemoDlg::OnBnClickedBut21()
{	(GetButtonCheck(IDC_BUT21) ? m_TabCtrl.SetCursor(IDC_CURSOR1) : m_TabCtrl.SetCursor(0u));
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Equal size of tabs.
void DemoDlg::OnBnClickedBut22()
{	m_TabCtrl.EqualTabsSize( GetButtonCheck(IDC_BUT22) );
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Hide single tab.
void DemoDlg::OnBnClickedBut23()
{	m_TabCtrl.HideSingleTab( GetButtonCheck(IDC_BUT23) );
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Remove tabs.
void DemoDlg::OnBnClickedBut24()
{	m_TabCtrl.EnableTabRemove( GetButtonCheck(IDC_BUT24) );
	EnableControl(IDC_BUT25, GetButtonCheck(IDC_BUT24) );
}
// 
// Delete tab for drag outside.
void DemoDlg::OnBnClickedBut25()
{	m_bDelDragOutside = GetButtonCheck(IDC_BUT25);
}
/////////////////////////////////////////////////////////////////////////////
// Show border.
void DemoDlg::OnBnClickedBut31()
{	m_TabCtrl.ShowBorder( GetButtonCheck(IDC_BUT31) );
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Show controls borders.
void DemoDlg::OnBnClickedBut32()
{	if(GetButtonCheck(IDC_BUT32))
	{	m_List1.ModifyStyle(0,WS_BORDER);
		m_List2.ModifyStyle(0,WS_BORDER);
		m_List3.ModifyStyle(0,WS_BORDER);
		m_Tree1.ModifyStyle(0,WS_BORDER);
		m_Tree2.ModifyStyle(0,WS_BORDER);
		m_Tree3.ModifyStyle(0,WS_BORDER);
		m_Edit1.ModifyStyle(0,WS_BORDER);
		m_Edit2.ModifyStyle(0,WS_BORDER);
		m_Dlg1.ModifyStyle(0,WS_BORDER);
	}
	else
	{	m_List1.ModifyStyle(WS_BORDER,0);
		m_List2.ModifyStyle(WS_BORDER,0);
		m_List3.ModifyStyle(WS_BORDER,0);
		m_Tree1.ModifyStyle(WS_BORDER,0);
		m_Tree2.ModifyStyle(WS_BORDER,0);
		m_Tree3.ModifyStyle(WS_BORDER,0);
		m_Edit1.ModifyStyle(WS_BORDER,0);
		m_Edit2.ModifyStyle(WS_BORDER,0);
		m_Dlg1.ModifyStyle(WS_BORDER,0);
	}
		// 
	m_List1.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	m_List2.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	m_List3.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	m_Tree1.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	m_Tree2.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	m_Tree3.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	m_Edit1.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	m_Edit2.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	m_Dlg1.SetWindowPos(nullptr,0,0,0,0,SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}
/////////////////////////////////////////////////////////////////////////////
// Show images.
void DemoDlg::OnBnClickedBut33()
{	(GetButtonCheck(IDC_BUT33) ?
		m_TabCtrl.CreateImages(nullptr,IDB_IMAGES_TAB_NORMAL,IDB_IMAGES_TAB_DISABLE,true,16) :
		m_TabCtrl.CreateImages(nullptr, 0,0, false,0, 0));
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Watch activity control.
void DemoDlg::OnBnClickedBut41()
{	m_TabCtrl.WatchCtrlActivity( GetButtonCheck(IDC_BUT41) );
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Bold font for selected tab.
void DemoDlg::OnBnClickedBut42()
{	if( GetButtonCheck(IDC_BUT42) )
	{	LOGFONT logfont;
		m_TabCtrl.GetFontNormal()->GetLogFont(&logfont/*out*/);
		logfont.lfWeight = FW_BOLD;
		m_TabCtrl.SetFontSelect(&logfont);
	}
	else
	{	CFont *font = m_TabCtrl.GetFontNormal();
		m_TabCtrl.SetFontSelect(font);
	}
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Show close button.
void DemoDlg::OnBnClickedBut51()
{	m_TabCtrl.ShowButtonClose( GetButtonCheck(IDC_BUT51) );
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Show menu button.
void DemoDlg::OnBnClickedBut52()
{	m_TabCtrl.ShowButtonMenu( GetButtonCheck(IDC_BUT52) );
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Show scroll buttons.
void DemoDlg::OnBnClickedBut53()
{	m_TabCtrl.ShowButtonsScroll( GetButtonCheck(IDC_BUT53) );
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Enable / Disable.
void DemoDlg::OnBnClickedBut61()
{	TabCtrl::HTAB tab = m_TabCtrl.GetSelectedTab();
	if(!tab && m_TabCtrl.GetNumberTabs()>0)
		tab = m_TabCtrl.GetTabHandleByIndex(0);
	if(tab)
	{	m_TabCtrl.DisableTab(tab,!m_TabCtrl.IsTabDisabled(tab));
		m_TabCtrl.Update();
	}
}
// 
// Enable All.
void DemoDlg::OnBnClickedBut62()
{	for(int i=0, c=m_TabCtrl.GetNumberTabs(); i<c; ++i)
		m_TabCtrl.DisableTab( m_TabCtrl.GetTabHandleByIndex(i), false);
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
// Add.
void DemoDlg::OnBnClickedBut63()
{	if( !IsExist(&m_Tree1) )
		m_TabCtrl.AddTab(m_Tree1,_T("Mail"),0);
	else if( !IsExist(&m_List1) )
		m_TabCtrl.AddTab(m_List1,_T("Calendar"),1);
	else if( !IsExist(&m_Dlg1) )
		m_TabCtrl.AddTab(m_Dlg1,_T("Modeless Dialog Box"),-1);
	else if( !IsExist(&m_Edit1) )
		m_TabCtrl.AddTab(m_Edit1,_T("Contacts"),2);
	else if( !IsExist(&m_Tree2) )
		m_TabCtrl.AddTab(m_Tree2,_T("Tasks"),3);
	else if( !IsExist(&m_List2) )
		m_TabCtrl.AddTab(m_List2,_T("Business Affairs"),4);
	else if( !IsExist(&m_Edit2) )
		m_TabCtrl.AddTab(m_Edit2,_T("Notes"),5);
	else if( !IsExist(&m_List3) )
		m_TabCtrl.AddTab(m_List3,_T("Folder List"),-1);
	else if( !IsExist(&m_Tree3) )
		m_TabCtrl.AddTab(m_Tree3,_T("Shortcuts"),6);
	m_TabCtrl.Update();
}
// 
// Delete.
void DemoDlg::OnBnClickedBut64()
{	TabCtrl::HTAB tab = m_TabCtrl.GetSelectedTab();
	if(tab)
	{	m_TabCtrl.DeleteTab(tab);
		m_TabCtrl.Update();
	}
}
// 
// Delete All.
void DemoDlg::OnBnClickedBut65()
{	m_TabCtrl.DeleteAllTabs();
	m_TabCtrl.Update();
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void DemoDlg::SetTabCtrlPos()
{	CWnd *baseWnd = GetDlgItem(IDC_TABCTRL_BASE);
		// 
	if(baseWnd)
	{	CRect rcBase;
		baseWnd->GetWindowRect(&rcBase/*out*/);
		ScreenToClient(&rcBase);
			// 
		CRect rc;
		GetClientRect(&rc/*out*/);
		rc.DeflateRect(rcBase.left,rcBase.top,rcBase.top,rcBase.top);
		m_TabCtrl.MoveWindow(&rc);
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
bool DemoDlg::IsExist(CWnd *pWnd) const
{	for(int i=0, c=m_TabCtrl.GetNumberTabs(); i<c; ++i)
		if(m_TabCtrl.GetTabWindow( m_TabCtrl.GetTabHandleByIndex(i) )==pWnd->m_hWnd)
			return true;
	return false;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void DemoDlg::SetButtonCheck(int id, bool check) const 
{	reinterpret_cast<CButton*>( GetDlgItem(id) )->SetCheck(check ? BST_CHECKED : BST_UNCHECKED);
}
bool DemoDlg::GetButtonCheck(int id) const
{	return reinterpret_cast<CButton*>( GetDlgItem(id) )->GetCheck() == BST_CHECKED;
}
// 
void DemoDlg::EnableControl(int id, bool enable) const
{	CWnd *wnd = GetDlgItem(id);
	enable ? wnd->ModifyStyle(WS_DISABLED,0) : wnd->ModifyStyle(0,WS_DISABLED);
	wnd->Invalidate();
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlNotify.
/////////////////////////////////////////////////////////////////////////////
// 
void DemoDlg::OnButtonCloseClicked(TabCtrl *ctrl, CRect const * /*rect*/, CPoint /*ptScr*/)
{	TabCtrl::HTAB tab = ctrl->GetSelectedTab();   // get handle of current active tab (whose child window is visible).
	if(tab)
	{	ctrl->DeleteTab(tab);
		ctrl->Update();
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void DemoDlg::OnButtonMenuClicked(TabCtrl *ctrl, CRect const *rect, CPoint /*ptScr*/)
{	CMenu menu;
	if( menu.CreatePopupMenu() )
	{	const int number = ctrl->GetNumberTabs();
		for(int i=0; i<number; ++i)
		{	TabCtrl::HTAB tab = ctrl->GetTabHandleByIndex(i);
			const CString text = ctrl->GetTabText(tab);
				// 
			MENUITEMINFO info;
			info.cbSize = sizeof(info);
			info.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
			info.wID = i+1;
			info.fState = (!ctrl->IsTabDisabled(tab) ? MFS_ENABLED : MFS_DISABLED);
			info.fType = MFT_STRING;
			if(tab==ctrl->GetSelectedTab())
			{	info.fState |= MFS_CHECKED;
				info.fType |= MFT_RADIOCHECK;
				info.hbmpChecked = nullptr;
			}
			info.dwTypeData = const_cast<TCHAR *>( text.GetString() );
			info.cch = text.GetLength();
			::InsertMenuItem(menu,i,TRUE,&info);
		}
			// 
		CRect rc(rect);
		ctrl->ClientToScreen(&rc);
		const int id = static_cast<int>( ::TrackPopupMenu(menu,TPM_RIGHTALIGN | TPM_RETURNCMD,rc.right,rc.bottom,0,m_hWnd,nullptr) );
		if(id!=0)
		{	TabCtrl::HTAB selTab = ctrl->GetTabHandleByIndex(id-1);
			ctrl->SelectTab(selTab);
			ctrl->EnsureTabVisible(selTab);
			ctrl->Update();
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void DemoDlg::OnDrag(TabCtrl *ctrl, TabCtrl::HTAB tab, CPoint /*ptScr*/, bool outside)
{	if(outside && m_bDelDragOutside)
	{	ctrl->DeleteTab(tab);
		ctrl->Update();
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////













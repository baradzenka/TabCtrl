/////////////////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////////////////
#include "TabCtrl/TabCtrl.h"
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
class TabDialog : public CDialog
{	DECLARE_MESSAGE_MAP()
	void OnCancel() override;
	void OnOK() override;
	afx_msg void OnBnClickedButton1();
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
struct TabCtrlComplex : TabCtrl
{	TabCtrlStyle_base styleBase;
		// 
	TabCtrlStyle_VS2003_client styleVS2003_client;
	TabCtrlStyle_VS2003_client_custom1 styleVS2003_client_custom1;
	TabCtrlStyle_VS2003_bars styleVS2003_bars;
	TabCtrlStyle_VS2003_bars_custom1 styleVS2003_bars_custom1;
		// 
	TabCtrlStyle_VS2008_client_classic styleVS2008_client_classic;
	TabCtrlStyle_VS2008_client_blue styleVS2008_client_blue;
	TabCtrlStyle_VS2008_client_silver styleVS2008_client_silver;
	TabCtrlStyle_VS2008_client_olive styleVS2008_client_olive;
	TabCtrlStyle_VS2008_bars_classic styleVS2008_bars_classic;
	TabCtrlStyle_VS2008_bars_classic_custom1 styleVS2008_bars_classic_custom1;
	TabCtrlStyle_VS2008_bars_blue styleVS2008_bars_blue;
	TabCtrlStyle_VS2008_bars_blue_custom1 styleVS2008_bars_blue_custom1;
	TabCtrlStyle_VS2008_bars_silver styleVS2008_bars_silver;
	TabCtrlStyle_VS2008_bars_silver_custom1 styleVS2008_bars_silver_custom1;
	TabCtrlStyle_VS2008_bars_olive styleVS2008_bars_olive;
	TabCtrlStyle_VS2008_bars_olive_custom1 styleVS2008_bars_olive_custom1;
		// 
	TabCtrlStyle_VS2010_client styleVS2010_client;
	TabCtrlStyle_VS2010_client_custom1 styleVS2010_client_custom1;
	TabCtrlStyle_VS2010_client_custom2 styleVS2010_client_custom2;
	TabCtrlStyle_VS2010_bars styleVS2010_bars;
		// 
	TabCtrlStyle_VS2019_client_light styleVS2019_client_light;
	TabCtrlStyle_VS2019_client_dark styleVS2019_client_dark;
	TabCtrlStyle_VS2019_client_blue styleVS2019_client_blue;
	TabCtrlStyle_VS2019_bars_light styleVS2019_bars_light;
	TabCtrlStyle_VS2019_bars_dark styleVS2019_bars_dark;
	TabCtrlStyle_VS2019_bars_blue styleVS2019_bars_blue;
};
/////////////////////////////////////////////////////////////////////////////
// 
class DemoDlg : public CDialog, 
	public TabCtrl::Notify
{
public:
	DemoDlg();

private: // TabCtrlNotify.
	void OnCloseButtonClicked(TabCtrl *ctrl, CRect const *rect, CPoint ptScr) override;
	void OnMenuButtonClicked(TabCtrl *ctrl, CRect const *rect, CPoint ptScr) override;
	void OnDrag(TabCtrl *ctrl, TabCtrl::HTAB tab, CPoint ptScr, bool outside) override;

private:
	TabCtrlComplex m_TabCtrl;
		// 
	CListCtrl m_List1, m_List2, m_List3;
	CTreeCtrl m_Tree1, m_Tree2, m_Tree3;
	CEdit m_Edit1, m_Edit2;
	TabDialog m_Dlg1;
		// 
	bool m_bDelDragOutside;
	CRect m_rcInit;

private:
	void SetTabCtrlPos();
	bool IsExist(CWnd *pWnd) const;
	void SetButtonCheck(int id, bool check) const;
	bool GetButtonCheck(int id) const;
	void EnableControl(int id, bool enable) const;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	BOOL OnInitDialog() override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedBut11();
	afx_msg void OnBnClickedBut12();
	afx_msg void OnBnClickedBut13();
	afx_msg void OnBnClickedBut14();
	afx_msg void OnBnClickedBut21();
	afx_msg void OnBnClickedBut22();
	afx_msg void OnBnClickedBut23();
	afx_msg void OnBnClickedBut24();
	afx_msg void OnBnClickedBut25();
	afx_msg void OnBnClickedBut31();
	afx_msg void OnBnClickedBut32();
	afx_msg void OnBnClickedBut33();
	afx_msg void OnBnClickedBut41();
	afx_msg void OnBnClickedBut42();
	afx_msg void OnBnClickedBut51();
	afx_msg void OnBnClickedBut52();
	afx_msg void OnBnClickedBut53();
	afx_msg void OnBnClickedBut61();
	afx_msg void OnBnClickedBut62();
	afx_msg void OnBnClickedBut63();
	afx_msg void OnBnClickedBut64();
	afx_msg void OnBnClickedBut65();
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////









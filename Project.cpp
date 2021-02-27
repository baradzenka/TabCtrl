#include "stdafx.h"
#include "MainFrm.h"


// 
class CApp : public CWinApp
{	BOOL InitInstance() override;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnAppAbout();
} theApp;


BEGIN_MESSAGE_MAP(CApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CApp initialization
BOOL CApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();


	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.

	CMainFrame *pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	// The one and only window has been initialized, so show and update it.
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();

	return TRUE;
}


// 
void CApp::OnAppAbout()
{	CDialog(IDD_ABOUTBOX).DoModal();
}



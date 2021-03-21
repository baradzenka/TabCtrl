//==========================================================
// Author: Baradzenka Aleh (baradzenka@gmail.com)
//==========================================================
// 
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "TabCtrl.h"
#include <cassert>
#include <vector>
#include <map>
/////////////////////////////////////////////////////////////////////////////
#pragma comment (lib, "Gdiplus.lib")
/////////////////////////////////////////////////////////////////////////////
#pragma warning(disable : 4355)   // 'this' : used in base member initializer list.
#undef max
#undef min
#undef new
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrl.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
struct TabCtrl::Private :
	IRecalc,
	IBehavior,
	Ability
{
	struct VirtualWindow : CDC
	{	VirtualWindow(CWnd *wnd)
		{	assert(wnd && ::IsWindow(wnd->m_hWnd));
			pwnd = wnd;
			pdc = pwnd->BeginPaint(&ps/*out*/);
			pwnd->GetClientRect(&rect/*out*/);
			if(CreateCompatibleDC(pdc) && bitmap.CreateCompatibleBitmap(pdc,rect.Width(),rect.Height()))
			{	SelectObject(&bitmap);
				SetBkMode(TRANSPARENT);
			}
		}
		~VirtualWindow()
		{	if(bitmap.m_hObject)
				pdc->BitBlt(0,0,rect.Width(),rect.Height(), this, 0,0, SRCCOPY);
			pwnd->EndPaint(&ps);
		}

	private:
		CWnd *pwnd;
		PAINTSTRUCT ps;
		CDC *pdc;
		CRect rect;
		CBitmap bitmap;
	};
		// 
	template<typename T> struct KeyboardHook
	{	void Add(T *obj, bool(T::*func)(UINT,UINT))
		{	Data *data = Lock();
			data->clients[obj] = func;
			Unlock(data);
		}
			// 
		void Delete(T *obj)
		{	Data *data = Lock();
			data->clients.erase(obj);
			Unlock(data);
		}

	private:
		struct Data : CRITICAL_SECTION
		{	Data()
			{	::InitializeCriticalSection(this);
				hook = ::SetWindowsHookEx(WH_KEYBOARD,static_cast<HOOKPROC>(HookProc),nullptr,::GetCurrentThreadId());
			}
			~Data()
			{	if(hook)
					::UnhookWindowsHookEx(hook);
				::DeleteCriticalSection(this);
			}
				// 
			HHOOK hook;
			std::map<T *,bool(T::*)(UINT,UINT)> clients;
		};
		static Data *GetData() { static Data data; return &data; }
			// 
		static Data *Lock() { Data *data=GetData(); ::EnterCriticalSection(data); return data; }
		static void Unlock(Data *data) { ::LeaveCriticalSection(data); }

	private:
		static LRESULT __stdcall HookProc(int code, WPARAM wParam, LPARAM lParam)
		{	Data *data = GetData();
				// 
			if(code==HC_ACTION &&
				!(lParam & 0x80000000))   // key is down.
			{
				Lock();
					// 
				typename std::map<T *,bool(T::*)(UINT,UINT)>::const_iterator i, n;
				for(i=data->clients.begin(); i!=data->clients.end(); )
				{	n = i++;
					if( !(n->first->*n->second)(static_cast<UINT>(wParam),static_cast<UINT>(lParam)) )
					{	Unlock(data);
						return 1;   // to prevent calling target window procedure (any nonzero value acceptable).
					}
				}
					// 
				Unlock(data);
			}
			return ::CallNextHookEx(data->hook,code,wParam,lParam);
		}
	};
		// 
	template<typename T> struct ActivityHook
	{	typedef std::pair<T *, void(T::*)(bool,HWND)> target_t;
			// 
		void Add(HWND wnd, T *obj, void(T::*func)(bool,HWND))
		{	Data *data = Lock();
			data->clients[wnd] = target_t(obj,func);
			Unlock(data);
		}
			// 
		void Delete(HWND wnd)
		{	Data *data = Lock();
			data->clients.erase(wnd);
			Unlock(data);
		}

	private:
		struct Data : CRITICAL_SECTION
		{	Data()
			{	::InitializeCriticalSection(this);
				hook = ::SetWindowsHookEx(WH_CALLWNDPROC,static_cast<HOOKPROC>(HookProc),nullptr,::GetCurrentThreadId());
			}
			~Data()
			{	if(hook)
					::UnhookWindowsHookEx(hook);
				::DeleteCriticalSection(this);
			}
				// 
			HHOOK hook;
			std::map<HWND,target_t> clients;
		};
		static Data *GetData() { static Data data; return &data; }
			// 
		static Data *Lock() { Data *data=GetData(); ::EnterCriticalSection(data); return data; }
		static void Unlock(Data *data) { ::LeaveCriticalSection(data); }

	private:
		static LRESULT __stdcall HookProc(int code, WPARAM wParam, LPARAM lParam)
		{	Data *data = GetData();
				// 
			if(code==HC_ACTION)
			{	CWPSTRUCT const *info = reinterpret_cast<CWPSTRUCT const *>(lParam);
					// 
				if(info->message==WM_SETFOCUS || info->message==WM_KILLFOCUS)
				{	Lock();
					CallClient(data,info->hwnd,info->message==WM_SETFOCUS);
					Unlock(data);
				}
			}
			return ::CallNextHookEx(data->hook,code,wParam,lParam);
		}
			// 
		static void CallClient(Data *data, HWND hwnd, bool value)
		{	typename std::map<HWND,target_t>::const_iterator i,n,e;
			for(i=data->clients.begin(), e=data->clients.end(); i!=e; )
			{	n = i++;
					// 
				for(HWND h=hwnd; h; h=::GetParent(h))
					if(h==n->first)
					{	(n->second.first->*n->second.second)(value,hwnd);
						break;
					}
			}
		}
	};

public:
	Private(TabCtrl &owner);
	~Private();

private:
	TabCtrl &o;

private:   // TabCtrl::IRecalc.
	int GetBorderWidth(TabCtrl const *ctrl, IRecalc *base) override;
		// 
	CRect GetControlAreaPadding(TabCtrl const *ctrl, IRecalc *base) override;
	CRect GetWindowsAreaPadding(TabCtrl const *ctrl, IRecalc *base) override;
		// 
	CRect GetTabHorzMargin(TabCtrl const *ctrl, IRecalc *base) override;
	CRect GetTabPadding(TabCtrl const *ctrl, IRecalc *base) override;
	int GetTabImageTextSpace(TabCtrl const *ctrl, IRecalc *base) override;
	int GetTabExtraWidth(TabCtrl const *ctrl, IRecalc *base, HTAB tab) override;
	int GetTabMinWidth(TabCtrl const *ctrl, IRecalc *base) override;
		// 
	CRect GetButtonCloseHorzMargin(TabCtrl const *ctrl, IRecalc *base) override;
	CRect GetButtonMenuHorzMargin(TabCtrl const *ctrl, IRecalc *base) override;
	CRect GetButtonScrollLeftHorzMargin(TabCtrl const *ctrl, IRecalc *base) override;
	CRect GetButtonScrollRightHorzMargin(TabCtrl const *ctrl, IRecalc *base) override;

private:   // TabCtrl::IBehavior.
	HTAB HitTest(TabCtrl const *ctrl, IBehavior *base, CPoint point) override;
	bool SetCursor(TabCtrl const *ctrl, IBehavior *base) override;

private:
	ULONG_PTR m_gdiPlusToken;

public:
	Draw *m_pDrawManager;
	IRecalc *m_pRecalcManager;
	IBehavior *m_pBehaviorManager;
	ToolTip *m_pToolTipManager;
	Ability *m_pAbilityManager;
	Notify *m_pNotifyManager;
		// 
	Layout m_Layout;
	Behavior m_Behavior;
		// 
	struct Image
	{	Gdiplus::Bitmap *bmp, *bmpRef;
		CSize size;
	} m_ImageSys, m_ImageNormal,m_ImageDisable;
	COLORREF m_clrImageSysTransp, m_clrImageTransp;
	HCURSOR m_hCursor, *m_phCursorRef;
	CFont m_FontNormal,*m_pFontNormalRef, m_FontSelect,*m_pFontSelectRef;
	struct ToolTipText { CString butClose, butMenu, butScrollLeft,butScrollRight; } m_sToolTipText;
		// 
	int m_iScrollingStep;
	bool m_bShowBorder;
	bool m_bEqualTabsSize;
	bool m_bEnableTabRemove;
	bool m_bHideSingleTab;
	bool m_bToolTip;
	bool m_bShowButtonClose;
	bool m_bShowButtonMenu;
	bool m_bShowButtonsScroll;
	bool m_bWatchActivityCtrl;

public:
	struct Tab : HTAB__
	{	HWND wnd;
		int image;
		CString text, tooltipText;
		bool disable;
		__int64 data;
			// 
		CRect rect;
		int width;
	};
	std::vector<Tab *> m_tabs;
	typedef std::vector<Tab *>::iterator i_tabs;
	typedef std::vector<Tab *>::reverse_iterator ri_tabs;
	typedef std::vector<Tab *>::const_iterator ci_tabs;
		// 
	HTAB m_hCurTab;
	HTAB m_hHoverArea, m_hPushedArea;
	int m_iTabsOffset, m_iMaxTabsOffset;
	bool m_bPartialView, m_bScrollLeftAllow,m_bScrollRightAllow;
		// 
	struct TabDrag
	{	bool active;
		CPoint ptStart;
		std::vector<Tab *> tabsBefore;
	} m_TabDrag;
		// 
	bool m_bActive;
		// 
	CRect m_rcCtrlArea, m_rcTabs, m_rcWindows;
	CRect m_rcButtonClose, m_rcButtonMenu, m_rcButtonScrollLeft,m_rcButtonScrollRight;
		// 
	CToolTipCtrl *m_pToolTip;
	bool *m_pLifeStatus;
		// 
	ActivityHook<Private> m_ActivityHook;
	KeyboardHook<Private> m_KeyboardHook;
		// 
	static HTAB HANDLE_BUT_CLOSE, HANDLE_BUT_MENU, HANDLE_BUT_SCROLLLEFT,HANDLE_BUT_SCROLLRIGHT;
	enum { TimerIdScrollLeftClick=1, TimerIdScrollLeftScrolling, TimerIdScrollRightClick, TimerIdScrollRightScrolling };

public:
	void OnActive(bool active, HWND wnd);   // callback from ActivityHook.
	bool OnKeyDown(UINT keyCode, UINT msgFlag);   // callback from KeyboardHook.

public:
	Tab *HandleToTab(HTAB h) { return const_cast<Tab *>( static_cast<Tab const *>(h) ); }
	Tab const *HandleToTab(HTAB h) const { return static_cast<Tab const *>(h); }
	HTAB InsertTab(i_tabs before, HWND wnd, TCHAR const *text, int image);
	void Recalc(bool redraw);   // recalculate control.
	int GetFullTabsWidth();
	void RecalcScale(int visibleTabsWidth, int fullTabsWidth);
	void RecalcScroll();
	void CalcTabsWidth();
	int CalcTabWidth(HTAB tab);
	void UpdateToolTips();
	HTAB GetTabWithWindowID(int id, HTAB exceptTab) const;   // get tab whose window has 'id' except 'exceptTab'.
	bool GetTabAndIndex(int id, HTAB *tab/*out*/, int *idx/*out*/) const;
	void LButtonDown(CPoint point);
	bool IsSystemButton(HTAB tab) const;
	void StepLeft();
	void StepRight();
	void StopScrolling();
	void StartTabDragging(CPoint point);
	void StopTabDragging(bool cancel);
	void AssignHoverArea(CPoint point);
	void SetFocusInChildWnd();
	bool LoadStateInner(CArchive *ar);
	void SaveStateInner(CArchive *ar) const;
	void ShowWindow(HWND wnd, bool show);
	void MoveChangedWindow(HWND wnd, CRect const *rc, bool redraw);
	bool LoadImage(HMODULE moduleRes/*or null*/, UINT resID, bool pngImage, Gdiplus::Bitmap **bmp/*out*/) const;
	bool CreateImageList(Gdiplus::Bitmap *bmp, int imageWidth, COLORREF clrMask/*or CLR_NONE*/, COLORREF clrBack/*or CLR_NONE*/, CImageList *imageList/*out*/) const;
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
TabCtrl::HTAB TabCtrl::Private::HANDLE_BUT_CLOSE = reinterpret_cast<HTAB>(1);
TabCtrl::HTAB TabCtrl::Private::HANDLE_BUT_MENU = reinterpret_cast<HTAB>(2);
TabCtrl::HTAB TabCtrl::Private::HANDLE_BUT_SCROLLLEFT = reinterpret_cast<HTAB>(3);
TabCtrl::HTAB TabCtrl::Private::HANDLE_BUT_SCROLLRIGHT = reinterpret_cast<HTAB>(4);
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(TabCtrl,CWnd)
/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(TabCtrl, CWnd)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_CAPTURECHANGED()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_WM_TIMER()
	ON_WM_SETCURSOR()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::TabCtrl() : 
	p( *new Private(*this) )
{
}
// 
TabCtrl::~TabCtrl()
{	delete &p;
}
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::Private::Private(TabCtrl &owner) : o(owner)
{	m_pDrawManager = nullptr;
	m_pRecalcManager = this;
	m_pBehaviorManager = this;
	m_pToolTipManager = nullptr;
	m_pAbilityManager = this;
	m_pNotifyManager = nullptr;
		// 
	m_Layout = LayoutTop;
	m_Behavior = BehaviorScale;
		// 
	m_ImageSys.bmp = m_ImageSys.bmpRef = nullptr;
	m_ImageNormal.bmp = m_ImageNormal.bmpRef = nullptr;
	m_ImageDisable.bmp = m_ImageDisable.bmpRef = nullptr;
	m_ImageSys.size.SetSize(0,0);
	m_ImageNormal.size = m_ImageDisable.size = m_ImageSys.size;
	m_hCursor = nullptr;
	m_phCursorRef = nullptr;
	m_pFontNormalRef = m_pFontSelectRef = nullptr;
		// 
	m_iScrollingStep = 15;
	m_bShowBorder = true;
	m_bEqualTabsSize = false;
	m_bEnableTabRemove = false;
	m_bHideSingleTab = false;
	m_bToolTip = true;
	m_bShowButtonClose = true;
	m_bShowButtonMenu = true;
	m_bShowButtonsScroll = true;
	m_bWatchActivityCtrl = false;
		// 
	m_gdiPlusToken = 0;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiPlusToken/*out*/,&gdiplusStartupInput,nullptr);
}
//
TabCtrl::Private::~Private()
{	o.DestroyWindow();
		// 
	::delete m_ImageSys.bmp;
	::delete m_ImageNormal.bmp;
	::delete m_ImageDisable.bmp;
	if(m_hCursor)
		::DestroyCursor(m_hCursor);
	if(m_gdiPlusToken)
		Gdiplus::GdiplusShutdown(m_gdiPlusToken);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
BOOL TabCtrl::Create(LPCTSTR /*lpszClassName*/, LPCTSTR /*lpszWindowName*/, DWORD style, const RECT& rect, CWnd* parentWnd, UINT id, CCreateContext* /*context*/)
{	return Create(parentWnd,style,rect,id);
}
// 
bool TabCtrl::Create(CWnd *parent, DWORD style, RECT const &rect, UINT id)
{	p.m_hCurTab = nullptr;
	p.m_hHoverArea = nullptr;
	p.m_hPushedArea = nullptr;
	p.m_iTabsOffset = 0;
	p.m_bPartialView = p.m_bScrollLeftAllow = p.m_bScrollRightAllow = false;
	p.m_TabDrag.active = false;
	p.m_bActive = false;
		// 
	p.m_pToolTip = nullptr;
	p.m_pLifeStatus = nullptr;
		// 
	const CString classname = AfxRegisterWndClass(CS_DBLCLKS,::LoadCursor(nullptr,IDC_ARROW),nullptr,nullptr);
	if( !CWnd::Create(classname,_T(""),style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,rect,parent,id) )
		return false;
		// 
	CFont *font = CFont::FromHandle( static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)) );
	if( !GetFontNormal() )
		SetFontNormal(font);
	if( !GetFontSelect() )
		SetFontSelect(font);
		// 
	if( p.m_sToolTipText.butClose.IsEmpty() )
		p.m_sToolTipText.butClose = _T("Close");
	if( p.m_sToolTipText.butMenu.IsEmpty() )
		p.m_sToolTipText.butMenu = _T("Menu");
	if( p.m_sToolTipText.butScrollLeft.IsEmpty() )
		p.m_sToolTipText.butScrollLeft = _T("Scroll Left");
	if( p.m_sToolTipText.butScrollRight.IsEmpty() )
		p.m_sToolTipText.butScrollRight = _T("Scroll Right");
		// 
	if(p.m_bWatchActivityCtrl)
		p.m_ActivityHook.Add(m_hWnd, &p,&Private::OnActive);
		// 
	return true;
}
// 
void TabCtrl::OnDestroy()
{	if(p.m_pToolTipManager && p.m_pToolTip) 
		p.m_pToolTipManager->DestroyToolTip(p.m_pToolTip);
	DeleteAllTabs();
		// 
	if(p.m_bWatchActivityCtrl)
		p.m_ActivityHook.Delete(m_hWnd);
		// 
	if(p.m_pLifeStatus)
		*p.m_pLifeStatus = false;
		// 
	CWnd::OnDestroy();
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::OnActive(bool active, HWND /*wnd*/)
{	if(active != m_bActive)
	{	m_bActive = active;
		o.Invalidate(FALSE);
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::WatchCtrlActivity(bool watch)
{	if(watch != p.m_bWatchActivityCtrl)
	{	p.m_bWatchActivityCtrl = watch;
		p.m_bActive = false;
			// 
		if( GetSafeHwnd() )
			if(p.m_bWatchActivityCtrl)   // on.
				p.m_ActivityHook.Add(m_hWnd, &p,&Private::OnActive);
			else	// off.
				p.m_ActivityHook.Delete(m_hWnd);
	}
}
// 
bool TabCtrl::IsWatchCtrlActivity() const
{	return p.m_bWatchActivityCtrl;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::IsActive() const
{	return p.m_bActive;
}
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrl::AddTab(HWND wnd, TCHAR const *text, int image)
{	return p.InsertTab(p.m_tabs.end(),wnd,text,image);
}
// 
TabCtrl::HTAB TabCtrl::InsertTab(HTAB before, HWND wnd, TCHAR const *text, int image)
{	assert( IsTabExist(before) );
		// 
	Private::i_tabs bef = p.m_tabs.begin() + GetTabIndexByHandle(before);
	return p.InsertTab(bef,wnd,text,image);
}
// 
TabCtrl::HTAB TabCtrl::Private::InsertTab(i_tabs before, HWND wnd, TCHAR const *text, int image)
{	assert(wnd && ::IsWindow(wnd) && ::GetParent(wnd)==o.m_hWnd);
	assert(text);
	assert(image>=-1);
	assert( ::GetDlgCtrlID(wnd) );   // ID==0 - this is error.
	assert(o.GetTabWithWindowID(::GetDlgCtrlID(wnd))==nullptr);   // window with this ID has inserted.
		// 
	Tab *tab = new (std::nothrow) Tab;
	tab->wnd = wnd;
	tab->image = image;
	tab->text = text;
	tab->disable = false;
	tab->data = 0;
		// 
	if(m_pNotifyManager)
		m_pNotifyManager->OnTabPreCreate(&o,wnd,text,image);
		// 
	m_tabs.insert(before,tab);
		// 
	if(!m_hCurTab)
		m_hCurTab = o.GetFirstEnableTab();
	else
		ShowWindow(wnd,false);
		// 
	if(m_pNotifyManager)
		m_pNotifyManager->OnTabPostCreate(&o,tab);
		// 
	return tab;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::RemoveTabBefore(HTAB before, HTAB src)
{	assert(IsTabExist(before) && IsTabExist(src));
		// 
	if(before!=src)
	{	int srcIdx = GetTabIndexByHandle(src);
		const int beforeIdx = GetTabIndexByHandle(before);
		if(srcIdx+1==beforeIdx)
			return;
		p.m_tabs.insert(p.m_tabs.begin()+beforeIdx, p.HandleToTab(src));
		if(beforeIdx<srcIdx)
			++srcIdx;
		p.m_tabs.erase( p.m_tabs.begin()+srcIdx );
	}
}
// 
void TabCtrl::RemoveTabAfter(HTAB after, HTAB src)
{	assert(IsTabExist(after) && IsTabExist(src));
		// 
	if(after!=src)
	{	int srcIdx = GetTabIndexByHandle(src);
		const int afterIdx = GetTabIndexByHandle(after);
		if(srcIdx-1==afterIdx)
			return;
		p.m_tabs.insert(p.m_tabs.begin()+1+afterIdx, p.HandleToTab(src));
		if(afterIdx < srcIdx)
			++srcIdx;
		p.m_tabs.erase( p.m_tabs.begin()+srcIdx );
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::DeleteTab(HTAB tab)
{	assert( IsTabExist(tab) );
		// 
	if(tab==p.m_hPushedArea)
	{	p.StopScrolling();
		p.StopTabDragging(true);
	}
	if(tab==p.m_hHoverArea)
		p.m_hHoverArea = nullptr;
		// 
	if(p.m_hCurTab==tab)
	{	p.m_hCurTab = GetNextEnableTab(tab);
		if(!p.m_hCurTab)
			p.m_hCurTab = GetPrevEnableTab(tab);
	}
		// 
	HWND wnd = GetTabWindow(tab);
	if(wnd && ::IsWindow(wnd) &&
		::GetParent(wnd)==m_hWnd)		// if only this is our child.
		p.ShowWindow(wnd,false);
		// 
	if(p.m_pNotifyManager)
		p.m_pNotifyManager->OnTabPreDestroy(this,tab);
		// 
	delete p.HandleToTab(tab);
	p.m_tabs.erase( p.m_tabs.begin()+GetTabIndexByHandle(tab) );
		// 
	if( p.m_tabs.empty() )
	{	p.m_iTabsOffset = 0;
		p.m_bPartialView = p.m_bScrollLeftAllow = p.m_bScrollRightAllow = false;
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::DeleteAllTabs()
{	for(Private::i_tabs i=p.m_tabs.begin(), e=p.m_tabs.end(); i!=e; ++i)
	{	if(p.m_pNotifyManager)
			p.m_pNotifyManager->OnTabPreDestroy(this,*i);
			// 
		HWND wnd = GetTabWindow(*i);
		if(wnd && ::IsWindow(wnd) && 
			::GetParent(wnd)==m_hWnd)   // if only this is our child.
			p.ShowWindow(wnd,false);
			// 
		delete *i;
	}
	p.m_tabs.clear();
	p.StopScrolling();
	p.StopTabDragging(true);
	p.m_hHoverArea = p.m_hPushedArea = p.m_hCurTab = nullptr;
	p.m_iTabsOffset = 0;
	p.m_bPartialView = p.m_bScrollLeftAllow = p.m_bScrollRightAllow = false;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetDrawManager(Draw *ptr/*or null*/)
{	p.m_pDrawManager = ptr;
}
// 
TabCtrl::Draw *TabCtrl::GetDrawManager() const
{	return p.m_pDrawManager;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetRecalcManager(IRecalc *ptr/*or null*/)
{	p.m_pRecalcManager = (ptr ? ptr : &p);
}
// 
TabCtrl::IRecalc *TabCtrl::GetRecalcManager() const
{	return p.m_pRecalcManager;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetBehaviorManager(IBehavior *ptr/*or null*/)
{	p.m_pBehaviorManager = (ptr ? ptr : &p);
}
// 
TabCtrl::IBehavior *TabCtrl::GetBehaviorManager() const
{	return p.m_pBehaviorManager;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetToolTipManager(ToolTip *ptr/*or null*/)
{	if(ptr!=p.m_pToolTipManager)
	{	if(p.m_pToolTipManager && p.m_pToolTip) 
			p.m_pToolTipManager->DestroyToolTip(p.m_pToolTip);
		p.m_pToolTip = nullptr;
		p.m_pToolTipManager = ptr;
	}
}
// 
TabCtrl::ToolTip *TabCtrl::GetToolTipManager() const
{	return p.m_pToolTipManager;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetAbilityManager(Ability *ptr/*or null*/)
{	p.m_pAbilityManager = (ptr ? ptr : &p);
}
// 
TabCtrl::Ability *TabCtrl::GetAbilityManager() const
{	return p.m_pAbilityManager;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetNotifyManager(Notify *ptr/*or null*/)
{	p.m_pNotifyManager = ptr;
}
// 
TabCtrl::Notify *TabCtrl::GetNotifyManager() const
{	return p.m_pNotifyManager;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetLayout(Layout layout)
{	p.m_Layout = layout;
}
// 
TabCtrl::Layout TabCtrl::GetLayout() const
{	return p.m_Layout;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetBehavior(Behavior behavior)
{	p.m_Behavior = behavior;
}
// 
TabCtrl::Behavior TabCtrl::GetBehavior() const
{	return p.m_Behavior;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::CreateSystemImages(HMODULE moduleRes/*or null*/, UINT resID/*or 0*/, bool pngImage, int imageWidth, COLORREF clrTransp/*=CLR_NONE*/)
{	assert(!resID || imageWidth>0);
		// 
	if(p.m_ImageSys.bmp)
	{	::delete p.m_ImageSys.bmp;
		p.m_ImageSys.bmp = nullptr;
	}
		// 
	const bool res = (!resID || p.LoadImage(moduleRes,resID,pngImage,&p.m_ImageSys.bmp/*out*/));
	p.m_ImageSys.bmpRef = p.m_ImageSys.bmp;
		// 
	(p.m_ImageSys.bmpRef ?
		p.m_ImageSys.size.SetSize(imageWidth, p.m_ImageSys.bmpRef->GetHeight() ) :
		p.m_ImageSys.size.SetSize(0,0));
	p.m_clrImageSysTransp = clrTransp;
		// 
	return res;
}
// 
void TabCtrl::SetSystemImagesRef(Gdiplus::Bitmap *bmp, int imageWidth, COLORREF clrTransp/*=CLR_NONE*/)
{	assert(!bmp || imageWidth>0);
		// 
	if(p.m_ImageSys.bmp)
	{	::delete p.m_ImageSys.bmp;
		p.m_ImageSys.bmp = nullptr;
	}
	p.m_ImageSys.bmpRef = bmp;
		// 
	(p.m_ImageSys.bmpRef ?
		p.m_ImageSys.size.SetSize(imageWidth, p.m_ImageSys.bmpRef->GetHeight() ) :
		p.m_ImageSys.size.SetSize(0,0));
	p.m_clrImageSysTransp = clrTransp;
}
//
Gdiplus::Bitmap *TabCtrl::GetSystemImages() const
{	return p.m_ImageSys.bmpRef;
}
// 
bool TabCtrl::GetSystemImageList(COLORREF clrDstBack/*or CLR_NONE*/, CImageList *imageList/*out*/) const
{	assert(imageList);
		// 
	return p.m_ImageSys.bmpRef && p.CreateImageList(p.m_ImageSys.bmpRef, p.m_ImageSys.size.cx, p.m_clrImageSysTransp, clrDstBack, imageList/*out*/);
}
// 
CSize TabCtrl::GetSystemImageSize() const
{	return p.m_ImageSys.size;
}
// 
COLORREF TabCtrl::GetSystemImagesTranspColor() const
{	return p.m_clrImageSysTransp;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
bool TabCtrl::CreateImages(HMODULE moduleRes/*or null*/, UINT resNormalID/*or 0*/, UINT resDisableID/*or 0*/, bool pngImage, int imageWidth, COLORREF clrTransp/*=CLR_NONE*/)
{	assert((!resNormalID && !resDisableID) || imageWidth>0);
		// 
	if(p.m_ImageNormal.bmp)
	{	::delete p.m_ImageNormal.bmp;
		p.m_ImageNormal.bmp = nullptr;
	}
	if(p.m_ImageDisable.bmp)
	{	::delete p.m_ImageDisable.bmp;
		p.m_ImageDisable.bmp = nullptr;
	}
		// 
	bool res = true;
	if(resNormalID)
		if( !p.LoadImage(moduleRes,resNormalID,pngImage,&p.m_ImageNormal.bmp/*out*/) )
			res = false;
	if(resDisableID)
		if( !p.LoadImage(moduleRes,resDisableID,pngImage,&p.m_ImageDisable.bmp/*out*/) )
			res = false;
	p.m_ImageNormal.bmpRef = p.m_ImageNormal.bmp;
	p.m_ImageDisable.bmpRef = p.m_ImageDisable.bmp;
		// 
	(p.m_ImageNormal.bmpRef ?
		p.m_ImageNormal.size.SetSize(imageWidth, p.m_ImageNormal.bmpRef->GetHeight() ) :
		p.m_ImageNormal.size.SetSize(0,0));
	(p.m_ImageDisable.bmpRef ?
		p.m_ImageDisable.size.SetSize(imageWidth, p.m_ImageDisable.bmpRef->GetHeight() ) :
		p.m_ImageDisable.size.SetSize(0,0));
	p.m_clrImageTransp = clrTransp;
		// 
	return res;
}
//
void TabCtrl::SetImagesRef(Gdiplus::Bitmap *bmpNormal/*or 0*/, Gdiplus::Bitmap *bmpDisable/*or 0*/, int imageWidth, COLORREF clrTransp/*=CLR_NONE*/)
{	assert((!bmpNormal && !bmpDisable) || imageWidth>0);
		// 
	if(p.m_ImageNormal.bmp)
	{	::delete p.m_ImageNormal.bmp;
		p.m_ImageNormal.bmp = nullptr;
	}
	if(p.m_ImageDisable.bmp)
	{	::delete p.m_ImageDisable.bmp;
		p.m_ImageDisable.bmp = nullptr;
	}
		// 
	p.m_ImageNormal.bmpRef = bmpNormal;
	p.m_ImageDisable.bmpRef = bmpDisable;
		// 
	(p.m_ImageNormal.bmpRef ?
		p.m_ImageNormal.size.SetSize(imageWidth, p.m_ImageNormal.bmpRef->GetHeight() ) :
		p.m_ImageNormal.size.SetSize(0,0));
	(p.m_ImageDisable.bmpRef ?
		p.m_ImageDisable.size.SetSize(imageWidth, p.m_ImageDisable.bmpRef->GetHeight() ) :
		p.m_ImageDisable.size.SetSize(0,0));
	p.m_clrImageTransp = clrTransp;
}
//
void TabCtrl::GetImages(Gdiplus::Bitmap **normal/*out,or null*/, Gdiplus::Bitmap **disable/*out,or null*/) const
{	if(normal)
		*normal = p.m_ImageNormal.bmpRef;
	if(disable)
		*disable = p.m_ImageDisable.bmpRef;
}
//
bool TabCtrl::GetImageList(COLORREF clrDstBack/*or CLR_NONE*/, CImageList *normal/*out,or null*/, CImageList *disable/*out,or null*/) const
{	if(normal)
		if(!p.m_ImageNormal.bmpRef || !p.CreateImageList(p.m_ImageNormal.bmpRef, p.m_ImageNormal.size.cx, p.m_clrImageTransp, clrDstBack, normal/*out*/))
			return false;
	if(disable)
		if(!p.m_ImageDisable.bmpRef || !p.CreateImageList(p.m_ImageDisable.bmpRef, p.m_ImageDisable.size.cx, p.m_clrImageTransp, clrDstBack, disable/*out*/))
			return false;
	return true;
}
// 
void TabCtrl::GetImageSize(CSize *szNormal/*out,or null*/, CSize *szDisable/*out,or null*/) const
{	if(szNormal)
	{	szNormal->cx = p.m_ImageNormal.size.cx;
		szNormal->cy = p.m_ImageNormal.size.cy;
	}
	if(szDisable)
	{	szDisable->cx = p.m_ImageDisable.size.cx;
		szDisable->cy = p.m_ImageDisable.size.cy;
	}
}
//
COLORREF TabCtrl::GetImagesTranspColor() const
{	return p.m_clrImageTransp;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::SetCursor(UINT resID)
{	return SetCursor(nullptr,resID);
}
// 
bool TabCtrl::SetCursor(HMODULE module, UINT resID)
{	if(p.m_hCursor)
	{	::DestroyCursor(p.m_hCursor);
		p.m_hCursor = nullptr;
	}
	p.m_phCursorRef = nullptr;
		// 
	if(resID)
	{	if(!module)
			module = AfxGetResourceHandle();
		if(!module)
			return false;
		p.m_hCursor = ::LoadCursor(module,MAKEINTRESOURCE(resID));
		if(!p.m_hCursor)
			return false;
		p.m_phCursorRef = &p.m_hCursor;
	}
	return true;
}
// 
bool TabCtrl::SetCursor(HCURSOR cursor)
{	if(p.m_hCursor)
	{	::DestroyCursor(p.m_hCursor);
		p.m_hCursor = nullptr;
	}
	p.m_phCursorRef = nullptr;
		// 
	if(cursor)
	{	p.m_hCursor = static_cast<HCURSOR>( CopyImage(cursor,IMAGE_CURSOR,0,0,0) );
		if(!p.m_hCursor)
			return false;
		p.m_phCursorRef = &p.m_hCursor;
	}
	return true;
}
//
void TabCtrl::SetCursorRef(HCURSOR *phCursor)
{	if(p.m_hCursor)
	{	::DestroyCursor(p.m_hCursor);
		p.m_hCursor = nullptr;
	}
	p.m_phCursorRef = phCursor;
}
// 
HCURSOR TabCtrl::GetCursor() const
{	return (p.m_phCursorRef ? *p.m_phCursorRef : nullptr);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::ShowBorder(bool show)
{	p.m_bShowBorder = show;
}
// 
bool TabCtrl::IsBorderVisible() const
{	return p.m_bShowBorder;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetTabText(HTAB tab, TCHAR const *text)
{	assert( IsTabExist(tab) );
	assert(text);
		// 
	p.HandleToTab(tab)->text = text;
}
// 
CString TabCtrl::GetTabText(HTAB tab) const
{	assert( IsTabExist(tab) );
		// 
	return p.HandleToTab(tab)->text;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetTabImage(HTAB tab, int image)
{	assert( IsTabExist(tab) );
	assert(image>=-1);
	p.HandleToTab(tab)->image = image;
}
// 
int TabCtrl::GetTabImage(HTAB tab) const
{	assert( IsTabExist(tab) );
	return p.HandleToTab(tab)->image;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetTabWindow(HTAB tab, HWND wnd)
{	assert( IsTabExist(tab) );
	assert(wnd && ::IsWindow(wnd) && ::GetParent(wnd)==m_hWnd);
	assert( ::GetDlgCtrlID(wnd) );
	assert(p.GetTabWithWindowID(::GetDlgCtrlID(wnd),tab)==nullptr);   // window with this ID has inserted.
		// 
	p.HandleToTab(tab)->wnd = wnd;
}
// 
HWND TabCtrl::GetTabWindow(HTAB tab) const
{	assert( IsTabExist(tab) );
	return p.HandleToTab(tab)->wnd;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetTabData(HTAB tab, __int64 data)
{	assert( IsTabExist(tab) );
	p.HandleToTab(tab)->data = data;
}
// 
__int64 TabCtrl::GetTabData(HTAB tab) const
{	assert( IsTabExist(tab) );
	return p.HandleToTab(tab)->data;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// copy: text, image, data, tooltip text and disable/enable state.
// 
void TabCtrl::CopyTabContent(HTAB dst, TabCtrl const *tabCtrlSrc, HTAB src)
{	assert( IsTabExist(dst) );
	assert(tabCtrlSrc && tabCtrlSrc->IsTabExist(src));
		// 
	const CString text = tabCtrlSrc->GetTabText(src);
	const CString tooltip_text = tabCtrlSrc->GetTabTooltipText(src);
		// 
	SetTabText(dst,text);
	SetTabTooltipText(dst,tooltip_text);
		// 
	SetTabImage(dst, tabCtrlSrc->GetTabImage(src) );
	SetTabData(dst, tabCtrlSrc->GetTabData(src) );
	DisableTab(dst, tabCtrlSrc->IsTabDisabled(src) );
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetTabTooltipText(HTAB tab, TCHAR const *text)
{	assert( IsTabExist(tab) );
	assert(text);
		// 
	p.HandleToTab(tab)->tooltipText = text;
}
// 
CString TabCtrl::GetTabTooltipText(HTAB tab) const
{	assert( IsTabExist(tab) );
		// 
	return p.HandleToTab(tab)->tooltipText;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetButtonCloseToolTipText(TCHAR const *text)
{	p.m_sToolTipText.butClose = text;
}
// 
CString TabCtrl::GetButtonCloseToolTipText() const
{	return p.m_sToolTipText.butClose;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetButtonMenuToolTipText(TCHAR const *text)
{	p.m_sToolTipText.butMenu = text;
}
// 
CString TabCtrl::GetButtonMenuToolTipText() const
{	return p.m_sToolTipText.butMenu;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetButtonScrollLeftToolTipText(TCHAR const *text)
{	p.m_sToolTipText.butScrollLeft = text;
}
// 
CString TabCtrl::GetButtonScrollLeftToolTipText() const
{	return p.m_sToolTipText.butScrollLeft;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetButtonScrollRightToolTipText(TCHAR const *text)
{	p.m_sToolTipText.butScrollRight = text;
}
// 
CString TabCtrl::GetButtonScrollRightToolTipText() const
{	return p.m_sToolTipText.butScrollRight;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Update(bool redraw)
{	p.Recalc(redraw);
	if(redraw)
		Invalidate(FALSE);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnSize(UINT nType, int cx, int cy)
{	CWnd::OnSize(nType, cx, cy);
		// 
	p.Recalc(true);
	Invalidate(FALSE);
}
/////////////////////////////////////////////////////////////////////////////
// Recalculate control.
// 
void TabCtrl::Private::Recalc(bool redraw)
{	o.GetClientRect(&m_rcWindows);
		// 
	if(m_bShowBorder)
	{	const int width = o.GetBorderWidth();
		m_rcWindows.DeflateRect(width,width);
	}
		// 
	const int count = o.GetNumberTabs();
	const bool bHideSingleTab = (count==1 && o.IsHideSingleTab());
		// 
	if(count>0 && !bHideSingleTab)
	{	m_rcCtrlArea = m_rcWindows;
			// 
		if(m_Layout==LayoutTop)
		{	m_rcWindows.top = m_rcCtrlArea.bottom = m_rcCtrlArea.top + o.CalcCtrlAreaHeight();
			m_rcWindows.bottom = std::max(m_rcWindows.bottom,m_rcWindows.top);
		}
		else
		{	m_rcWindows.bottom = m_rcCtrlArea.top = m_rcCtrlArea.bottom - o.CalcCtrlAreaHeight();
			m_rcWindows.top = std::min(m_rcWindows.top,m_rcWindows.bottom);
		}
			// 
			// 
		if(m_hCurTab && !o.IsTabExist(m_hCurTab))
			m_hCurTab=nullptr;
			// 
		HTAB hOldCurTab = m_hCurTab;
			// 
		if(m_hCurTab==nullptr)
			m_hCurTab = o.GetFirstEnableTab();
		else
			if( HandleToTab(m_hCurTab)->disable )
			{	HTAB hCurTab = o.GetNextEnableTab(m_hCurTab);
				if(hCurTab==nullptr)
					hCurTab = o.GetPrevEnableTab(m_hCurTab);
				m_hCurTab = hCurTab;
			}
			// 
		if(m_hCurTab)
		{	CRect rcWindows(m_rcWindows);
			rcWindows.DeflateRect( o.GetWindowsAreaPadding() );
			MoveChangedWindow(HandleToTab(m_hCurTab)->wnd,&rcWindows,redraw);
			if( !::IsWindowVisible(HandleToTab(m_hCurTab)->wnd) )
				ShowWindow(HandleToTab(m_hCurTab)->wnd,true);
		}
		if(hOldCurTab!=m_hCurTab)
			if(hOldCurTab)
				ShowWindow(HandleToTab(hOldCurTab)->wnd,false);
			// 
			// 
		CRect rcCtrlAreaPadding = o.GetControlAreaPadding();
		m_rcTabs = m_rcCtrlArea;
		m_rcTabs.DeflateRect(&rcCtrlAreaPadding);
			// 
		CalcTabsWidth();
			// 
			// 
		const bool bShowCloseButton = (m_bShowButtonClose && m_ImageSys.bmpRef && m_pAbilityManager->CanShowButtonClose(&o));
		const bool bShowMenuButton = (m_bShowButtonMenu && m_ImageSys.bmpRef && m_pAbilityManager->CanShowButtonMenu(&o));
		const bool bShowScrollButtons = (m_Behavior==BehaviorScroll && m_bShowButtonsScroll && 
			m_ImageSys.bmpRef && m_pAbilityManager->CanShowButtonScroll(&o));
			// 
		const int iSysImagePosY = (m_rcTabs.top + m_rcTabs.bottom - m_ImageSys.size.cy) / 2;
			// 
		if(bShowCloseButton)
		{	const CRect rcCloseHorzMargin = o.GetButtonCloseHorzMargin();
				// 
			m_rcButtonClose = m_rcTabs;
			m_rcButtonClose.right -= rcCloseHorzMargin.right;
			m_rcButtonClose.left = m_rcButtonClose.right - m_ImageSys.size.cx;
			m_rcTabs.right = m_rcButtonClose.left - rcCloseHorzMargin.left;
				// 
			m_rcButtonClose.top = iSysImagePosY;
			m_rcButtonClose.bottom = m_rcButtonClose.top + m_ImageSys.size.cy;
		}
		else
			m_rcButtonClose.SetRectEmpty();
			// 
		if(bShowMenuButton)
		{	const CRect rcMenuHorzMargin = o.GetButtonMenuHorzMargin();
				// 
			m_rcButtonMenu = m_rcTabs;
			m_rcButtonMenu.right -= rcMenuHorzMargin.right;
			m_rcButtonMenu.left = m_rcButtonMenu.right - m_ImageSys.size.cx;
			m_rcTabs.right = m_rcButtonMenu.left - rcMenuHorzMargin.left;
				// 
			m_rcButtonMenu.top = iSysImagePosY;
			m_rcButtonMenu.bottom = m_rcButtonMenu.top + m_ImageSys.size.cy;
		}
		else
			m_rcButtonMenu.SetRectEmpty();
			// 
		if(bShowScrollButtons)
		{	const CRect rcLeftScrollMargin = o.GetButtonScrollLeftHorzMargin();
			const CRect rcRightScrollMargin = o.GetButtonScrollRightHorzMargin();
				// 
			m_rcButtonScrollRight = m_rcTabs;
			m_rcButtonScrollRight.right -= rcRightScrollMargin.right;
			m_rcButtonScrollRight.left = m_rcButtonScrollRight.right - m_ImageSys.size.cx;
			m_rcButtonScrollLeft.right = m_rcButtonScrollRight.left - rcRightScrollMargin.left - rcLeftScrollMargin.right;
			m_rcButtonScrollLeft.left = m_rcButtonScrollLeft.right - m_ImageSys.size.cx;
			m_rcTabs.right = m_rcButtonScrollLeft.left - rcLeftScrollMargin.left;
				// 
			m_rcButtonScrollLeft.top = m_rcButtonScrollRight.top = iSysImagePosY;
			m_rcButtonScrollLeft.bottom = m_rcButtonScrollRight.bottom = m_rcButtonScrollLeft.top + m_ImageSys.size.cy;
		}
		else
		{	m_rcButtonScrollLeft.SetRectEmpty();
			m_rcButtonScrollRight.SetRectEmpty();
		}
			// 
			// 
		const int visibleTabsWidth = std::max(0,m_rcTabs.Width()-1);
		const int fullTabsWidth = GetFullTabsWidth();
			// 
		m_iMaxTabsOffset = std::max(0, fullTabsWidth-visibleTabsWidth);
			// 
		if(m_iTabsOffset<0)
			m_iTabsOffset = 0;
		if(m_iTabsOffset>m_iMaxTabsOffset)
			m_iTabsOffset = m_iMaxTabsOffset;
			// 
		m_bPartialView = (m_iMaxTabsOffset>0);
		m_bScrollLeftAllow = (m_bPartialView && m_iTabsOffset>0);
		m_bScrollRightAllow = (m_bPartialView && m_iTabsOffset<m_iMaxTabsOffset);
			// 
		if(m_Behavior==BehaviorScale)
			RecalcScale(visibleTabsWidth,fullTabsWidth);
		else
			RecalcScroll();
	}
	else
	{	if(bHideSingleTab)
		{	Tab *tab = m_tabs.front();
			tab->rect.SetRectEmpty();
			tab->width = 0;
				// 
			if(m_hCurTab && HandleToTab(m_hCurTab)!=tab)
				if( ::IsWindow(HandleToTab(m_hCurTab)->wnd) )
					::ShowWindow(HandleToTab(m_hCurTab)->wnd,SW_HIDE);
				// 
			m_hCurTab = (!tab->disable ? tab : nullptr);
				// 
			if(m_hCurTab)
			{	CRect rcWindows(m_rcWindows);
				rcWindows.DeflateRect( o.GetWindowsAreaPadding() );
				MoveChangedWindow(HandleToTab(m_hCurTab)->wnd,&rcWindows,redraw);
				if( !::IsWindowVisible(HandleToTab(m_hCurTab)->wnd) )
					ShowWindow(HandleToTab(m_hCurTab)->wnd,true);
			}
			else
				if( ::IsWindow(tab->wnd) )
					::ShowWindow(tab->wnd,SW_HIDE);
		}
			// 
		m_rcCtrlArea.SetRectEmpty();
		m_rcTabs.SetRectEmpty();
		m_rcButtonClose.SetRectEmpty();
		m_rcButtonMenu.SetRectEmpty();
		m_rcButtonScrollLeft.SetRectEmpty();
		m_rcButtonScrollRight.SetRectEmpty();
	}
		// 
		// 
	UpdateToolTips();
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::CalcTabsWidth()
{	const int iMinTabWidth = o.GetTabMinWidth();
		// 
	if(!m_bEqualTabsSize)
		for(ci_tabs i=m_tabs.begin(), e=m_tabs.end(); i!=e; ++i)
			(*i)->width = std::max(iMinTabWidth,CalcTabWidth(*i));
	else
	{	int maxWidth = 0;
		ci_tabs i, e=m_tabs.end();
			// 
		for(i=m_tabs.begin(); i!=e; ++i)
			maxWidth = std::max(maxWidth,CalcTabWidth(*i));
		maxWidth = std::max(maxWidth,iMinTabWidth);
			// 
		for(i=m_tabs.begin(); i!=e; ++i)
			(*i)->width = maxWidth;
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrl::Private::GetFullTabsWidth()
{	int width = 0;
		// 
	for(i_tabs i=m_tabs.begin(), e=m_tabs.end(); i!=e; ++i)
		width += (*i)->width;
		// 
	const CRect rcTabHorzMargin = o.GetTabHorzMargin();
	return width + static_cast<int>( m_tabs.size() )*(rcTabHorzMargin.left+rcTabHorzMargin.right);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::RecalcScale(int visibleTabsWidth, int fullTabsWidth)
{	const CRect rcTabHorzMargin = o.GetTabHorzMargin();
	const bool partialView = (fullTabsWidth > visibleTabsWidth);
		// 
	int pos = m_rcTabs.left;
	i_tabs i, e=m_tabs.end();
		// 
	if(!partialView)
		for(i=m_tabs.begin(); i!=e; ++i)
		{	(*i)->rect = m_rcTabs;
			(*i)->rect.left = pos;
			(*i)->rect.right = pos += (rcTabHorzMargin.left + (*i)->width + rcTabHorzMargin.right);
			(*i)->rect.DeflateRect(rcTabHorzMargin.left,0,rcTabHorzMargin.right,0);
		}
	else
	{	const int iMinTabWidth = o.GetTabMinWidth();
		const int totalTabsIndent = static_cast<int>( m_tabs.size() ) * (rcTabHorzMargin.left + rcTabHorzMargin.right);
		const int iEqualWidth = std::max(1, (visibleTabsWidth-totalTabsIndent) / static_cast<int>( m_tabs.size() ));
			// 
		if(m_bEqualTabsSize)
			for(i=m_tabs.begin(); i!=e; ++i)
			{	(*i)->rect = m_rcTabs;
				(*i)->rect.left = pos;
				(*i)->rect.right = pos += (rcTabHorzMargin.left + std::max(iMinTabWidth,iEqualWidth) + rcTabHorzMargin.right);
				(*i)->rect.DeflateRect(rcTabHorzMargin.left,0,rcTabHorzMargin.right,0);
			}
		else
		{	int iTotalCorrectWidth = 0;
			for(i=m_tabs.begin(); i!=e; ++i)
				if((*i)->width>iEqualWidth)
					iTotalCorrectWidth += (*i)->width-iEqualWidth;
				// 
			const int iTail = fullTabsWidth - visibleTabsWidth;
			int width;
				// 
			for(i=m_tabs.begin(); i!=e; ++i)
			{	if(i!=e-1)
				{	if((*i)->width<=iEqualWidth)
						width = (*i)->width;
					else
						width = std::max(iMinTabWidth, (*i)->width - static_cast<int>(static_cast<double>(iTail) * (static_cast<double>((*i)->width - iEqualWidth) / static_cast<double>(iTotalCorrectWidth)) + 0.5) );
				}
				else
					width = std::max<int>(iMinTabWidth, (m_rcTabs.right-1) - pos - (rcTabHorzMargin.left + rcTabHorzMargin.right));
					// 
				(*i)->rect = m_rcTabs;
				(*i)->rect.left = pos;
				(*i)->rect.right = pos += (rcTabHorzMargin.left + width + rcTabHorzMargin.right);
				(*i)->rect.DeflateRect(rcTabHorzMargin.left,0,rcTabHorzMargin.right,0);
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::RecalcScroll()
{	const CRect rcTabHorzMargin = o.GetTabHorzMargin();
		// 
	int pos = m_rcTabs.left - m_iTabsOffset;
		// 
	for(i_tabs i=m_tabs.begin(), e=m_tabs.end(); i!=e; ++i)
	{	(*i)->rect = m_rcTabs;
		(*i)->rect.left = pos;
		(*i)->rect.right = pos += (rcTabHorzMargin.left + (*i)->width + rcTabHorzMargin.right);
		(*i)->rect.DeflateRect(rcTabHorzMargin.left,0,rcTabHorzMargin.right,0);
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrl::CalcCtrlAreaHeight()
{	CClientDC dc(this);
	CFont *pOldFont = dc.SelectObject( GetFontNormal() );
	int iTextHeight = dc.GetTextExtent(_T("H"),1).cy;
	dc.SelectObject( GetFontSelect() );
	iTextHeight = std::max<int>(iTextHeight, dc.GetTextExtent(_T("H"),1).cy);
	dc.SelectObject(pOldFont);
		// 
	const CRect rcCtrlAreaPadding = GetControlAreaPadding();
	const CRect rcTabPadding = GetTabPadding();
		// 
	return rcCtrlAreaPadding.top + 
		std::max( rcTabPadding.top + std::max<int>(std::max(p.m_ImageNormal.size.cy,p.m_ImageDisable.size.cy),iTextHeight) + rcTabPadding.bottom, p.m_ImageSys.size.cy) +
		rcCtrlAreaPadding.bottom;
}
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrl::Private::CalcTabWidth(HTAB tab)
{	_ASSERT( o.IsTabExist(tab) );
		// 
	int imageWidth = 0;
		// 
	if(HandleToTab(tab)->image!=-1 &&
		((!HandleToTab(tab)->disable && m_ImageNormal.bmpRef) || 
		(HandleToTab(tab)->disable && m_ImageDisable.bmpRef)))
			imageWidth = (!HandleToTab(tab)->disable ? m_ImageNormal.size.cx : m_ImageDisable.size.cx) + o.GetTabImageTextSpace();
		// 
	CClientDC dc(&o);
	CFont *pOldFont = dc.SelectObject( o.GetFontNormal() );
	int textWidth = dc.GetTextExtent(HandleToTab(tab)->text).cx;
	dc.SelectObject( o.GetFontSelect() );
	textWidth = std::max<int>(textWidth, dc.GetTextExtent(HandleToTab(tab)->text).cx);
	dc.SelectObject(pOldFont);
		// 
	const CRect rcTabPadding = o.GetTabPadding();
	return rcTabPadding.left + imageWidth + textWidth + o.GetTabExtraWidth(tab) + rcTabPadding.right;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Don't use CWnd::PreTranslateMessage to call CToolTipCtrl::RelayEvent.
//  If TabCtrl is in a Regular MFC DLL, then CWnd::PreTranslateMessage is not called. 
// 
LRESULT TabCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{	if(p.m_bToolTip)
		if(p.m_pToolTip && p.m_pToolTip->m_hWnd)
			if(message==WM_LBUTTONDOWN || message==WM_LBUTTONUP || message==WM_MBUTTONDOWN || message==WM_MBUTTONUP ||   // All messages required for TTM_RELAYEVENT.
				message==WM_MOUSEMOVE || message==WM_NCMOUSEMOVE || message==WM_RBUTTONDOWN || message==WM_RBUTTONUP)
			{
				// Don't use AfxGetCurrentMessage(). If TabCtrl is in a Regular MFC DLL, then we get an empty MSG. 
				MSG msg = {m_hWnd,message,wParam,lParam,0,{0,0}};
				p.m_pToolTip->RelayEvent(&msg);
			}
	return CWnd::WindowProc(message,wParam,lParam);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::UpdateToolTips()
{	if(m_pToolTipManager)
	{	if(!m_pToolTip)
			m_pToolTip = m_pToolTipManager->CreateToolTip(&o);
			// 
		if(m_pToolTip && m_pToolTip->m_hWnd)
		{	for(int c=m_pToolTip->GetToolCount(); c>0; --c)
				m_pToolTip->DelTool(&o,c);
					// 
			int c = 1;
			if(!m_rcButtonClose.IsRectEmpty() && !m_sToolTipText.butClose.IsEmpty())
				m_pToolTip->AddTool(&o,m_sToolTipText.butClose,&m_rcButtonClose,c++);
			if(!m_rcButtonMenu.IsRectEmpty() && !m_sToolTipText.butMenu.IsEmpty())
				m_pToolTip->AddTool(&o,m_sToolTipText.butMenu,&m_rcButtonMenu,c++);
			if(!m_rcButtonScrollLeft.IsRectEmpty() && !m_sToolTipText.butScrollLeft.IsEmpty())
				m_pToolTip->AddTool(&o,m_sToolTipText.butScrollLeft,&m_rcButtonScrollLeft,c++);
			if(!m_rcButtonScrollRight.IsRectEmpty() && !m_sToolTipText.butScrollRight.IsEmpty())
				m_pToolTip->AddTool(&o,m_sToolTipText.butScrollRight,&m_rcButtonScrollRight,c++);
				// 
			for(Private::i_tabs i=m_tabs.begin(), e=m_tabs.end(); i!=e; ++i)
			{	CRect rc((*i)->rect);
				rc.left = std::max(rc.left,m_rcTabs.left);
				rc.right = std::min(rc.right,m_rcTabs.right);
					// 
				if( !rc.IsRectEmpty() )
					if( !(*i)->tooltipText.IsEmpty() )
						m_pToolTip->AddTool(&o,(*i)->tooltipText,&rc,c++);
					else 
						if(rc.Width() < (*i)->width)   // partial view of tab.
							m_pToolTip->AddTool(&o,(*i)->text,&rc,c++);
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnPaint()
{	if(!p.m_pDrawManager)
	{	CPaintDC dc(this);
		return;
	}
		// 
	Private::VirtualWindow virtwnd(this);
	if( !virtwnd.GetSafeHdc() )
	{	CPaintDC dc(this);
		return;
	}
		// 
	p.m_pDrawManager->DrawBegin(this,&virtwnd);
		// 
	if( !p.m_tabs.empty() )
	{	p.m_pDrawManager->DrawControlAreaBack(this,&virtwnd,&p.m_rcCtrlArea);
			// 
		CFont *pOldFont = static_cast<CFont*>( virtwnd.SelectObject(GetFontNormal()) );
			// 
		CRgn rgn;
		rgn.CreateRectRgn(p.m_rcTabs.left,p.m_rcTabs.top,std::max(p.m_rcTabs.left,p.m_rcTabs.right),p.m_rcTabs.bottom);
		virtwnd.SelectClipRgn(&rgn,RGN_COPY);
			// 
		if( p.m_pDrawManager->IsDrawTabsStraightOrder(this) )   // left to right.
		{	for(Private::i_tabs i=p.m_tabs.begin(), e=p.m_tabs.end(); i!=e; ++i)
				if(*i!=p.m_hCurTab && IsTabVisible(*i,nullptr))
					p.m_pDrawManager->DrawTab(this,&virtwnd,*i,&rgn);
		}
		else	// right to left.
			for(Private::ri_tabs i=p.m_tabs.rbegin(), e=p.m_tabs.rend(); i!=e; ++i)
				if(*i!=p.m_hCurTab && IsTabVisible(*i,nullptr))
					p.m_pDrawManager->DrawTab(this,&virtwnd,*i,&rgn);
			// 
		if(p.m_hCurTab)
			if(!p.m_bWatchActivityCtrl || p.m_bActive)
			{	CFont *oldFont = static_cast<CFont*>( virtwnd.SelectObject(GetFontSelect()) );
				p.m_pDrawManager->DrawTab(this,&virtwnd,p.m_hCurTab,&rgn);
				virtwnd.SelectObject(oldFont);
			}
			else
				p.m_pDrawManager->DrawTab(this,&virtwnd,p.m_hCurTab,&rgn);
			// 
		virtwnd.SelectClipRgn(nullptr,RGN_COPY);
		virtwnd.SelectObject(pOldFont);
			// 
		if( !p.m_rcButtonScrollLeft.IsRectEmpty() )
		{	bool hover = p.m_hHoverArea==Private::HANDLE_BUT_SCROLLLEFT && (p.m_hPushedArea==nullptr || p.m_hPushedArea==Private::HANDLE_BUT_SCROLLLEFT);
			p.m_pDrawManager->DrawButtonScrollLeft(this,&virtwnd,&p.m_rcButtonScrollLeft,hover,p.m_hPushedArea==Private::HANDLE_BUT_SCROLLLEFT,p.m_bScrollLeftAllow);
			hover = p.m_hHoverArea==Private::HANDLE_BUT_SCROLLRIGHT && (p.m_hPushedArea==nullptr || p.m_hPushedArea==Private::HANDLE_BUT_SCROLLRIGHT);
			p.m_pDrawManager->DrawButtonScrollRight(this,&virtwnd,&p.m_rcButtonScrollRight,hover,p.m_hPushedArea==Private::HANDLE_BUT_SCROLLRIGHT,p.m_bScrollRightAllow);
		}
		if( !p.m_rcButtonMenu.IsRectEmpty() )
		{	const bool hover = p.m_hHoverArea==Private::HANDLE_BUT_MENU && (p.m_hPushedArea==nullptr || p.m_hPushedArea==Private::HANDLE_BUT_MENU);
			p.m_pDrawManager->DrawButtonMenu(this,&virtwnd,&p.m_rcButtonMenu,hover,p.m_hPushedArea==Private::HANDLE_BUT_MENU,p.m_bPartialView);
		}
		if( !p.m_rcButtonClose.IsRectEmpty() )
		{	const bool hover = p.m_hHoverArea==Private::HANDLE_BUT_CLOSE && (p.m_hPushedArea==nullptr || p.m_hPushedArea==Private::HANDLE_BUT_CLOSE);
			p.m_pDrawManager->DrawButtonClose(this,&virtwnd,&p.m_rcButtonClose,hover,p.m_hPushedArea==Private::HANDLE_BUT_CLOSE);
		}
	}
		// 
	const CRect rcWndsAreaPadding = GetWindowsAreaPadding();
	if(p.m_tabs.empty() || p.m_hCurTab==nullptr || !rcWndsAreaPadding.IsRectNull())
		p.m_pDrawManager->DrawWindowsAreaBack(this,&virtwnd,&p.m_rcWindows);
		// 
	if(p.m_bShowBorder)
		if(GetBorderWidth()>0)
		{	CRect rc;
			GetClientRect(&rc);
			p.m_pDrawManager->DrawBorder(this,&virtwnd,&rc);
		}
		// 
	p.m_pDrawManager->DrawEnd(this,&virtwnd);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::EqualTabsSize(bool equal)
{	p.m_bEqualTabsSize = equal;
}
// 
bool TabCtrl::IsEqualTabsSize() const
{	return p.m_bEqualTabsSize;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::EnableTabRemove(bool enable)
{	p.m_bEnableTabRemove = enable;
}
// 
bool TabCtrl::IsTabRemoveEnable() const
{	return p.m_bEnableTabRemove;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::HideSingleTab(bool hide)
{	p.m_bHideSingleTab = hide;
}
// 
bool TabCtrl::IsHideSingleTab() const
{	return p.m_bHideSingleTab;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::EnableToolTip(bool enable)
{	p.m_bToolTip = enable;
}
// 
bool TabCtrl::IsToolTipEnable() const
{	return p.m_bToolTip;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::ShowButtonsScroll(bool show)
{	p.m_bShowButtonsScroll = show;
}
// 
bool TabCtrl::IsButtonsScrollVisible() const
{	return p.m_bShowButtonsScroll;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::ShowButtonMenu(bool show)
{	p.m_bShowButtonMenu = show;
}
// 
bool TabCtrl::IsButtonMenuVisible() const
{	return p.m_bShowButtonMenu;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::ShowButtonClose(bool show)
{	p.m_bShowButtonClose = show;
}
// 
bool TabCtrl::IsButtonCloseVisible() const
{	return p.m_bShowButtonClose;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::SetFontNormal(CFont *font)
{	assert(font && font->m_hObject);
		// 
	LOGFONT logfont;
	font->GetLogFont(&logfont/*out*/);
	return SetFontNormal(&logfont);
}
//
void TabCtrl::SetFontNormalRef(CFont *font)
{	assert(font && font->m_hObject);
		// 
	if(p.m_FontNormal.m_hObject)
		p.m_FontNormal.DeleteObject();
	p.m_pFontNormalRef = font;
}
// 
bool TabCtrl::SetFontNormal(LOGFONT const *lf)
{	assert(lf);
		// 
	if(p.m_FontNormal.m_hObject)
		p.m_FontNormal.DeleteObject();
	p.m_pFontNormalRef = nullptr;
	if( !p.m_FontNormal.CreateFontIndirect(lf) )
		return false;
	p.m_pFontNormalRef = &p.m_FontNormal;
	return true;
}
// 
CFont *TabCtrl::GetFontNormal()
{	return p.m_pFontNormalRef;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::SetFontSelect(CFont *font)
{	assert(font && font->m_hObject);
		// 
	LOGFONT logfont;
	font->GetLogFont(&logfont/*out*/);
	return SetFontSelect(&logfont);
}
//
void TabCtrl::SetFontSelectRef(CFont *font)
{	assert(font && font->m_hObject);
		// 
	if(p.m_FontSelect.m_hObject)
		p.m_FontSelect.DeleteObject();
	p.m_pFontSelectRef = font;
}
// 
bool TabCtrl::SetFontSelect(LOGFONT const *lf)
{	assert(lf);
		// 
	if(p.m_FontSelect.m_hObject)
		p.m_FontSelect.DeleteObject();
	p.m_pFontSelectRef = nullptr;
	if( !p.m_FontSelect.CreateFontIndirect(lf) )
		return false;
	p.m_pFontSelectRef = &p.m_FontSelect;
	return true;
}
// 
CFont *TabCtrl::GetFontSelect()
{	return p.m_pFontSelectRef;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrl::GetNumberTabs() const
{	return static_cast<int>( p.m_tabs.size() );
}
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrl::GetTabHandleByIndex(int idx) const
{	assert(idx>=0 && idx<GetNumberTabs());
	return p.m_tabs[idx];
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrl::GetFirstEnableTab() const
{	for(Private::ci_tabs i=p.m_tabs.begin(), e=p.m_tabs.end(); i!=e; ++i)
		if(!(*i)->disable)
			return *i;
	return nullptr;
}
// 
TabCtrl::HTAB TabCtrl::GetPrevEnableTab(HTAB tab) const
{	for(Private::ci_tabs i=p.m_tabs.begin()+GetTabIndexByHandle(tab); i!=p.m_tabs.begin(); )
		if(!(*--i)->disable)
			return *i;
	return nullptr;
}
// 
TabCtrl::HTAB TabCtrl::GetNextEnableTab(HTAB tab) const
{	for(Private::ci_tabs i=p.m_tabs.begin()+GetTabIndexByHandle(tab)+1, e=p.m_tabs.end(); i!=e; ++i)
		if(!(*i)->disable)
			return *i;
	return nullptr;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SelectTab(HTAB tab)
{	assert( IsTabExist(tab) );
		// 
	if(p.m_hCurTab==tab || p.HandleToTab(tab)->disable)
		return;
		// 
	CRect rcWindows(p.m_rcWindows);
	rcWindows.DeflateRect( GetWindowsAreaPadding() );
	p.MoveChangedWindow(p.HandleToTab(tab)->wnd,&rcWindows,false);
	p.ShowWindow(p.HandleToTab(tab)->wnd,true);
	if(p.m_hCurTab)
		p.ShowWindow(p.HandleToTab(p.m_hCurTab)->wnd,false);
		// 
	p.m_hCurTab = tab;
}
// 
TabCtrl::HTAB TabCtrl::GetSelectedTab() const
{	return p.m_hCurTab;
}
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrl::GetTabUnderCursor() const
{	return (p.m_hHoverArea && !p.IsSystemButton(p.m_hHoverArea) ? p.m_hHoverArea : nullptr);
}
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrl::GetPushedTab() const
{	return (p.m_hPushedArea && !p.IsSystemButton(p.m_hPushedArea) ? p.m_hPushedArea : nullptr);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::DisableTab(HTAB tab, bool disable)
{	assert( IsTabExist(tab) );
	p.HandleToTab(tab)->disable = disable;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::IsTabDisabled(HTAB tab) const
{	assert( IsTabExist(tab) );
	return p.HandleToTab(tab)->disable;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::EnsureTabVisible(HTAB tab)
{	if(p.m_Behavior==BehaviorScroll)
	{	Private::Tab *i = p.HandleToTab(tab);
			// 
		if(i->rect.left < p.m_rcTabs.left)
			p.m_iTabsOffset -= p.m_rcTabs.left-i->rect.left;
		else if(i->rect.right > p.m_rcTabs.right)
			p.m_iTabsOffset += i->rect.right-p.m_rcTabs.right;
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::ScrollTabsToBegin()
{	if(p.m_Behavior==BehaviorScroll)
		p.m_iTabsOffset = 0;
}
// 
void TabCtrl::ScrollTabsToEnd()
{	if(p.m_Behavior==BehaviorScroll)
		p.m_iTabsOffset = p.m_iMaxTabsOffset;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::SetTabsScrollingStep(int step)
{	assert(step>=1);
	p.m_iScrollingStep = step;
}
// 
int TabCtrl::GetTabsScrollingStep() const
{	return p.m_iScrollingStep;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrl::HitTest(CPoint point) const
{	return p.m_pBehaviorManager->HitTest(const_cast<TabCtrl*>(this),&p,point);
}
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrl::GetTabIndexByHandle(HTAB tab) const
{	for(Private::ci_tabs i=p.m_tabs.begin(), e=p.m_tabs.end(); i!=e; ++i)
		if(*i==p.HandleToTab(tab))
			return static_cast<int>(i-p.m_tabs.begin());
	return -1;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::IsTabExist(HTAB tab) const
{	return GetTabIndexByHandle(tab)!=-1;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
CRect TabCtrl::GetTabRect(HTAB tab) const
{	assert( IsTabExist(tab) );
	return p.HandleToTab(tab)->rect;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::IsTabVisible(HTAB tab, bool *partially/*out,or null*/) const
{	assert( IsTabExist(tab) );
		// 
	CRect const &rc = p.HandleToTab(tab)->rect;
		// 
	const int tabRight = std::max(p.m_rcTabs.left,p.m_rcTabs.right);
	if(rc.right<=p.m_rcTabs.left || rc.left>=tabRight)
	{	if(partially)
			*partially = false;
		return false;
	}
	if(partially)
		*partially = (rc.left<p.m_rcTabs.left || rc.right>tabRight);
	return true;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrl::GetTabWithWindowID(int id) const
{	return p.GetTabWithWindowID(id,nullptr);
}
// 
TabCtrl::HTAB TabCtrl::Private::GetTabWithWindowID(int id, HTAB exceptTab) const
{	for(ci_tabs i=m_tabs.begin(), e=m_tabs.end(); i!=e; ++i)
		if(*i!=exceptTab && ::GetDlgCtrlID((*i)->wnd)==id)
			return *i;
	return nullptr;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::Private::GetTabAndIndex(int id, HTAB *tab/*out*/, int *idx/*out*/) const
{	for(ci_tabs i=m_tabs.begin(), e=m_tabs.end(); i!=e; ++i)
		if(::GetDlgCtrlID((*i)->wnd)==id)
		{	if(tab)
				*tab = *i;
			if(idx)
				*idx = static_cast<int>(i-m_tabs.begin());
			return true;
		}
	return false;
}
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrl::CompareTabsPosition(HTAB hTab1, HTAB hTab2) const
{	assert(IsTabExist(hTab1) && IsTabExist(hTab2));
		// 
	if(hTab1==hTab2)
		return 0;
		// 
	for(Private::ci_tabs i=p.m_tabs.begin(), e=p.m_tabs.end(); i!=e; ++i)
	{	if(*i==hTab1)
			return -1;
		if(*i==hTab2)
			return 1;
	}
	return -1;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrl::Private::GetBorderWidth(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return 1;
}
// 
CRect TabCtrl::Private::GetControlAreaPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	return (ctrl->GetLayout()==LayoutTop ? CRect(3,1,3,2/*indent*/) : CRect(3,2/*indent*/,3,1));
}
// 
CRect TabCtrl::Private::GetWindowsAreaPadding(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(0,0,0,0);
}
// 
CRect TabCtrl::Private::GetTabHorzMargin(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(0,0,0,0);
}
// 
CRect TabCtrl::Private::GetTabPadding(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(5,3,5,3);
}
// 
int TabCtrl::Private::GetTabImageTextSpace(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return 3;
}
// 
int TabCtrl::Private::GetTabExtraWidth(TabCtrl const * /*ctrl*/, IRecalc * /*base*/, HTAB /*tab*/)
{	return 0;
}
// 
int TabCtrl::Private::GetTabMinWidth(TabCtrl const *ctrl, IRecalc * /*base*/)
{	const CRect rcTabPadding = ctrl->GetTabPadding();
	CSize szImage, szImageDisabled;
	ctrl->GetImageSize(&szImage/*out*/,&szImageDisabled/*out*/);
	return rcTabPadding.left + std::max(szImage.cx,szImageDisabled.cx) + rcTabPadding.right;
}
// 
CRect TabCtrl::Private::GetButtonCloseHorzMargin(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(2,0,2,0);
}
// 
CRect TabCtrl::Private::GetButtonMenuHorzMargin(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(8,0,0,0);
}
// 
CRect TabCtrl::Private::GetButtonScrollLeftHorzMargin(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(8,0,0,0);
}
// 
CRect TabCtrl::Private::GetButtonScrollRightHorzMargin(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(0,0,0,0);
}
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrl::Private::HitTest(TabCtrl const *ctrl, IBehavior * /*base*/, CPoint point)   // get tab in the given point.
{	if( CRect(ctrl->GetTabsArea()).PtInRect(point) )
		for(int i=0, c=ctrl->GetNumberTabs(); i<c; ++i)
		{	HTAB tab = ctrl->GetTabHandleByIndex(i);
			const CRect rc = ctrl->GetTabRect(tab);
			if( rc.PtInRect(point) )
				return tab;
		}
	return nullptr;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::Private::SetCursor(TabCtrl const *ctrl, IBehavior * /*base*/)
{	HCURSOR hCursor = ctrl->GetCursor();
		// 
	if(hCursor)
	{	CPoint point;
		::GetCursorPos(&point);
		ctrl->ScreenToClient(&point);
			// 
		if( ctrl->HitTest(point) )
		{	::SetCursor(hCursor);
			return true;
		}
	}
	return false;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
CRect TabCtrl::GetCtrlArea() const { return p.m_rcCtrlArea; }
CRect TabCtrl::GetTabsArea() const { return p.m_rcTabs; }
CRect TabCtrl::GetWindowsArea() const { return p.m_rcWindows; }
// 
CRect TabCtrl::GetButtonCloseRect() const { return p.m_rcButtonClose; }
CRect TabCtrl::GetButtonMenuRect() const { return p.m_rcButtonMenu; }
CRect TabCtrl::GetButtonScrollLeftRect() const { return p.m_rcButtonScrollLeft; }
CRect TabCtrl::GetButtonScrollRightRect() const { return p.m_rcButtonScrollRight; }
// 
int TabCtrl::GetBorderWidth() const { return p.m_pRecalcManager->GetBorderWidth(this,&p); }
CRect TabCtrl::GetControlAreaPadding() const { return p.m_pRecalcManager->GetControlAreaPadding(this,&p); }
CRect TabCtrl::GetWindowsAreaPadding() const { return p.m_pRecalcManager->GetWindowsAreaPadding(this,&p); }
CRect TabCtrl::GetTabHorzMargin() const { return p.m_pRecalcManager->GetTabHorzMargin(this,&p); }
CRect TabCtrl::GetTabPadding() const { return p.m_pRecalcManager->GetTabPadding(this,&p); }
int TabCtrl::GetTabImageTextSpace() const { return p.m_pRecalcManager->GetTabImageTextSpace(this,&p); }
int TabCtrl::GetTabExtraWidth(HTAB tab) const { return p.m_pRecalcManager->GetTabExtraWidth(this,&p,tab); }
int TabCtrl::GetTabMinWidth() const { return p.m_pRecalcManager->GetTabMinWidth(this,&p); }
CRect TabCtrl::GetButtonCloseHorzMargin() const { return p.m_pRecalcManager->GetButtonCloseHorzMargin(this,&p); }
CRect TabCtrl::GetButtonMenuHorzMargin() const { return p.m_pRecalcManager->GetButtonMenuHorzMargin(this,&p); }
CRect TabCtrl::GetButtonScrollLeftHorzMargin() const { return p.m_pRecalcManager->GetButtonScrollLeftHorzMargin(this,&p); }
CRect TabCtrl::GetButtonScrollRightHorzMargin() const { return p.m_pRecalcManager->GetButtonScrollRightHorzMargin(this,&p); }
// 
CToolTipCtrl *TabCtrl::GetToolTip() const { return p.m_pToolTip; }
// 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnMouseMove(UINT nFlags, CPoint point)
{	CWnd::OnMouseMove(nFlags, point);
		// 
	p.AssignHoverArea(point);
		// 
	if(IsTabRemoveEnable() &&
		p.m_hPushedArea && !p.IsSystemButton(p.m_hPushedArea))
		if(!p.m_TabDrag.active)
		{	if( !CRect(p.m_TabDrag.ptStart-CSize(3,3),CSize(6,6)).PtInRect(point) )
				p.StartTabDragging(point);
		}
		else
		{	if( !p.m_rcCtrlArea.PtInRect(point) )
			{	if(p.m_pNotifyManager)
				{	CPoint pt(point);
					ClientToScreen(&pt);
					p.m_pNotifyManager->OnDrag(this,p.m_hPushedArea,pt,true);
				}
			}
			else
			{	if(p.m_hHoverArea && !p.IsSystemButton(p.m_hHoverArea) && p.m_hPushedArea!=p.m_hHoverArea)
				{	if(GetTabIndexByHandle(p.m_hHoverArea) < GetTabIndexByHandle(p.m_hPushedArea))
					{	CRect rc(p.HandleToTab(p.m_hHoverArea)->rect);
						rc.right = rc.left + p.HandleToTab(p.m_hPushedArea)->rect.Width();
							// 
						if( rc.PtInRect(point) )
						{	RemoveTabBefore(p.m_hHoverArea,p.m_hPushedArea);
							p.Recalc(true);
							p.AssignHoverArea(point);
							Invalidate(FALSE);
						}
					}
					else
					{	CRect rc(p.HandleToTab(p.m_hHoverArea)->rect);
						rc.left = rc.right - p.HandleToTab(p.m_hPushedArea)->rect.Width();
							// 
						if( rc.PtInRect(point) )
						{	RemoveTabAfter(p.m_hHoverArea,p.m_hPushedArea);
							p.Recalc(true);
							p.AssignHoverArea(point);
							Invalidate(FALSE);
						}
					}
				}
					// 
				if(p.m_pNotifyManager)
				{	CPoint pt(point);
					ClientToScreen(&pt);
					p.m_pNotifyManager->OnDrag(this,p.m_hPushedArea,pt,false);
				}
			}
		}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::AssignHoverArea(CPoint point)
{	HTAB hHoverAreaOld = m_hHoverArea;
	m_hHoverArea = nullptr;
		// 
	if(m_hCurTab && m_rcButtonClose.PtInRect(point))
		m_hHoverArea = HANDLE_BUT_CLOSE;
	else if( m_rcButtonMenu.PtInRect(point) )
		m_hHoverArea = HANDLE_BUT_MENU;
	else if( m_rcButtonScrollLeft.PtInRect(point) )
		m_hHoverArea = HANDLE_BUT_SCROLLLEFT;
	else if( m_rcButtonScrollRight.PtInRect(point) )
		m_hHoverArea = HANDLE_BUT_SCROLLRIGHT;
		// 
	if(m_hHoverArea==nullptr)
		m_hHoverArea = o.HitTest(point);
		// 
	if(hHoverAreaOld!=m_hHoverArea)
		::RedrawWindow(o.m_hWnd,nullptr,nullptr,RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN);   // !!! not use Invalidate(...) - can be artifactes while tracking tab.
		// 
	TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT),TME_LEAVE,o.m_hWnd,0 };
	::TrackMouseEvent(&tme);
}
/////////////////////////////////////////////////////////////////////////////
// 
LRESULT TabCtrl::OnMouseLeave(WPARAM wp, LPARAM lp)
{	if(p.m_hHoverArea)
	{	p.m_hHoverArea = nullptr;
		Invalidate(FALSE);
	}
		// 
	return CWnd::DefWindowProc(WM_MOUSELEAVE,wp,lp);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{	p.LButtonDown(point);
	p.SetFocusInChildWnd();
		// 
	CWnd::OnLButtonDown(nFlags, point);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{	p.LButtonDown(point);
		// 
	CWnd::OnLButtonDblClk(nFlags, point);
		// 
	if(p.m_hHoverArea && !p.IsSystemButton(p.m_hHoverArea))
		if(p.m_pNotifyManager)
		{	CPoint pt(point);
			ClientToScreen(&pt);
			p.m_pNotifyManager->OnLButtonDblClk(this,p.m_hHoverArea,pt);
		}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::LButtonDown(CPoint point)
{	if(m_hHoverArea)
	{	if( !IsSystemButton(m_hHoverArea) )   // this is tab.
		{	if( o.IsTabRemoveEnable() )
			{	m_TabDrag.ptStart = point;
				m_hPushedArea = m_hHoverArea;
				o.SetCapture();
			}
				// 
			const bool select = (!o.IsTabDisabled(m_hHoverArea) && o.GetSelectedTab()!=m_hHoverArea);
				// 
			if(m_pNotifyManager)
			{	CPoint pt(point);
				o.ClientToScreen(&pt);
				m_pNotifyManager->OnLButtonDown(&o,m_hHoverArea,pt);
			}
				// 
			if(select)
			{	o.SelectTab(m_hHoverArea);
				Recalc(true);
				if(m_pNotifyManager)
					m_pNotifyManager->OnTabSelected(&o,m_hHoverArea);
			}
		}
		else
		{	m_hPushedArea = m_hHoverArea;
				// 
			if(m_hPushedArea==HANDLE_BUT_SCROLLLEFT)
			{	StepLeft();
				o.SetTimer(TimerIdScrollLeftClick,300,nullptr);
				Recalc(true);
			}
			else if(m_hPushedArea==HANDLE_BUT_SCROLLRIGHT)
			{	StepRight();
				o.SetTimer(TimerIdScrollRightClick,300,nullptr);
				Recalc(true);
			}
				// 
			o.SetCapture();
		}
	}
		// 
	o.Invalidate(FALSE);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{	CWnd::OnLButtonUp(nFlags, point);
		// 
	if(p.m_hPushedArea)
	{	const bool close = (p.m_hPushedArea==Private::HANDLE_BUT_CLOSE && p.m_rcButtonClose.PtInRect(point));
		const bool menu = (p.m_hPushedArea==Private::HANDLE_BUT_MENU && p.m_rcButtonMenu.PtInRect(point));
			// 
		p.StopScrolling();
		Invalidate(FALSE);
			// 
		bool alive = true;
		p.m_pLifeStatus = &alive;
		p.StopTabDragging(false);
			// 
		if(alive)
			if(p.m_pNotifyManager)
				if(close)
				{	CPoint pt(point);
					ClientToScreen(&pt);
						// 
					p.m_hPushedArea = Private::HANDLE_BUT_CLOSE;
					p.m_pNotifyManager->OnButtonCloseClicked(this,&p.m_rcButtonClose,pt);
						// 
					if(alive)
					{	p.m_hPushedArea = nullptr;
						Invalidate(FALSE);
					}
				}
				else if(menu)
				{	CPoint pt(point);
					ClientToScreen(&pt);
						// 
					p.m_hPushedArea = Private::HANDLE_BUT_MENU;
					p.m_pNotifyManager->OnButtonMenuClicked(this,&p.m_rcButtonMenu,pt);
						// 
					if(alive)
					{	p.m_hPushedArea = nullptr;
						Invalidate(FALSE);
					}
				}
			//
		if(alive)
			p.m_pLifeStatus = nullptr;
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{	p.SetFocusInChildWnd();
		// 
	if(p.m_hHoverArea==nullptr || !p.IsSystemButton(p.m_hHoverArea))
		if(p.m_pNotifyManager)
		{	CPoint pt(point);
			ClientToScreen(&pt);
			p.m_pNotifyManager->OnRButtonDown(this,p.m_hHoverArea,pt);
		}
		// 
	CWnd::OnRButtonDown(nFlags, point);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{	CWnd::OnRButtonUp(nFlags, point);
		// 
	if(p.m_hHoverArea==nullptr || !p.IsSystemButton(p.m_hHoverArea))
		if(p.m_pNotifyManager)
		{	CPoint pt(point);
			ClientToScreen(&pt);
			p.m_pNotifyManager->OnRButtonUp(this,p.m_hHoverArea,pt);
		}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnMButtonDown(UINT nFlags, CPoint point)
{	p.SetFocusInChildWnd();
		// 
	CWnd::OnMButtonDown(nFlags, point);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnCaptureChanged(CWnd *pWnd)
{	if(pWnd!=this)
	{	p.StopScrolling();
		p.StopTabDragging(true);
		Invalidate(FALSE);
	}
		// 
	CWnd::OnCaptureChanged(pWnd);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::StopScrolling()
{	if(m_hPushedArea && IsSystemButton(m_hPushedArea))
	{	if(m_hPushedArea==HANDLE_BUT_SCROLLLEFT)
		{	o.KillTimer(TimerIdScrollLeftClick);
			o.KillTimer(TimerIdScrollLeftScrolling);
		}
		else if(m_hPushedArea==HANDLE_BUT_SCROLLRIGHT)
		{	o.KillTimer(TimerIdScrollRightClick);
			o.KillTimer(TimerIdScrollRightScrolling);
		}
			// 
		m_hPushedArea = nullptr;
		if(::GetCapture()==o.m_hWnd)
			::ReleaseCapture();
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::StartTabDragging(CPoint point)
{	m_TabDrag.active = true;
		// 
	m_KeyboardHook.Add(this,&Private::OnKeyDown);
		// 
	m_TabDrag.tabsBefore = m_tabs;
		// 
	if(m_pNotifyManager)
	{	CPoint pt(point);
		o.ClientToScreen(&pt);
		m_pNotifyManager->OnStartDrag(&o,m_hPushedArea,pt);
	}
}
// 
void TabCtrl::Private::StopTabDragging(bool cancel)
{	if(m_hPushedArea && !IsSystemButton(m_hPushedArea))
	{	HTAB hPushedArea = m_hPushedArea;
			// 
		m_hPushedArea = nullptr;
		if(::GetCapture()==o.m_hWnd)
			::ReleaseCapture();
			// 
		if(m_TabDrag.active)
		{	m_TabDrag.active = false;
				// 
			m_KeyboardHook.Delete(this);
				// 
			if(cancel && !m_TabDrag.tabsBefore.empty())
			{	m_tabs.swap(m_TabDrag.tabsBefore);
				Recalc(true);
			}
				// 
			if(m_pNotifyManager)
				m_pNotifyManager->OnFinishDrag(&o,hPushedArea,cancel);
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::Private::OnKeyDown(UINT keyCode, UINT /*msgFlag*/)
{	if(keyCode==VK_ESCAPE)
		if( o.IsTabDragging() )
		{	o.CancelTabDragging();
			return false;
		}
	return true;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::IsTabDragging() const
{	return p.m_TabDrag.active;
}
// 
void TabCtrl::CancelTabDragging()
{	p.StopTabDragging(true);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnTimer(UINT_PTR nIDEvent)
{	switch(nIDEvent)
	{	case Private::TimerIdScrollLeftClick:
			KillTimer(Private::TimerIdScrollLeftClick);
			if(p.m_hPushedArea==Private::HANDLE_BUT_SCROLLLEFT)
				SetTimer(Private::TimerIdScrollLeftScrolling,20,nullptr);
			break;
		case Private::TimerIdScrollLeftScrolling:
			if(p.m_hHoverArea==Private::HANDLE_BUT_SCROLLLEFT)
			{	p.StepLeft();
				Update();
			}
			break;
			// 
		case Private::TimerIdScrollRightClick:
			KillTimer(Private::TimerIdScrollRightClick);
			if(p.m_hPushedArea==Private::HANDLE_BUT_SCROLLRIGHT)
				SetTimer(Private::TimerIdScrollRightScrolling,20,nullptr);
			break;
		case Private::TimerIdScrollRightScrolling:
			if(p.m_hHoverArea==Private::HANDLE_BUT_SCROLLRIGHT)
			{	p.StepRight();
				Update();
			}
			break;
	}
		// 
	CWnd::OnTimer(nIDEvent);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::StepLeft()
{	m_iTabsOffset -= m_iScrollingStep;
}
// 
void TabCtrl::Private::StepRight()
{	m_iTabsOffset += m_iScrollingStep;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::OnSetFocus(CWnd* pOldWnd)
{	CWnd::OnSetFocus(pOldWnd);
		// 
	p.SetFocusInChildWnd();
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::SetFocusInChildWnd()
{	HTAB tab = o.GetSelectedTab();
	if(tab)
	{	HWND wnd = o.GetTabWindow(tab);
		if(wnd && ::IsWindow(wnd) && ::GetFocus()!=wnd)
			::SetFocus(wnd);
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
BOOL TabCtrl::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message)
{	if( p.m_pBehaviorManager->SetCursor(this,&p) )
		return TRUE;
		// 
	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::Private::IsSystemButton(HTAB tab) const
{	return tab==HANDLE_BUT_CLOSE || tab==HANDLE_BUT_MENU || tab==HANDLE_BUT_SCROLLLEFT || tab==HANDLE_BUT_SCROLLRIGHT;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::SaveState(CWinApp *app, TCHAR const *section, TCHAR const *entry) const
{	assert(app && section && entry);
		// 
	CMemFile file;
	CArchive ar(&file,CArchive::store);
	if( !SaveState(&ar) )
		return false;
	ar.Flush();
	ar.Close();
		// 
	const UINT uDataSize = static_cast<UINT>(file.GetLength());
	BYTE *pData = file.Detach();
	const bool res = app->WriteProfileBinary(section,entry,pData,uDataSize)!=0;
	free(pData);
	return res;
}
// 
bool TabCtrl::LoadState(CWinApp *app, TCHAR const *section, TCHAR const *entry)
{	assert(app && section && entry);
		//
	bool res = false;
	BYTE *pData = nullptr;
	UINT uDataSize;
		// 
	try
	{	if( app->GetProfileBinary(section,entry,&pData,&uDataSize) )
		{	CMemFile file(pData,uDataSize);
			CArchive ar(&file,CArchive::load);
			res = LoadState(&ar);
		}
	}
	catch(CMemoryException* pEx)
	{	pEx->Delete();
	}
	if(pData)
		delete [] pData;
		// 
	return res;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::LoadState(CArchive *ar)
{	try
	{	p.LoadStateInner(ar);
		return true;
	}
	catch(CMemoryException* pEx)
	{	pEx->Delete();
	}
	catch(CArchiveException* pEx)
	{	pEx->Delete();
	}
	catch(...)
	{
	}
	return false;
}
// 
bool TabCtrl::SaveState(CArchive *ar) const
{	try
	{	p.SaveStateInner(ar);
		return true;
	}
	catch(CMemoryException* pEx)
	{	pEx->Delete();
	}
	catch(CArchiveException* pEx)
	{	pEx->Delete();
	}
	catch(...)
	{
	}
	return false;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::Private::LoadStateInner(CArchive *ar)
{	int count;
	*ar >> count;   // count tabs.
		// 
	assert(count==o.GetNumberTabs());   // changed count of tabs.
		// 
	int select = -1;
	if(count>1)
		*ar >> select;   // index of selected tab.
		// 
	int id, target;
	HTAB tab;
		// 
	for(int i=0; i<count; ++i)
	{	*ar >> id;
			// 
		const bool found = GetTabAndIndex(id,&tab,&target);
			// 
		if(!found)
		{	TRACE("Not found tab with control id=%d (TabCtrl::LoadStateInner)",id);
			return false;
		}
			// 
		if(i!=count-1)
		{	m_tabs.insert(m_tabs.begin()+i, HandleToTab(tab) );
			m_tabs.erase(m_tabs.begin()+(++target));
		}
	}
		// 
	if(select!=-1)
		o.SelectTab( o.GetTabHandleByIndex(select) );
		// 
	return true;
}
// 
void TabCtrl::Private::SaveStateInner(CArchive *ar) const
{	const int count = o.GetNumberTabs();
	*ar << count;
		// 
	if(count>1)
	{	HTAB hSelTab = o.GetSelectedTab();
		*ar << (hSelTab ? o.GetTabIndexByHandle(hSelTab) : -1);
	}
		// 
	for(Private::ci_tabs i=m_tabs.begin(), e=m_tabs.end(); i!=e; ++i)
		*ar << ::GetDlgCtrlID((*i)->wnd);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::ShowWindow(HWND wnd, bool show)
{	(show ? ::ShowWindow(wnd,SW_SHOWNA) : ::ShowWindow(wnd,SW_HIDE));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrl::Private::MoveChangedWindow(HWND wnd, CRect const *rcNew, bool redraw)
{	CRect rcOld;
	::GetWindowRect(wnd,&rcOld/*out*/);
	::MapWindowPoints(HWND_DESKTOP,o.m_hWnd,reinterpret_cast<POINT *>(&rcOld),2);
	if(*rcNew!=rcOld)
		::MoveWindow(wnd,rcNew->left,rcNew->top,rcNew->Width(),rcNew->Height(),redraw);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::Private::LoadImage(HMODULE moduleRes/*or null*/, UINT resID, bool pngImage, Gdiplus::Bitmap **bmp/*out*/) const
{	assert(resID);
	assert(bmp);
		// 
	*bmp = nullptr;
		// 
	if(!moduleRes)
		moduleRes = AfxFindResourceHandle(MAKEINTRESOURCE(resID),(pngImage ? _T("PNG") : RT_BITMAP));
	if(moduleRes)
	{	if(!pngImage)   // bmp.
			*bmp = ::new (std::nothrow) Gdiplus::Bitmap(moduleRes,MAKEINTRESOURCEW(resID));
		else   // png.
		{	HRSRC hRsrc = ::FindResource(moduleRes,MAKEINTRESOURCE(resID),_T("PNG"));
			if(hRsrc)
			{	HGLOBAL hGlobal = ::LoadResource(moduleRes,hRsrc);
				if(hGlobal)
				{	const void *lpBuffer = ::LockResource(hGlobal);
					if(lpBuffer)
					{	const UINT uiSize = static_cast<UINT>( ::SizeofResource(moduleRes,hRsrc) );
						HGLOBAL hRes = ::GlobalAlloc(GMEM_MOVEABLE, uiSize);
						if(hRes)
						{	void *lpResBuffer = ::GlobalLock(hRes);
							if(lpResBuffer)
							{	memcpy(lpResBuffer, lpBuffer, uiSize);
								IStream *pStream = nullptr;
								if(::CreateStreamOnHGlobal(hRes, FALSE, &pStream/*out*/)==S_OK)
								{	*bmp = ::new (std::nothrow) Gdiplus::Bitmap(pStream,FALSE);
									pStream->Release();
								}
								::GlobalUnlock(lpResBuffer);
							}
							::GlobalFree(hRes);
						}
						::UnlockResource(hGlobal);
					}
					::FreeResource(hGlobal);
				}
			}
		}
	}
	if(*bmp && (*bmp)->GetLastStatus()!=Gdiplus::Ok)
	{	::delete *bmp;
		return false;
	}
	return (*bmp)!=nullptr;
}
/////////////////////////////////////////////////////////////////////////////
// 
bool TabCtrl::Private::CreateImageList(Gdiplus::Bitmap *bmp, int imageWidth, 
	COLORREF clrMask/*or CLR_NONE*/, COLORREF clrBack/*or CLR_NONE*/, CImageList *imageList/*out*/) const
{
	assert(bmp);
	assert(imageWidth>0);
	assert(imageList);
		// 
	if(imageList->m_hImageList)
		imageList->DeleteImageList();
		// 
	bool res = false;
	const Gdiplus::Rect rect(0,0,bmp->GetWidth(),bmp->GetHeight());
	if( imageList->Create(imageWidth,bmp->GetHeight(),ILC_COLOR24 | ILC_MASK,1,0) )
	{	Gdiplus::Bitmap *bmpCnvrt = bmp->Clone(rect,PixelFormat32bppARGB);
		if(bmpCnvrt)
		{	if(bmpCnvrt->GetLastStatus()==Gdiplus::Ok)
			{	Gdiplus::BitmapData data;
				if(bmpCnvrt->LockBits(&rect,Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,PixelFormat32bppARGB,&data)==Gdiplus::Ok)
				{	CBitmap cbmp;
					if( cbmp.CreateBitmap(rect.Width,rect.Height,1,32,nullptr) )
					{	const UINT maskRGB = (clrMask & 0x0000ff00) | (clrMask & 0xff)<<16 | (clrMask & 0x00ff0000)>>16;
						const UINT number = data.Width * data.Height;
						UINT32 *ptr = static_cast<UINT32 *>(data.Scan0);
						for(UINT32 *e=ptr+number; ptr!=e; ++ptr)
						{	const unsigned char a = static_cast<unsigned char>(*ptr >> 24);
							if(a==0)
								*ptr = maskRGB;
							else if(a==255)
							{	if(clrMask!=CLR_NONE)
									if((*ptr & 0x00ffffff)==maskRGB)
										*ptr = maskRGB;
							}
							else   // a!=255.
								if(clrBack!=CLR_NONE)
								{	const UINT _a = 255u - a;
									const UINT r = ((*ptr & 0xff) * a + (clrBack>>16 & 0xff) * _a) / 255u;
									const UINT g = ((*ptr>>8 & 0xff) * a + (clrBack>>8 & 0xff) * _a) / 255u;
									const UINT b = ((*ptr>>16 & 0xff) * a + (clrBack & 0xff) * _a) / 255u;
									*ptr = r | (g<<8) | (b<<16);
								}
						}
						cbmp.SetBitmapBits(number*4,data.Scan0);
							// 
						res = imageList->Add(&cbmp,clrMask & 0x00ffffff)!=-1;
					}
					bmpCnvrt->UnlockBits(&data);
				}
			}
			delete bmpCnvrt;
		}
	}
	if(!res && imageList->m_hImageList)
		imageList->DeleteImageList();
	return res;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_base.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::Install(TabCtrl *ctrl)
{	ctrl->SetDrawManager(this);
	ctrl->SetRecalcManager(this);
	ctrl->SetBehaviorManager(this);
	ctrl->SetToolTipManager(this);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawBorder(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	DrawRect(dc,rect, GetBorderColor(ctrl) );
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	CRect rc(*rect);
		// 
	if(ctrl->GetLayout()==TabCtrl::LayoutTop)
	{		// draw background for tabs area.
		rc.bottom -= ctrl->GetControlAreaPadding().bottom;
		FillSolidRect(dc,&rc, GetCtrlAreaBackColor(ctrl) );
			// draw separator.
		DrawLine(dc,rect->left,rc.bottom-1,rect->right,rc.bottom-1, GetTabBorderColor(ctrl) );
			// draw background for windows area.
		rc.top = rc.bottom;
		rc.bottom = rect->bottom;
		if( !rc.IsRectEmpty() )
			FillSolidRect(dc,&rc, GetChildWndBackColor(ctrl) );
	}
	else
	{		// draw background for windows area.
		rc.bottom = rc.top + ctrl->GetControlAreaPadding().top;
		if( !rc.IsRectEmpty() )
			FillSolidRect(dc,&rc, GetChildWndBackColor(ctrl) );
			// draw separator.
		DrawLine(dc,rect->left,rc.bottom,rect->right,rc.bottom, GetTabBorderColor(ctrl) );
			// draw background for tabs area.
		rc.top = rc.bottom+1;
		rc.bottom = rect->bottom;
		FillSolidRect(dc,&rc, GetCtrlAreaBackColor(ctrl) );
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
		FillSolidRect(dc,rect, GetEmptyWndsAreaBackColor(ctrl) );
	else
		FillSolidRect(dc,rect, GetTabSelectedBackColor(ctrl) );
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawTab(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRgn *rgn)
{	const CRect rc = ctrl->GetTabRect(tab);
		// draw background.
	DrawTabBack(ctrl,dc,tab,&rc,rgn);
		// draw image and text;
	DrawTabContext(ctrl,dc,tab,&rc,rgn);
}
/////////////////////////////////////////////////////////////////////////////
// Draw image and text.
// 
void TabCtrlStyle_base::DrawTabContext(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn *rgn)
{	CRect rcTabPadding = ctrl->GetTabPadding();
		// 
	CRect rc(rect);
	rc.DeflateRect(&rcTabPadding);
		// 
	const bool disable = ctrl->IsTabDisabled(tab);
	const CString text = ctrl->GetTabText(tab);
	const int textWidth = dc->GetTextExtent(text).cx;
		// 
	Gdiplus::Bitmap *images;
	(!disable ? ctrl->GetImages(&images/*out*/,nullptr) : ctrl->GetImages(nullptr,&images/*out*/));
		// 
		// draw image.
	if(ctrl->GetTabImage(tab)>-1 && images)
	{	CSize szImage;
		(!disable ? ctrl->GetImageSize(&szImage,nullptr) : ctrl->GetImageSize(nullptr,&szImage));
			// 
		const int iContentWidth = szImage.cx + ctrl->GetTabImageTextSpace() + textWidth;
		rc.left += (iContentWidth<rc.Width() ? (rc.Width()-iContentWidth)/2 : 0);
			// 
		DrawTabImage(ctrl,dc,tab,&rc,rgn);
			// 
		rc.left += szImage.cx + ctrl->GetTabImageTextSpace();
	}
	else
		rc.left += (textWidth<rc.Width() ? (rc.Width()-textWidth)/2 : 0);
		// 
		// draw text.
	if( !rc.IsRectEmpty() )
		DrawTabText(ctrl,dc,tab,&rc,rgn);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	const COLORREF clrBorder = GetTabBorderColor(ctrl);
	const COLORREF clrBack = GetTabSelectedBackColor(ctrl);
		// 
	if(ctrl->GetSelectedTab()!=tab)
	{	if(ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab))   // highlighted tab.
		{	CRect rc(rect);
				// 
			if(ctrl->GetLayout()==TabCtrl::LayoutTop)
			{	rc.DeflateRect(2,1,1,1);
				DrawHalfRoundFrame(dc,&rc,SideTop,1,clrBorder,clrBack);
				rc.DeflateRect(1,rc.Height()/2,1,0);
				DrawGradient(dc,&rc,false,clrBack,clrBorder);
			}
			else
			{	rc.DeflateRect(2,1,1,1);
				DrawHalfRoundFrame(dc,&rc,SideBottom,1,clrBorder,clrBack);
			}
		}
			// 
		const CRect rcTabPadding = ctrl->GetTabPadding();
		int topMargin = rcTabPadding.top + 1;
		int bottomMargin = rcTabPadding.bottom + 1;
		(ctrl->GetLayout()==TabCtrl::LayoutTop ? bottomMargin+=1/*separator*/ : topMargin+=1/*separator*/);
			// 
		if(tab==ctrl->GetTabHandleByIndex(0))   // this is first tab.
			DrawLine(dc,rect->left,rect->top+topMargin,rect->left,rect->bottom-bottomMargin,clrBorder);
		DrawLine(dc,rect->right,rect->top+topMargin,rect->right,rect->bottom-bottomMargin,clrBorder);
	}
	else
		DrawHalfRoundFrame(dc,rect,
			(ctrl->GetLayout()==TabCtrl::LayoutTop ? SideTop : SideBottom),2,clrBorder,clrBack);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawTabImage(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	Gdiplus::Bitmap *images;
	CSize szImage;
		// 
	if( !ctrl->IsTabDisabled(tab) )
	{	ctrl->GetImages(&images/*out*/,nullptr);
		ctrl->GetImageSize(&szImage/*out*/,nullptr);
	}
	else
	{	ctrl->GetImages(nullptr,&images/*out*/);
		ctrl->GetImageSize(nullptr,&szImage/*out*/);
	}
	const CPoint pt(rect->left,(rect->top+rect->bottom-szImage.cy+1)/2);
	const int image = ctrl->GetTabImage(tab);
	DrawImage(ctrl,dc,images,pt, image, szImage, ctrl->GetImagesTranspColor());
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawTabText(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	const CString text = ctrl->GetTabText(tab);
	dc->SetTextColor( GetTabTextColor(ctrl,tab) );
	dc->DrawText(text,const_cast<CRect *>(rect),DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
	{	FillSolidRect(dc,rect,MixingColors(::GetSysColor(COLOR_HIGHLIGHT),::GetSysColor(COLOR_WINDOW),30));   // it is approximate color (VisualStudio uses some another way).
		DrawRect(dc,rect, ::GetSysColor(COLOR_HIGHLIGHT) );
	}
	else if(hover && pushed)
	{	FillSolidRect(dc,rect,MixingColors(::GetSysColor(COLOR_HIGHLIGHT),::GetSysColor(COLOR_WINDOW),22));   // it is approximate color (VisualStudio uses some another way).
		DrawRect(dc,rect, ::GetSysColor(COLOR_HIGHLIGHT) );
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawButtonClose(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	DrawButtonFrame(ctrl,dc,rect,hover,pushed);
		// 
	Gdiplus::Bitmap *images = ctrl->GetSystemImages();
	const COLORREF clrTransp = ctrl->GetSystemImagesTranspColor();
	DrawMarker(ctrl, dc, images, rect, TabCtrl::SysImageButtonClose, clrTransp, GetButtonCloseColor(ctrl,hover,pushed));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawButtonMenu(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	DrawButtonFrame(ctrl,dc,rect,hover,pushed);
		// 
	Gdiplus::Bitmap *images = ctrl->GetSystemImages();
	const COLORREF clrTransp = ctrl->GetSystemImagesTranspColor();
	DrawMarker(ctrl, dc, images, rect, (!partialView ? TabCtrl::SysImageButtonMenuFullView : TabCtrl::SysImageButtonMenuPartialView),
		clrTransp, GetButtonMenuColor(ctrl,hover,pushed));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawButtonScrollLeft(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	DrawButtonFrame(ctrl,dc,rect,hover,pushed);
		// 
	Gdiplus::Bitmap *images = ctrl->GetSystemImages();
	const COLORREF clrTransp = ctrl->GetSystemImagesTranspColor();
	DrawMarker(ctrl, dc, images, rect, (!partialView ? TabCtrl::SysImageButtonScrollLeftForbid : TabCtrl::SysImageButtonScrollLeftAllow),
		clrTransp, GetButtonScrollLeftColor(ctrl,hover,pushed));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawButtonScrollRight(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	DrawButtonFrame(ctrl,dc,rect,hover,pushed);
		// 
	Gdiplus::Bitmap *images = ctrl->GetSystemImages();
	const COLORREF clrTransp = ctrl->GetSystemImagesTranspColor();
	DrawMarker(ctrl, dc, images, rect, (!partialView ? TabCtrl::SysImageButtonScrollRightForbid : TabCtrl::SysImageButtonScrollRightAllow),
		clrTransp, GetButtonScrollRightColor(ctrl,hover,pushed));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
CToolTipCtrl *TabCtrlStyle_base::CreateToolTip(TabCtrl *ctrl)
{
	#ifdef AFX_TOOLTIP_TYPE_ALL   // for MFC Feature Pack.
		CToolTipCtrl *tooltip = nullptr;
		return (CTooltipManager::CreateToolTip(tooltip/*out*/,ctrl,AFX_TOOLTIP_TYPE_TAB) ? tooltip : nullptr);
	#else
		CToolTipCtrl *toolTip = nullptr;
		try
		{	toolTip = new CToolTipCtrl;
		}
		catch(std::bad_alloc &)
		{	return nullptr;
		}
		if( !toolTip->Create(ctrl,TTS_ALWAYSTIP) )
		{	delete toolTip;
			return nullptr;
		}
			// 
		DWORD dwClassStyle = ::GetClassLong(toolTip->m_hWnd,GCL_STYLE);
		dwClassStyle |= CS_DROPSHADOW;   // enables the drop shadow effect.
		::SetClassLong(toolTip->m_hWnd,GCL_STYLE,dwClassStyle);
		return toolTip;
	#endif
}
// 
void TabCtrlStyle_base::DestroyToolTip(CToolTipCtrl *tooltip)
{	
	#ifdef AFX_TOOLTIP_TYPE_ALL   // for MFC Feature Pack.
		CTooltipManager::DeleteToolTip(tooltip);
	#else
		delete tooltip;
	#endif
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_base::GetBorderColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_BTNSHADOW);
}
// 
COLORREF TabCtrlStyle_base::GetTabBorderColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_BTNSHADOW);
}
// 
COLORREF TabCtrlStyle_base::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_BTNFACE);
}
// 
COLORREF TabCtrlStyle_base::GetWndsAreaBackColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_BTNFACE);
}
// 
COLORREF TabCtrlStyle_base::GetTabSelectedBackColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_WINDOW);
}
// 
COLORREF TabCtrlStyle_base::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	return (!ctrl->IsTabDisabled(tab) ? ::GetSysColor(COLOR_BTNTEXT) : ::GetSysColor(COLOR_GRAYTEXT));
}
// 
COLORREF TabCtrlStyle_base::GetButtonCloseColor(TabCtrl const *ctrl, bool /*hover*/, bool /*pushed*/)
{	return (ctrl->GetSelectedTab() ? ::GetSysColor(COLOR_BTNTEXT) : ::GetSysColor(COLOR_GRAYTEXT));
}
COLORREF TabCtrlStyle_base::GetButtonMenuColor(TabCtrl const * /*ctrl*/, bool /*hover*/, bool /*pushed*/)
{	return ::GetSysColor(COLOR_BTNTEXT);
}
COLORREF TabCtrlStyle_base::GetButtonScrollLeftColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return GetButtonMenuColor(ctrl,hover,pushed);
}
COLORREF TabCtrlStyle_base::GetButtonScrollRightColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return GetButtonMenuColor(ctrl,hover,pushed);
}
// 
COLORREF TabCtrlStyle_base::GetChildWndBackColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_WINDOW);
}
// 
COLORREF TabCtrlStyle_base::GetEmptyWndsAreaBackColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_APPWORKSPACE);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_base
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawMarker(TabCtrl const * /*ctrl*/, CDC *dc, Gdiplus::Bitmap *bmp, CRect const *rcDst, int image, COLORREF clrTransp, COLORREF clrFill)
{	Gdiplus::ImageAttributes att;
	if(clrTransp==CLR_NONE)
	{	static Gdiplus::ColorMap map = {0xff000000,0};
		map.newColor.SetValue(0xff000000 | (clrFill & 0xff)<<16 | (clrFill & 0x0000ff00) | (clrFill & 0x00ff0000)>>16);
		att.SetRemapTable(1, &map, Gdiplus::ColorAdjustTypeBitmap);
	}
	else
	{	static Gdiplus::ColorMap map[2] = {0,0, 0xff000000,0};
		map[0].oldColor.SetValue(0xff000000 | (clrTransp & 0xff)<<16 | (clrTransp & 0x0000ff00) | (clrTransp & 0x00ff0000)>>16);
		map[1].newColor.SetValue(0xff000000 | (clrFill & 0xff)<<16 | (clrFill & 0x0000ff00) | (clrFill & 0x00ff0000)>>16);
		att.SetRemapTable(2, map, Gdiplus::ColorAdjustTypeBitmap);
	}
	const Gdiplus::Rect rect(rcDst->left,rcDst->top,rcDst->Width(),rcDst->Height());
	Gdiplus::Graphics gr(dc->m_hDC);
	gr.DrawImage(bmp, rect, rect.Width*image,0,rect.Width,rect.Height, Gdiplus::UnitPixel, &att);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawImage(TabCtrl const * /*ctrl*/, CDC *dc, Gdiplus::Bitmap *bmp, CPoint const &ptDst, int image, CSize const &szSrc, COLORREF clrTransp)
{	Gdiplus::Graphics gr(dc->m_hDC);
	if(clrTransp==CLR_NONE)
		gr.DrawImage(bmp, ptDst.x,ptDst.y, image*szSrc.cx,0,szSrc.cx,szSrc.cy, Gdiplus::UnitPixel);
	else   // draw with color key.
	{	const Gdiplus::Rect rcDest(ptDst.x,ptDst.y,szSrc.cx,szSrc.cy);
		const Gdiplus::Color clrTr(GetRValue(clrTransp),GetGValue(clrTransp),GetBValue(clrTransp));
		Gdiplus::ImageAttributes att;
		att.SetColorKey(clrTr,clrTr,Gdiplus::ColorAdjustTypeBitmap);
		gr.DrawImage(bmp, rcDest, image*szSrc.cx,0,szSrc.cx,szSrc.cy, Gdiplus::UnitPixel, &att);
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawHalfRoundFrame(CDC *pDC, CRect const *rect, Side side, int radius, COLORREF clrBorder, COLORREF clrBack)
{	POINT pts[6];
	if(side==SideTop)
	{	pts[0].x = rect->left; pts[0].y = rect->bottom-1;
		pts[1].x = rect->left; pts[1].y = rect->top+radius;
		pts[2].x = rect->left+radius; pts[2].y = rect->top;
		pts[3].x = rect->right-1-radius; pts[3].y = rect->top;
		pts[4].x = rect->right-1; pts[4].y = rect->top+radius;
		pts[5].x = rect->right-1; pts[5].y = rect->bottom-1;
	}
	else
	{	pts[0].x = rect->left; pts[0].y = rect->top;
		pts[1].x = rect->left; pts[1].y = rect->bottom-1-radius;
		pts[2].x = rect->left+radius; pts[2].y = rect->bottom-1;
		pts[3].x = rect->right-1-radius; pts[3].y = rect->bottom-1;
		pts[4].x = rect->right-1; pts[4].y = rect->bottom-1-radius;
		pts[5].x = rect->right-1; pts[5].y = rect->top;
	}
	DrawFrame(pDC,pts,sizeof(pts)/sizeof(POINT),clrBorder,clrBack);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawFrame(CDC *pDC, POINT const *pPoints, int iCount, COLORREF clrBorder, COLORREF clrBack)
{	CPen pen(PS_SOLID,1,clrBack);
	CPen *pOldPen = pDC->SelectObject(&pen);
		// 
	CBrush brush(clrBack);
	CBrush *pBrushOld = pDC->SelectObject(&brush);
		// 
	pDC->Polygon(pPoints,iCount);
		// 
	pDC->SelectObject(pBrushOld);
	pDC->SelectObject(pOldPen);
		// 
	DrawFrame(pDC,pPoints,iCount,clrBorder);
}
// 
void TabCtrlStyle_base::DrawFrame(CDC *pDC, POINT const *pPoints, int iCount, COLORREF clrLine)
{	CPen pen(PS_SOLID,1,clrLine);
	CPen *pOldPen = pDC->SelectObject(&pen);
		// 
	pDC->MoveTo(pPoints[0].x,pPoints[0].y);
	for(int i=1; i<iCount; ++i)
		pDC->LineTo(pPoints[i].x,pPoints[i].y);
	pDC->SetPixelV(pPoints[iCount-1].x,pPoints[iCount-1].y,clrLine);	// draw last pixel.
		// 
	pDC->SelectObject(pOldPen);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawGradient(CDC *pDC, CRect const *rect, bool horz, COLORREF clrTopLeft, COLORREF clrBottomRight)
{	GRADIENT_RECT gRect = {0,1};
	TRIVERTEX vert[2] = 
	{	{rect->left,rect->top,static_cast<COLOR16>((GetRValue(clrTopLeft) << 8)),static_cast<COLOR16>((GetGValue(clrTopLeft) << 8)),static_cast<COLOR16>((GetBValue(clrTopLeft) << 8)),0},
		{rect->right,rect->bottom,static_cast<COLOR16>((GetRValue(clrBottomRight) << 8)),static_cast<COLOR16>((GetGValue(clrBottomRight) << 8)),static_cast<COLOR16>((GetBValue(clrBottomRight) << 8)),0}
	};
	::GradientFill(pDC->m_hDC,vert,2,&gRect,1,(horz ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawLine(CDC *pDC, int x1, int y1, int x2, int y2, COLORREF clrLine)
{	CPen pen(PS_SOLID,1,clrLine);
	CPen *pOldPen = pDC->SelectObject(&pen);
	pDC->MoveTo(x1,y1);
	pDC->LineTo(x2,y2);
	pDC->SelectObject(pOldPen);
}
// 
void TabCtrlStyle_base::DrawLine(CDC *pDC, int x1, int y1, int x2, int y2)
{	pDC->MoveTo(x1,y1);
	pDC->LineTo(x2,y2);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::DrawRect(CDC *pDC, int x1, int y1, int x2, int y2, COLORREF clrLine)
{	CPen pen(PS_SOLID,1,clrLine);
	CPen *pOldPen = pDC->SelectObject(&pen);
	pDC->MoveTo(x1,y1);
	pDC->LineTo(x1,y2);
	pDC->LineTo(x2,y2);
	pDC->LineTo(x2,y1);
	pDC->LineTo(x1,y1);
	pDC->SelectObject(pOldPen);
}
// 
void TabCtrlStyle_base::DrawRect(CDC *pDC, CRect const *rect, COLORREF clrLine)
{	DrawRect(pDC,rect->left,rect->top,rect->right-1,rect->bottom-1,clrLine);
}
// 
void TabCtrlStyle_base::DrawRect(CDC *pDC, CRect const *rect)
{	pDC->MoveTo(rect->left,rect->top);
	pDC->LineTo(rect->left,rect->bottom-1);
	pDC->LineTo(rect->right-1,rect->bottom-1);
	pDC->LineTo(rect->right-1,rect->top);
	pDC->LineTo(rect->left,rect->top);
}
/////////////////////////////////////////////////////////////////////////////
	// 
COLORREF TabCtrlStyle_base::MixingColors(COLORREF src, COLORREF dst, int percent)
{	const int ipercent = 100 - percent;
	return RGB(
		(GetRValue(src) * percent + GetRValue(dst) * ipercent) / 100,
		(GetGValue(src) * percent + GetGValue(dst) * ipercent) / 100,
		(GetBValue(src) * percent + GetBValue(dst) * ipercent) / 100);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_base::FillSolidRect(CDC *dc, CRect const *rc, COLORREF color)
{	HBRUSH hBrush = ::CreateSolidBrush(color);
	::FillRect(dc->m_hDC,rc,hBrush);
	::DeleteObject(hBrush);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2003_base
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2003_base::DrawButtonClose(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	DrawButtonFrame(ctrl,dc,rect,hover,pushed);
		// 
	CRect rc(rect);
	if(hover && pushed)
		rc.OffsetRect(1,1);
		// 
	Gdiplus::Bitmap *images = ctrl->GetSystemImages();
	const COLORREF clrTransp = ctrl->GetSystemImagesTranspColor();
	DrawMarker(ctrl, dc, images, &rc, TabCtrl::SysImageButtonClose, clrTransp, GetButtonCloseColor(ctrl,hover,pushed));
}
// 
void TabCtrlStyle_VS2003_base::DrawButtonMenu(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	DrawButtonFrame(ctrl,dc,rect,hover,pushed);
		// 
	CRect rc(rect);
	if(hover && pushed)
		rc.OffsetRect(1,1);
		// 
	Gdiplus::Bitmap *images = ctrl->GetSystemImages();
	const COLORREF clrTransp = ctrl->GetSystemImagesTranspColor();
	DrawMarker(ctrl, dc, images, &rc, (!partialView ? TabCtrl::SysImageButtonMenuFullView : TabCtrl::SysImageButtonMenuPartialView),
		clrTransp, GetButtonMenuColor(ctrl,hover,pushed));
}
// 
void TabCtrlStyle_VS2003_base::DrawButtonScrollLeft(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	DrawButtonFrame(ctrl,dc,rect,hover,pushed);
		// 
	CRect rc(rect);
	if(hover && pushed)
		rc.OffsetRect(1,1);
		// 
	Gdiplus::Bitmap *images = ctrl->GetSystemImages();
	const COLORREF clrTransp = ctrl->GetSystemImagesTranspColor();
	DrawMarker(ctrl, dc, images, &rc, (!partialView ? TabCtrl::SysImageButtonScrollLeftForbid : TabCtrl::SysImageButtonScrollLeftAllow),
		clrTransp, GetButtonScrollLeftColor(ctrl,hover,pushed) );
}
// 
void TabCtrlStyle_VS2003_base::DrawButtonScrollRight(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	DrawButtonFrame(ctrl,dc,rect,hover,pushed);
		// 
	CRect rc(rect);
	if(hover && pushed)
		rc.OffsetRect(1,1);
		// 
	Gdiplus::Bitmap *images = ctrl->GetSystemImages();
	const COLORREF clrTransp = ctrl->GetSystemImagesTranspColor();
	DrawMarker(ctrl, dc, images, &rc, (!partialView ? TabCtrl::SysImageButtonScrollRightForbid : TabCtrl::SysImageButtonScrollRightAllow),
		clrTransp, GetButtonScrollRightColor(ctrl,hover,pushed) );
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2003_base::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover && !pushed)
		dc->DrawEdge(const_cast<CRect *>(rect),BDR_RAISEDOUTER,BF_RECT);
	else if(hover && pushed)
		dc->DrawEdge(const_cast<CRect *>(rect),BDR_SUNKENINNER,BF_RECT);
}
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2003_base::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return MixingColors(::GetSysColor(COLOR_BTNFACE),::GetSysColor(COLOR_BTNHIGHLIGHT),45);   // it is approximate color (VS2003 uses some another way).
}
// 
COLORREF TabCtrlStyle_VS2003_base::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	if(ctrl->GetSelectedTab()==tab)
	{	if(!ctrl->IsWatchCtrlActivity() || ctrl->IsActive())
			return ::GetSysColor(COLOR_BTNTEXT);
		return MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(0,0,0),55);
	}
		// 
	return (!ctrl->IsTabDisabled(tab) ? 
		MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(0,0,0),55) :   // it is approximate color (VS2003 uses some another way).
		TabCtrlStyle_base::GetTabTextColor(ctrl,tab));
}
// 
COLORREF TabCtrlStyle_VS2003_base::GetButtonCloseColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return (ctrl->GetSelectedTab() ?
		MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(0,0,0),55) :
		TabCtrlStyle_base::GetButtonCloseColor(ctrl,hover,pushed));
}
COLORREF TabCtrlStyle_VS2003_base::GetButtonMenuColor(TabCtrl const * /*ctrl*/, bool /*hover*/, bool /*pushed*/)
{	return MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(0,0,0),55);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2003_client
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
CRect TabCtrlStyle_VS2003_client::GetControlAreaPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(5,2,3,0) : CRect(5,0,3,2));
}
// 
CRect TabCtrlStyle_VS2003_client::GetWindowsAreaPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	const bool bHideSingleTab = (ctrl->GetNumberTabs()==1 && ctrl->IsHideSingleTab());
	if(!bHideSingleTab)
		return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(2,3,2,2) : CRect(2,2,2,3));
	return CRect(2,2,2,2);
}
// 
CRect TabCtrlStyle_VS2003_client::GetTabPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? 
		CRect(6,1/*border*/+2,6,2+1/*line*/) : CRect(6,1/*line*/+2,6,2+1/*border*/));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2003_client::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	FillSolidRect(dc,rect,GetCtrlAreaBackColor(ctrl));
	if(ctrl->GetLayout()==TabCtrl::LayoutTop)
		DrawLine(dc,rect->left,rect->bottom-1,rect->right,rect->bottom-1,::GetSysColor(COLOR_BTNHIGHLIGHT));
	else
		DrawLine(dc,rect->left,rect->top,rect->right,rect->top,::GetSysColor(COLOR_BTNHIGHLIGHT));
	DrawLine(dc,rect->left,rect->top,rect->left,rect->bottom,GetWndsAreaBackColor(ctrl));
	DrawLine(dc,rect->right-1,rect->top,rect->right-1,rect->bottom,GetWndsAreaBackColor(ctrl));
}
// 
void TabCtrlStyle_VS2003_client::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
	const bool select = (ctrl->GetSelectedTab()==tab);
		// 
	CRect rc(rect);
		// 
	if(select)
	{	CPen penBorder(PS_SOLID,1, GetTabBorderColor(ctrl) );
		CPen *pOldPen = dc->SelectObject(&penBorder);
			// 
		if(top)
		{	FillSolidRect(dc,&rc,GetWndsAreaBackColor(ctrl));   // fill background.
				// draw left-top border.
			dc->MoveTo(rc.left,rc.bottom-1);
			dc->LineTo(rc.left,rc.top);
			dc->LineTo(rc.right,rc.top);
				// draw right border.
			DrawLine(dc,rc.right-1,rc.top+1,rc.right-1,rc.bottom,::GetSysColor(COLOR_BTNTEXT));
		}
		else
		{	FillSolidRect(dc,&rc,GetWndsAreaBackColor(ctrl));   // fill background.
				// draw left-bottom border.
			dc->MoveTo(rc.left,rc.top);
			dc->LineTo(rc.left,rc.bottom-1);
			dc->LineTo(rc.right,rc.bottom-1);
				// draw right border.
			DrawLine(dc,rc.right-1,rc.top,rc.right-1,rc.bottom-1,::GetSysColor(COLOR_BTNTEXT));
		}
		dc->SelectObject(pOldPen);
	}
	else
	{	CRect rcTabPadding = ctrl->GetTabPadding();
		rc.DeflateRect(0,rcTabPadding.top-1,0,rcTabPadding.bottom-1);
		DrawLine(dc,rc.right,rc.top,rc.right,rc.bottom, ::GetSysColor(COLOR_BTNSHADOW) );   // draw right separator.
	}
}
// 
void TabCtrlStyle_VS2003_client::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
		TabCtrlStyle_base::DrawWindowsAreaBack(ctrl,dc,rect);
	else
	{	CRect rc(rect);
		FillSolidRect(dc,&rc,GetWndsAreaBackColor(ctrl));
			// 
		rc.DeflateRect( GetWindowsAreaPadding(ctrl,nullptr) );
		rc.InflateRect(1,1);
			// 
		if( !rc.IsRectEmpty() )
		{	const COLORREF color = GetBorderColor(ctrl);
			DrawRect(dc,&rc,color);   // rect around child window.
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2003_client::GetTabBorderColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_BTNHIGHLIGHT);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2003_client_custom1
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2003_client_custom1::DrawButtonClose(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	TabCtrlStyle_base::DrawButtonClose(ctrl,dc,rect,hover,pushed);
}
// 
void TabCtrlStyle_VS2003_client_custom1::DrawButtonMenu(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	TabCtrlStyle_base::DrawButtonMenu(ctrl,dc,rect,hover,pushed,partialView);
}
// 
void TabCtrlStyle_VS2003_client_custom1::DrawButtonScrollLeft(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	TabCtrlStyle_base::DrawButtonScrollLeft(ctrl,dc,rect,hover,pushed,partialView);
}
// 
void TabCtrlStyle_VS2003_client_custom1::DrawButtonScrollRight(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	TabCtrlStyle_base::DrawButtonScrollRight(ctrl,dc,rect,hover,pushed,partialView);
}
// 
void TabCtrlStyle_VS2003_client_custom1::DrawButtonFrame(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	TabCtrlStyle_base::DrawButtonFrame(ctrl,dc,rect,hover,pushed);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2003_bars
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrlStyle_VS2003_bars::GetBorderWidth(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return 0;
}
// 
CRect TabCtrlStyle_VS2003_bars::GetControlAreaPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(5,2,3,2) : CRect(5,2,3,2));
}
// 
CRect TabCtrlStyle_VS2003_bars::GetTabPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? 
		CRect(6,1/*border*/+1,6,1+1/*line*/) : CRect(6,1/*line*/+1,6,1+1/*border*/));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2003_bars::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	CRect rc(rect);
	const COLORREF clrWndsAreaBack = GetWndsAreaBackColor(ctrl);
		// 
	if(ctrl->GetLayout()==TabCtrl::LayoutTop)
	{	DrawLine(dc,rc.left+1,rc.top,rc.right-1,rc.top,clrWndsAreaBack);
		++rc.top;
		rc.bottom -= 3;
		FillSolidRect(dc,&rc,GetCtrlAreaBackColor(ctrl));
		rc.top = rc.bottom;
		rc.bottom = rect->bottom;
		DrawLine(dc,rc.left+1,rc.top,rc.right-1,rc.top,::GetSysColor(COLOR_BTNTEXT));
		++rc.top;
		FillSolidRect(dc,&rc,clrWndsAreaBack);
	}
	else
	{	rc.top += 3;
		--rc.bottom;
		DrawLine(dc,rc.left+1,rc.bottom,rc.right-1,rc.bottom,clrWndsAreaBack);
		FillSolidRect(dc,&rc,GetCtrlAreaBackColor(ctrl));
		--rc.top;
		DrawLine(dc,rc.left+1,rc.top,rc.right-1,rc.top,::GetSysColor(COLOR_BTNTEXT));
		rc.bottom = rc.top;
		rc.top = rect->top;
		FillSolidRect(dc,&rc,clrWndsAreaBack);
	}
	DrawLine(dc,rect->left,rect->top,rect->left,rect->bottom,clrWndsAreaBack);
	DrawLine(dc,rect->right-1,rect->top,rect->right-1,rect->bottom,clrWndsAreaBack);
}
// 
void TabCtrlStyle_VS2003_bars::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
	const bool select = (ctrl->GetSelectedTab()==tab);
		// 
	CRect rc(rect);
		// 
	if(select)
	{	CPen penBorder(PS_SOLID,1, GetTabBorderColor(ctrl) );
		CPen *pOldPen = dc->SelectObject(&penBorder);
			// 
		if(top)
		{	rc.DeflateRect(1,1,1,0);
			FillSolidRect(dc,&rc,GetWndsAreaBackColor(ctrl));   // fill background.
				// draw left-top border.
			rc.InflateRect(1,1,1,0);
			dc->MoveTo(rc.left+1,rc.top);
			dc->LineTo(rc.right-1,rc.top);
			dc->LineTo(rc.right-1,rc.bottom);
				// draw right border.
			DrawLine(dc,rc.left,rc.top,rc.left,rc.bottom-1,::GetSysColor(COLOR_BTNHIGHLIGHT));
		}
		else
		{	rc.DeflateRect(1,0,1,1);
			FillSolidRect(dc,&rc,GetWndsAreaBackColor(ctrl));   // fill background.
				// draw left-bottom border.
			rc.InflateRect(1,0,1,1);
			dc->MoveTo(rc.left+1,rc.bottom-1);
			dc->LineTo(rc.right-1,rc.bottom-1);
			dc->LineTo(rc.right-1,rc.top-1);
				// draw right border.
			DrawLine(dc,rc.left,rc.top+1,rc.left,rc.bottom,::GetSysColor(COLOR_BTNHIGHLIGHT));
		}
		dc->SelectObject(pOldPen);
	}
	else
	{	const CRect rcTabPadding = ctrl->GetTabPadding();
		if(top)
			rc.DeflateRect(0,rcTabPadding.top,0,rcTabPadding.bottom+1);
		else
			rc.DeflateRect(0,rcTabPadding.top+1,0,rcTabPadding.bottom);
		DrawLine(dc,rc.right,rc.top,rc.right,rc.bottom, ::GetSysColor(COLOR_BTNSHADOW) );   // draw right separator.
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2003_bars::GetTabBorderColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_BTNTEXT);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2003_bars_custom1
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
CRect TabCtrlStyle_VS2003_bars_custom1::GetControlAreaPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(4,1,2,0) : CRect(4,0,2,1));
}
// 
CRect TabCtrlStyle_VS2003_bars_custom1::GetWindowsAreaPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	const bool bHideSingleTab = (ctrl->GetNumberTabs()==1 && ctrl->IsHideSingleTab());
	if(!bHideSingleTab)
		return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(1,0,1,1) : CRect(1,1,1,0));
	return CRect(1,1,1,1);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2003_bars_custom1::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	CRect rc(rect);
	const COLORREF clrBorder = GetTabBorderColor(ctrl);
	const COLORREF clrCtrlAreaBack = GetCtrlAreaBackColor(ctrl);
		// 
	if(ctrl->GetLayout()==TabCtrl::LayoutTop)
	{	--rc.bottom;
		FillSolidRect(dc,&rc,clrCtrlAreaBack);
		DrawLine(dc,rc.left,rc.bottom,rc.right,rc.bottom,clrBorder);
	}
	else
	{	DrawLine(dc,rc.left,rc.top,rc.right,rc.top,clrBorder);
		++rc.top;
		FillSolidRect(dc,&rc,clrCtrlAreaBack);
	}
}
// 
void TabCtrlStyle_VS2003_bars_custom1::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	const int count = ctrl->GetNumberTabs();
		// 
	if(count==0 || ctrl->GetSelectedTab()==nullptr)
		TabCtrlStyle_base::DrawWindowsAreaBack(ctrl,dc,rect);
	else
	{	const COLORREF clrBorder = GetTabBorderColor(ctrl);
		const bool bHideSingleTab = (count==1 && ctrl->IsHideSingleTab());
			// 
		if(bHideSingleTab)
			DrawRect(dc,rect,clrBorder);
		else
		{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
				// 
			CPen penBorder(PS_SOLID,1,clrBorder);
			CPen *pOldPen = dc->SelectObject(&penBorder);
				// 
			if(top)
			{	dc->MoveTo(rect->left,rect->top);
				dc->LineTo(rect->left,rect->bottom-1);
				dc->LineTo(rect->right-1,rect->bottom-1);
				dc->LineTo(rect->right-1,rect->top-1);
			}
			else
			{	dc->MoveTo(rect->left,rect->bottom-1);
				dc->LineTo(rect->left,rect->top);
				dc->LineTo(rect->right-1,rect->top);
				dc->LineTo(rect->right-1,rect->bottom);
			}
			dc->SelectObject(pOldPen);
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2003_bars_custom1::DrawButtonClose(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	TabCtrlStyle_base::DrawButtonClose(ctrl,dc,rect,hover,pushed);
}
// 
void TabCtrlStyle_VS2003_bars_custom1::DrawButtonMenu(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	TabCtrlStyle_base::DrawButtonMenu(ctrl,dc,rect,hover,pushed,partialView);
}
// 
void TabCtrlStyle_VS2003_bars_custom1::DrawButtonScrollLeft(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	TabCtrlStyle_base::DrawButtonScrollLeft(ctrl,dc,rect,hover,pushed,partialView);
}
// 
void TabCtrlStyle_VS2003_bars_custom1::DrawButtonScrollRight(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed, bool partialView)
{	TabCtrlStyle_base::DrawButtonScrollRight(ctrl,dc,rect,hover,pushed,partialView);
}
// 
void TabCtrlStyle_VS2003_bars_custom1::DrawButtonFrame(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	TabCtrlStyle_base::DrawButtonFrame(ctrl,dc,rect,hover,pushed);
}
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2003_bars_custom1::GetTabBorderColor(TabCtrl const * /*ctrl*/)
{	return ::GetSysColor(COLOR_BTNSHADOW);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_client_base
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrlStyle_VS2008_client_base::GetBorderWidth(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return 0;
}
// 
CRect TabCtrlStyle_VS2008_client_base::GetControlAreaPadding(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return CRect(0,0,3,0);
}
CRect TabCtrlStyle_VS2008_client_base::GetWindowsAreaPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	const bool bHideSingleTab = (ctrl->GetNumberTabs()==1 && ctrl->IsHideSingleTab());
	if(!bHideSingleTab)
		return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(5,3,5,5) : CRect(5,5,5,3));
	return CRect(5,5,5,5);
}
// 
CRect TabCtrlStyle_VS2008_client_base::GetTabPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(6,1/*border*/+2,6,3+2/*indent*/) : CRect(6,2/*indent*/+2,6,3+1/*border*/));
}
int TabCtrlStyle_VS2008_client_base::GetTabExtraWidth(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/, TabCtrl::HTAB tab)
{	if( ctrl->GetTabIndexByHandle(tab) )
		return 0;   // it isn't first tab.
	return GetSlantWidth(ctrl) - 6;
}
int TabCtrlStyle_VS2008_client_base::GetTabMinWidth(TabCtrl const *ctrl, TabCtrl::IRecalc *base)
{	return base->GetTabMinWidth(ctrl,base) + (GetSlantWidth(ctrl)-6);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_client_base::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	const bool allDisabled = !ctrl->GetSelectedTab();   // all tabs are disabled.
		// 
	FillSolidRect(dc,rect,GetCtrlAreaBackColor(ctrl));
		// 
	CPen penBorder(PS_SOLID,1, GetTabBorderColor(ctrl,true,allDisabled) );
	CPen penOutline(PS_SOLID,1, GetTabOutlineColor(ctrl,true,false,allDisabled,true) );
		// 
	CPen *pOldPen = dc->SelectObject(&penBorder);
		// 
	if(ctrl->GetLayout()==TabCtrl::LayoutTop)
	{	if(!allDisabled)
		{		// draw border.
			dc->MoveTo(rect->left+1,rect->bottom-1);
			dc->LineTo(rect->left+2,rect->bottom-2);
			dc->LineTo(rect->right-3,rect->bottom-2);
			dc->LineTo(rect->right-1,rect->bottom);
				// draw outline.
			dc->SelectObject(&penOutline);
			dc->MoveTo(rect->left+2,rect->bottom-1);
			dc->LineTo(rect->right-2,rect->bottom-1);
		}
		else
		{		// draw border.
			dc->MoveTo(rect->left,rect->bottom-2);
			dc->LineTo(rect->right,rect->bottom-2);
				// draw outline.
			dc->SelectObject(&penOutline);
			dc->MoveTo(rect->left,rect->bottom-1);
			dc->LineTo(rect->right,rect->bottom-1);
		}
	}
	else
	{	if(!allDisabled)
		{		// draw border.
			dc->MoveTo(rect->left+1,rect->top);
			dc->LineTo(rect->left+2,rect->top+1);
			dc->LineTo(rect->right-3,rect->top+1);
			dc->LineTo(rect->right-1,rect->top-1);
				// draw outline.
			dc->SelectObject(&penOutline);
			dc->MoveTo(rect->left+2,rect->top);
			dc->LineTo(rect->right-2,rect->top);
		}
		else
		{		// draw border.
			dc->MoveTo(rect->left,rect->top+1);
			dc->LineTo(rect->right,rect->top+1);
				// draw outline.
			dc->SelectObject(&penOutline);
			dc->MoveTo(rect->left,rect->top);
			dc->LineTo(rect->right,rect->top);
		}
	}
		// 
	dc->SelectObject(pOldPen);
}
// 
void TabCtrlStyle_VS2008_client_base::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn *rgn)
{	const bool selected = (ctrl->GetSelectedTab()==tab);
	const bool hover = (ctrl->GetTabUnderCursor()==tab) && (ctrl->GetPushedTab()==nullptr);
	const bool disabled = ctrl->IsTabDisabled(tab);
	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		// 
	const COLORREF clrBorder = GetTabBorderColor(ctrl,selected,disabled);
	const COLORREF clrBackLight = GetTabGradientLightColor(ctrl,selected,hover,disabled);
	const COLORREF clrBackDark = GetTabGradientDarkColor(ctrl,selected,hover,disabled);
	const COLORREF clrOutlineLeft = GetTabOutlineColor(ctrl,selected,hover,disabled,true);
	const COLORREF clrOutlineRight = GetTabOutlineColor(ctrl,selected,hover,disabled,false);
		// 
	CPen penBorder(PS_SOLID,1,clrBorder);
	CPen penOutlineLeft(PS_SOLID,1,clrOutlineLeft);
	CPen penOutlineRight(PS_SOLID,1,clrOutlineRight);
		// 
	POINT pts[8];
	CRect rcFill;
	GetTabOutline(ctrl,tab,rect,top,pts,&rcFill);
		// 
	CRgn rgnGrad;
	rgnGrad.CreatePolygonRgn(pts,sizeof(pts)/sizeof(POINT),WINDING);   // create clip region for gradient.
		// 
	CRgn rgnRes;
	rgnRes.CreateRectRgn(0,0,0,0);
	rgnRes.CombineRgn(&rgnGrad,rgn,RGN_AND);
		// 
	dc->SelectClipRgn(&rgnRes,RGN_COPY);
		// 
		// draw back.
	(top ?
		DrawGradient(dc,&rcFill,false,clrBackLight,clrBackDark) :
		DrawGradient(dc,&rcFill,false,clrBackDark,clrBackLight));
		// 
		// left outline.
	CPen *pOldPen = dc->SelectObject(&penOutlineLeft);
	dc->MoveTo(pts[1].x+1,pts[1].y);
	dc->LineTo(pts[2].x+1,pts[2].y);
	dc->LineTo(pts[3].x+1,pts[3].y);
		// 
		// right outline.
	pOldPen = dc->SelectObject(&penOutlineRight);
	dc->MoveTo(pts[4].x-1,pts[4].y);
	dc->LineTo(pts[5].x-1,pts[5].y);
	dc->LineTo(pts[6].x-1,pts[6].y);
		// 
	dc->SelectClipRgn(rgn,RGN_COPY);   // restore clip region.
		// 
		// draw border.
	pOldPen = dc->SelectObject(&penBorder);
	dc->MoveTo(pts[1].x,pts[1].y);
	dc->LineTo(pts[2].x,pts[2].y);
	dc->LineTo(pts[3].x,pts[3].y);
	dc->LineTo(pts[4].x,pts[4].y);
	dc->LineTo(pts[5].x,pts[5].y);
	dc->LineTo(pts[6].x,pts[6].y);
		// 
	dc->SelectObject(pOldPen);
}
// 
void TabCtrlStyle_VS2008_client_base::DrawTabContext(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn *rgn)
{	CRect rc(rect);
	if(ctrl->GetSelectedTab()!=tab)
		(ctrl->GetLayout()==TabCtrl::LayoutTop ? rc.top+=2 : rc.bottom-=2);
	rc.left += ctrl->GetTabExtraWidth(tab);
	TabCtrlStyle_base::DrawTabContext(ctrl,dc,tab,&rc,rgn);
}
// 
void TabCtrlStyle_VS2008_client_base::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	const int count = ctrl->GetNumberTabs();
		// 
	if(count==0 || ctrl->GetSelectedTab()==nullptr)
		TabCtrlStyle_base::DrawWindowsAreaBack(ctrl,dc,rect);
	else
	{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		const bool bHideSingleTab = (count==1 && ctrl->IsHideSingleTab());
			// 
		const COLORREF clrBorder = GetTabBorderColor(ctrl,true,false);
		const COLORREF clrOutline = GetTabOutlineColor(ctrl,true,false,false,true);
		const COLORREF clrBack = GetTabGradientDarkColor(ctrl,true,false,false);
		const COLORREF clrBackOut = GetWndsAreaBackColor(ctrl);
			// 
		CPen penBorder(PS_SOLID,1,clrBorder);
		CPen penOutline(PS_SOLID,1,clrOutline);
		CPen penBack(PS_SOLID,1,clrBack);
			// 
		CRect rc(rect);
			// 
		CPen *pOldPen = dc->SelectObject(&penBorder);
			// 
		if(bHideSingleTab)
		{		// outside border.
			DrawBeveledRect(dc,&rc,2);
				// left-top corner.
			::SetPixelV(dc->m_hDC,rc.left,rc.top,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.left+1,rc.top,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.left,rc.top+1,clrBackOut);
				// right-top corner.
			::SetPixelV(dc->m_hDC,rc.right-1,rc.top,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.right-2,rc.top,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.right-1,rc.top+1,clrBackOut);
				// left-bottom corner.
			::SetPixelV(dc->m_hDC,rc.left,rc.bottom-1,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.left+1,rc.bottom-1,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.left,rc.bottom-2,clrBackOut);
				// right-bottom corner.
			::SetPixelV(dc->m_hDC,rc.right-1,rc.bottom-1,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.right-2,rc.bottom-1,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.right-1,rc.bottom-2,clrBackOut);
				// inside border.
			rc.DeflateRect(4,4);
			if( !rc.IsRectEmpty() )
				DrawRect(dc,&rc);
		}
		else if(top)
		{		// outside border.
			dc->MoveTo(rc.left,rc.top);
			dc->LineTo(rc.left,rc.bottom-3);
			dc->LineTo(rc.left+2,rc.bottom-1);
			dc->LineTo(rc.right-3,rc.bottom-1);
			dc->LineTo(rc.right-1,rc.bottom-3);
			dc->LineTo(rc.right-1,rc.top-1);
				// left-bottom corner.
			::SetPixelV(dc->m_hDC,rc.left,rc.bottom-1,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.left+1,rc.bottom-1,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.left,rc.bottom-2,clrBackOut);
				// right-bottom corner.
			::SetPixelV(dc->m_hDC,rc.right-1,rc.bottom-1,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.right-2,rc.bottom-1,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.right-1,rc.bottom-2,clrBackOut);
				// inside border.
			rc.DeflateRect(4,2,4,4);
			if( !rc.IsRectEmpty() )
				DrawRect(dc,&rc);
		}
		else
		{		// outside border.
			dc->MoveTo(rc.left,rc.bottom-1);
			dc->LineTo(rc.left,rc.top+2);
			dc->LineTo(rc.left+2,rc.top);
			dc->LineTo(rc.right-3,rc.top);
			dc->LineTo(rc.right-1,rc.top+2);
			dc->LineTo(rc.right-1,rc.bottom);
				// left-bottom corner.
			::SetPixelV(dc->m_hDC,rc.left,rc.top,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.left+1,rc.top,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.left,rc.top+1,clrBackOut);
				// right-bottom corner.
			::SetPixelV(dc->m_hDC,rc.right-1,rc.top,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.right-2,rc.top,clrBackOut);
			::SetPixelV(dc->m_hDC,rc.right-1,rc.top+1,clrBackOut);
				// inside border.
			rc.DeflateRect(4,4,4,2);
			if( !rc.IsRectEmpty() )
				DrawRect(dc,&rc);
		}
			// 
			// draw back.
		dc->SelectObject(&penBack);
			// 
		rc.InflateRect(1,1);
		if( !rc.IsRectEmpty() )
			DrawRect(dc,&rc);
		rc.InflateRect(1,1);
		if( !rc.IsRectEmpty() )
			DrawRect(dc,&rc);
			// 
			// draw outline;
		dc->SelectObject(&penOutline);
			// 
		if(bHideSingleTab)
		{	rc.InflateRect(1,1);
			if( !rc.IsRectEmpty() )
			{	dc->MoveTo(rc.left,rc.top+1);
				dc->LineTo(rc.left,rc.bottom-1);
				dc->MoveTo(rc.left+1,rc.bottom-1);
				dc->LineTo(rc.right-1,rc.bottom-1);
				dc->MoveTo(rc.right-1,rc.bottom-2);
				dc->LineTo(rc.right-1,rc.top);
				dc->MoveTo(rc.right-2,rc.top);
				dc->LineTo(rc.left,rc.top);
			}
		}
		else if(top)
		{	rc.InflateRect(1,0,1,1);
			if( !rc.IsRectEmpty() )
			{	dc->MoveTo(rc.left,rc.top);
				dc->LineTo(rc.left,rc.bottom-1);
				dc->MoveTo(rc.left+1,rc.bottom-1);
				dc->LineTo(rc.right-1,rc.bottom-1);
				dc->MoveTo(rc.right-1,rc.bottom-2);
				dc->LineTo(rc.right-1,rc.top-1);
			}
		}
		else
		{	rc.InflateRect(1,1,1,0);
			if( !rc.IsRectEmpty() )
			{	dc->MoveTo(rc.left,rc.bottom-1);
				dc->LineTo(rc.left,rc.top);
				dc->MoveTo(rc.left+1,rc.top);
				dc->LineTo(rc.right-1,rc.top);
				dc->MoveTo(rc.right-1,rc.top+1);
				dc->LineTo(rc.right-1,rc.bottom);
			}
		}
			// 
		dc->SelectObject(pOldPen);
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
TabCtrl::HTAB TabCtrlStyle_VS2008_client_base::HitTest(TabCtrl const *ctrl, IBehavior * /*base*/, CPoint point)   // get tab in the given point.
{	if( CRect(ctrl->GetTabsArea()).PtInRect(point) )
	{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
			// 
		TabCtrl::HTAB hTabSel = ctrl->GetSelectedTab();
		if(hTabSel && HitTest(ctrl,hTabSel,top,point))
			return hTabSel;
			// 
		for(int i=0, c=ctrl->GetNumberTabs(); i<c; ++i)
		{	TabCtrl::HTAB tab = ctrl->GetTabHandleByIndex(i);
			if(tab!=hTabSel && HitTest(ctrl,tab,top,point))
				return tab;
		}
	}
	return nullptr;
}
// 
bool TabCtrlStyle_VS2008_client_base::HitTest(TabCtrl const *ctrl, TabCtrl::HTAB tab, bool top, CPoint point) const
{	const CRect rc = ctrl->GetTabRect(tab);
	POINT pts[8];
	GetTabOutline(ctrl,tab,&rc,top,pts,nullptr);
		// 
	CRgn rgn;
	rgn.CreatePolygonRgn(pts,sizeof(pts)/sizeof(POINT),WINDING);
	return rgn.PtInRegion(point)!=0;
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_client_base::GetTabOutline(TabCtrl const *ctrl, TabCtrl::HTAB tab, CRect const *rect, bool top, POINT pts[8]/*out*/, RECT *rcFill/*out*/) const
{	const bool first = (ctrl->GetTabIndexByHandle(tab)==0);
	const bool selected = (ctrl->GetSelectedTab()==tab);
	int iSlantWidth = GetSlantWidth(ctrl);
		// 
	CRect rc(rect);
		// 
	if(!selected)
	{	rc.top += 2;
		rc.bottom -= 2;
		iSlantWidth -= 4;
	}
		// 
	if(!first)
		rc.left -= (iSlantWidth - 6);
	else if(!selected)
		rc.left += 4;
		// 
	if(top)
	{	if(!selected)
		{	pts[0].x = rc.left; pts[0].y = rc.bottom;
			pts[1].x = rc.left+1; pts[1].y = rc.bottom-1;
		}
		else
		{	pts[0].x = rc.left+2; pts[0].y = rc.bottom;
			pts[1].x = rc.left+2; pts[1].y = rc.bottom-2;
		}
		pts[2].x = rc.left+iSlantWidth-2; pts[2].y = rc.top+2;
		pts[3].x = rc.left+iSlantWidth+3; pts[3].y = rc.top;
		pts[4].x = rc.right-3; pts[4].y = rc.top;
		pts[5].x = rc.right-1; pts[5].y = rc.top+2;
		if(!selected)
		{	pts[6].x = rc.right-1; pts[6].y = rc.bottom;
		}
		else
		{	pts[6].x = rc.right-1; pts[6].y = rc.bottom-1;
		}
		pts[7].x = rc.right-1; pts[7].y = rc.bottom;
	}
	else
	{	if(!selected)
		{	pts[0].x = rc.left; pts[0].y = rc.top;
			pts[1].x = rc.left+1; pts[1].y = rc.top;
		}
		else
		{	pts[0].x = rc.left+2; pts[0].y = rc.top;
			pts[1].x = rc.left+2; pts[1].y = rc.top+1;
		}
		pts[2].x = rc.left+iSlantWidth-2; pts[2].y = rc.bottom-3;
		pts[3].x = rc.left+iSlantWidth+3; pts[3].y = rc.bottom-1;
		pts[4].x = rc.right-3; pts[4].y = rc.bottom-1;
		pts[5].x = rc.right-1; pts[5].y = rc.bottom-3;
		if(!selected)
		{	pts[6].x = rc.right-1; pts[6].y = rc.top-1;
		}
		else
		{	pts[6].x = rc.right-1; pts[6].y = rc.top;
		}
		pts[7].x = rc.right-1; pts[7].y = rc.top;
	}
		// 
	if(rcFill)
	{	*rcFill = rc;
		if(!selected && !first) 
			rcFill->left = rect->left;
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrlStyle_VS2008_client_base::GetSlantWidth(TabCtrl const *ctrl) const
{	return CRect(ctrl->GetTabsArea()).Height();
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_client_base::DrawBeveledRect(CDC *pDC, CRect const *rect, int bevel)
{	pDC->MoveTo(rect->left,rect->top+bevel);
	pDC->LineTo(rect->left,rect->bottom-bevel-1);
	pDC->LineTo(rect->left+bevel,rect->bottom-1);
	pDC->LineTo(rect->right-bevel-1,rect->bottom-1);
	pDC->LineTo(rect->right-1,rect->bottom-bevel-1);
	pDC->LineTo(rect->right-1,rect->top+bevel);
	pDC->LineTo(rect->right-bevel-1,rect->top);
	pDC->LineTo(rect->left+bevel,rect->top);
	pDC->LineTo(rect->left,rect->top+bevel);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_client_classic
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
 
COLORREF TabCtrlStyle_VS2008_client_classic::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool /*active*/, bool /*disable*/)
{	return ::GetSysColor(COLOR_BTNSHADOW);
}
COLORREF TabCtrlStyle_VS2008_client_classic::GetTabOutlineColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable, bool /*left*/)
{	if(active)
		return ::GetSysColor(COLOR_WINDOW);
	if(hover && !disable)
		return ::GetSysColor(COLOR_WINDOW);
	if(disable)
		return ::GetSysColor(COLOR_BTNFACE);
	return ::GetSysColor(COLOR_BTNFACE);
}
COLORREF TabCtrlStyle_VS2008_client_classic::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable)
{	if(active)
		return ::GetSysColor(COLOR_BTNHIGHLIGHT);
	if(hover && !disable)
		return ::GetSysColor(COLOR_BTNHIGHLIGHT);
	return ::GetSysColor(COLOR_BTNFACE);
}
COLORREF TabCtrlStyle_VS2008_client_classic::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable)
{	if(active)
		return ::GetSysColor(COLOR_BTNFACE);
	if(hover && !disable)
		return ::GetSysColor(COLOR_BTNHIGHLIGHT);
	return ::GetSysColor(COLOR_BTNFACE);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_client_blue
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_client_blue::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool active, bool disable)
{	if(active)
		return RGB(105,161,191);
	if(disable)
		return RGB(145,150,162);
	return RGB(145,150,162);
}
COLORREF TabCtrlStyle_VS2008_client_blue::GetTabOutlineColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable, bool left)
{	if(active)
		return RGB(255,255,255);
	if(hover && !disable)
		return RGB(255,255,255);
	if(disable)
		return RGB(140,171,204);
	return (left ? RGB(242,250,255) : RGB(140,171,204));
}
COLORREF TabCtrlStyle_VS2008_client_blue::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable)
{	if(active)
		return RGB(252,253,254);
	if(hover && !disable)
		return RGB(247,252,254);
	if(disable)
		return RGB(207,223,237);
	return RGB(236,245,252);
}
COLORREF TabCtrlStyle_VS2008_client_blue::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable)
{	if(active)
		return RGB(210,230,250);
	if(hover && !disable)
		return RGB(129,208,241);
	if(disable)
		return RGB(207,223,237);
	return RGB(152,180,210);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_client_silver
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_client_silver::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool active, bool disable)
{	if(active)
		return RGB(147,145,176);
	if(disable)
		return RGB(157,157,161);
	return RGB(157,157,161);
}
COLORREF TabCtrlStyle_VS2008_client_silver::GetTabOutlineColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable, bool left)
{	if(active)
		return RGB(255,255,255);
	if(hover && !disable)
		return RGB(255,255,255);
	if(disable)
		return RGB(172,171,196);
	return (left ? RGB(255,255,255) : RGB(172,171,196));
}
COLORREF TabCtrlStyle_VS2008_client_silver::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable)
{	if(active)
		return RGB(247,247,253);
	if(hover && !disable)
		return RGB(247,247,253);
	if(disable)
		return RGB(203,205,217);
	return RGB(234,235,240);
}
COLORREF TabCtrlStyle_VS2008_client_silver::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable)
{	if(active)
		return RGB(225,226,236);
	if(hover && !disable)
		return RGB(182,185,201);
	if(disable)
		return RGB(203,205,217);
	return RGB(172,171,196);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_client_silver::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
	{	FillSolidRect(dc,rect,RGB(255,227,173));
		DrawRect(dc,rect,RGB(74,73,107));
	}
	else if(hover && pushed)
	{	FillSolidRect(dc,rect,RGB(255,182,115));
		DrawRect(dc,rect,RGB(74,73,107));
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_client_olive
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_client_olive::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool active, bool disable)
{	if(active)
		return RGB(147,160,112);
	if(disable)
		return RGB(172,168,153);
	return RGB(172,168,153);
}
COLORREF TabCtrlStyle_VS2008_client_olive::GetTabOutlineColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable, bool left)
{	if(active)
		return RGB(255,255,255);
	if(hover && !disable)
		return RGB(255,255,255);
	if(disable)
		return RGB(165,179,133);
	return (left ? RGB(255,255,255) : RGB(165,179,133));
}
COLORREF TabCtrlStyle_VS2008_client_olive::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable)
{	if(active)
		return RGB(250,251,247);
	if(hover && !disable)
		return RGB(245,247,240);
	if(disable)
		return RGB(208,217,181);
	return RGB(241,244,233);
}
COLORREF TabCtrlStyle_VS2008_client_olive::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool active, bool hover, bool disable)
{	if(active)
		return RGB(173,190,126);
	if(hover && !disable)
		return RGB(197,210,165);
	if(disable)
		return RGB(208,217,181);
	return RGB(165,179,133);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_client_olive::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
	{	FillSolidRect(dc,rect,RGB(255,227,173));
		DrawRect(dc,rect,RGB(118,128,95));
	}
	else if(hover && pushed)
	{	FillSolidRect(dc,rect,RGB(255,182,115));
		DrawRect(dc,rect,RGB(118,128,95));
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_base
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrlStyle_VS2008_bars_base::GetBorderWidth(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return 0;
}
// 
CRect TabCtrlStyle_VS2008_bars_base::GetControlAreaPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(0,1,0,2) : CRect(0,2,0,1));
}
// 
CRect TabCtrlStyle_VS2008_bars_base::GetTabPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? 
		CRect(1/*border*/+2,1+1/*border*/+2/*indent*/,3,1/*border*/+1) : 
		CRect(1/*border*/+2,1/*border*/+1,3,1+1/*border*/+2/*indent*/));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_bars_base::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	FillSolidRect(dc,rect, GetCtrlAreaBackColor(ctrl) );
		// 
	const COLORREF clrBorder = GetTabBorderColor(ctrl,false);
		// 
	if(ctrl->GetLayout()==TabCtrl::LayoutTop)
		DrawLine(dc,rect->left,rect->bottom-3,rect->right,rect->bottom-3, clrBorder );
	else
		DrawLine(dc,rect->left,rect->top+2,rect->right,rect->top+2, clrBorder );
}
// 
void TabCtrlStyle_VS2008_bars_base::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
	const bool select = (ctrl->GetSelectedTab()==tab);
	const int count = ctrl->GetNumberTabs();
		// 
	const COLORREF clrBorder = GetTabBorderColor(ctrl,false);
		// 
	CPen penBorder(PS_SOLID,1,clrBorder);
	CPen *pOldPen = dc->SelectObject(&penBorder);
		// 
	if(select)
	{	const bool firstTab = (ctrl->GetTabHandleByIndex(0)==tab);
		const COLORREF clrBackSelected = GetTabSelectedBackColor(ctrl);
			// 
			// draw back.
		FillSolidRect(dc,rect,clrBackSelected);   // fill background.
			// 
			// draw tab border.
		if(top)
		{	dc->MoveTo(rect->left,rect->bottom - (firstTab ? 1 : 2));
			dc->LineTo(rect->left,rect->top);
			dc->LineTo(rect->right-1,rect->top);
			dc->LineTo(rect->right-1,rect->bottom-1);
		}
		else
		{	dc->MoveTo(rect->left,rect->top + (firstTab ? 0 : 1));
			dc->LineTo(rect->left,rect->bottom-1);
			dc->LineTo(rect->right-1,rect->bottom-1);
			dc->LineTo(rect->right-1,rect->top);
		}
	}
	else	// tab isn't selected.
	{	const bool hover = (ctrl->GetTabUnderCursor()==tab) && (ctrl->GetPushedTab()==nullptr);
		const bool disabled = ctrl->IsTabDisabled(tab);
			// 		
		const COLORREF clrBorderHover = GetTabBorderColor(ctrl,true);
		const COLORREF clrBackLight = GetTabGradientLightColor(ctrl,hover,disabled);
		const COLORREF clrBackDark = GetTabGradientDarkColor(ctrl,hover,disabled);
			// 
		TabCtrl::HTAB tabSel = ctrl->GetSelectedTab();
		const int cmpRes = (tabSel ? ctrl->CompareTabsPosition(tab,tabSel) : -1);
			// 
		CRect rc(rect);
			// 
		if(top)
		{	rc.DeflateRect(0,2,0,1);
				// 
				// draw back.
			DrawGradient(dc,&rc,false,clrBackLight,clrBackDark);
				// 
			if(cmpRes<0)   // before selected tab or there isn't selected tab.
			{		// draw tab border.
				if(hover && !disabled)
				{	DrawLine(dc,rc.left,rc.bottom-1,rc.left,rc.top,clrBorderHover);
					DrawLine(dc,rc.left,rc.top,rc.right,rc.top,clrBorderHover);
				}
				else
				{	dc->MoveTo(rc.left,rc.bottom-1);
					dc->LineTo(rc.left,rc.top);
					dc->LineTo(rc.right,rc.top);
				}
					// draw outline.
				DrawLine(dc,rc.left+1,rc.top+1,rc.left+1,rc.bottom,clrBackLight);
			}
			else	// after selected tab.
			{		// draw tab border.
				if(hover && !disabled)
				{	DrawLine(dc,rc.left,rc.top,rc.right-1,rc.top,clrBorderHover);
					DrawLine(dc,rc.right-1,rc.top,rc.right-1,rc.bottom,clrBorderHover);
				}
				else
				{	dc->MoveTo(rc.left,rc.top);
					dc->LineTo(rc.right-1,rc.top);
					dc->LineTo(rc.right-1,rc.bottom);
				}
					// draw outline.
				DrawLine(dc,rc.right-2,rc.top+1,rc.right-2,rc.bottom,clrBackLight);
			}
		}
		else		// bottom.
		{	rc.DeflateRect(0,1,0,2);
				// 
				// draw back.
			DrawGradient(dc,&rc,false,clrBackDark,clrBackLight);
				// 
			if(cmpRes<0)   // before selected tab or there isn't selected tab.
			{		// draw tab border.
				if(hover && !disabled)
				{	DrawLine(dc,rc.left,rc.top,rc.left,rc.bottom-1,clrBorderHover);
					DrawLine(dc,rc.left,rc.bottom-1,rc.right,rc.bottom-1,clrBorderHover);
				}
				else
				{	dc->MoveTo(rc.left,rc.top);
					dc->LineTo(rc.left,rc.bottom-1);
					dc->LineTo(rc.right,rc.bottom-1);
				}
					// draw outline.
				DrawLine(dc,rc.left+1,rc.top,rc.left+1,rc.bottom-1,clrBackLight);
			}
			else	// after selected tab.
			{		// draw tab border.
				if(hover && !disabled)
				{	DrawLine(dc,rc.left,rc.bottom-1,rc.right-1,rc.bottom-1,clrBorderHover);
					DrawLine(dc,rc.right-1,rc.bottom-1,rc.right-1,rc.top-1,clrBorderHover);
				}
				else
				{	dc->MoveTo(rc.left,rc.bottom-1);
					dc->LineTo(rc.right-1,rc.bottom-1);
					dc->LineTo(rc.right-1,rc.top-1);
				}
					// draw outline.
				DrawLine(dc,rc.right-2,rc.top,rc.right-2,rc.bottom-1,clrBackLight);
			}
		}
			// 
			// draw closing line.
		if(cmpRes<0)   // before selected tab or there isn't selected tab.
		{	const bool lastTab = (ctrl->GetTabHandleByIndex(count-1)==tab);
				// 
			if(lastTab)
				DrawLine(dc,rc.right-1,rc.top,rc.right-1,rc.bottom);
		}
	}
	dc->SelectObject(pOldPen);
}
// 
void TabCtrlStyle_VS2008_bars_base::DrawTabContext(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn *rgn)
{	CRect rc(rect);
	if(ctrl->GetSelectedTab()==tab)
		(ctrl->GetLayout()==TabCtrl::LayoutTop ? rc.top-=2 : rc.bottom+=2);
	TabCtrlStyle_base::DrawTabContext(ctrl,dc,tab,&rc,rgn);
}
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_base::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	if( ctrl->IsTabDisabled(tab) )		// disabled.
		return TabCtrlStyle_base::GetTabTextColor(ctrl,tab);
	if(ctrl->GetSelectedTab()==tab)   // selected.
		return ::GetSysColor(COLOR_BTNTEXT);
	return MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(0,0,0),55);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_classic
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_classic::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool hover)
{	return (hover ? ::GetSysColor(COLOR_3DDKSHADOW) : ::GetSysColor(COLOR_BTNSHADOW));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_classic::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool /*hover*/, bool /*disable*/)
{	return ::GetSysColor(COLOR_WINDOW);
}
// 
COLORREF TabCtrlStyle_VS2008_bars_classic::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? 
		MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(255,255,255),75) :
		MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(255,255,255),55));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_blue
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_blue::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool hover)
{	return (hover ? RGB(60,127,177) : RGB(137,140,149));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_blue::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(250,253,254) : RGB(252,252,252));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_blue::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(167,217,245) : RGB(207,207,207));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_silver
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_silver::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool hover)
{	return (hover ? RGB(119,119,146) : RGB(137,140,149));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_silver::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(247,247,253) : RGB(252,252,252));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_silver::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(180,179,202) : RGB(207,207,207));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_bars_silver::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
	{	FillSolidRect(dc,rect,RGB(255,227,173));
		DrawRect(dc,rect,RGB(74,73,107));
	}
	else if(hover && pushed)
	{	FillSolidRect(dc,rect,RGB(255,182,115));
		DrawRect(dc,rect,RGB(74,73,107));
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_olive
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_olive::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool hover)
{	return (hover ? RGB(143,158,116) : RGB(137,140,149));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_olive::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(250,251,247) : RGB(252,252,252));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_olive::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(182,198,141) : RGB(207,207,207));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_bars_olive::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
	{	FillSolidRect(dc,rect,RGB(255,227,173));
		DrawRect(dc,rect,RGB(118,128,95));
	}
	else if(hover && pushed)
	{	FillSolidRect(dc,rect,RGB(255,182,115));
		DrawRect(dc,rect,RGB(118,128,95));
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_custom1_base
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
CRect TabCtrlStyle_VS2008_bars_custom1_base::GetControlAreaPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(0,1,0,0) : CRect(0,0,0,1));
}
// 
CRect TabCtrlStyle_VS2008_bars_custom1_base::GetWindowsAreaPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	const bool bHideSingleTab = (ctrl->GetNumberTabs()==1 && ctrl->IsHideSingleTab());
	if(!bHideSingleTab)
		return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(1,0,1,1) : CRect(1,1,1,0));
	return CRect(1,1,1,1);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_bars_custom1_base::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	FillSolidRect(dc,rect, GetCtrlAreaBackColor(ctrl) );
		// 
	const COLORREF clrBorder = GetTabBorderColor(ctrl,false);
		// 
	if(ctrl->GetLayout()==TabCtrl::LayoutTop)
		DrawLine(dc,rect->left,rect->bottom-1,rect->right,rect->bottom-1, clrBorder );
	else
		DrawLine(dc,rect->left,rect->top,rect->right,rect->top, clrBorder );
}
// 
void TabCtrlStyle_VS2008_bars_custom1_base::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	const int count = ctrl->GetNumberTabs();
		// 
	if(count==0 || ctrl->GetSelectedTab()==nullptr)
		TabCtrlStyle_base::DrawWindowsAreaBack(ctrl,dc,rect);
	else
	{	const COLORREF clrBorder = GetTabBorderColor(ctrl,false);
		const bool bHideSingleTab = (count==1 && ctrl->IsHideSingleTab());
			// 
		if(bHideSingleTab)
			DrawRect(dc,rect,clrBorder);
		else
		{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
				// 
			CPen penBorder(PS_SOLID,1,clrBorder);
			CPen *pOldPen = dc->SelectObject(&penBorder);
				// 
			if(top)
			{	dc->MoveTo(rect->left,rect->top);
				dc->LineTo(rect->left,rect->bottom-1);
				dc->LineTo(rect->right-1,rect->bottom-1);
				dc->LineTo(rect->right-1,rect->top-1);
			}
			else
			{	dc->MoveTo(rect->left,rect->bottom-1);
				dc->LineTo(rect->left,rect->top);
				dc->LineTo(rect->right-1,rect->top);
				dc->LineTo(rect->right-1,rect->bottom);
			}
			dc->SelectObject(pOldPen);
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_classic_custom1
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_classic_custom1::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool hover)
{	return (hover ? ::GetSysColor(COLOR_3DDKSHADOW) : ::GetSysColor(COLOR_BTNSHADOW));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_classic_custom1::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool /*hover*/, bool /*disable*/)
{	return ::GetSysColor(COLOR_WINDOW);
}
// 
COLORREF TabCtrlStyle_VS2008_bars_classic_custom1::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? 
		MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(255,255,255),75) :
		MixingColors(::GetSysColor(COLOR_BTNSHADOW),RGB(255,255,255),55));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_blue
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_blue_custom1::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool hover)
{	return (hover ? RGB(60,127,177) : RGB(137,140,149));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_blue_custom1::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(250,253,254) : RGB(252,252,252));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_blue_custom1::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(167,217,245) : RGB(207,207,207));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_silver
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_silver_custom1::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool hover)
{	return (hover ? RGB(119,119,146) : RGB(137,140,149));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_silver_custom1::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(247,247,253) : RGB(252,252,252));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_silver_custom1::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(180,179,202) : RGB(207,207,207));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_bars_silver_custom1::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
	{	FillSolidRect(dc,rect,RGB(255,227,173));
		DrawRect(dc,rect,RGB(74,73,107));
	}
	else if(hover && pushed)
	{	FillSolidRect(dc,rect,RGB(255,182,115));
		DrawRect(dc,rect,RGB(74,73,107));
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2008_bars_olive
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2008_bars_olive_custom1::GetTabBorderColor(TabCtrl const * /*ctrl*/, bool hover)
{	return (hover ? RGB(143,158,116) : RGB(137,140,149));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_olive_custom1::GetTabGradientLightColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(250,251,247) : RGB(252,252,252));
}
// 
COLORREF TabCtrlStyle_VS2008_bars_olive_custom1::GetTabGradientDarkColor(TabCtrl const * /*ctrl*/, bool hover, bool disable)
{	return ((hover && !disable) ? RGB(182,198,141) : RGB(207,207,207));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2008_bars_olive_custom1::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
	{	FillSolidRect(dc,rect,RGB(255,227,173));
		DrawRect(dc,rect,RGB(118,128,95));
	}
	else if(hover && pushed)
	{	FillSolidRect(dc,rect,RGB(255,182,115));
		DrawRect(dc,rect,RGB(118,128,95));
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2010_client
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrlStyle_VS2010_client::GetBorderWidth(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return 1;
}
// 
CRect TabCtrlStyle_VS2010_client::GetControlAreaPadding(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return CRect(0,0,3,0);
}
// 
CRect TabCtrlStyle_VS2010_client::GetWindowsAreaPadding(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return CRect(0,4,0,4);
}
// 
CRect TabCtrlStyle_VS2010_client::GetTabPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? 
		CRect(5,1/*border*/+3,5,3) : CRect(5,3,5,3+1/*border*/));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2010_client::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	FillSolidRect(dc,rect, GetCtrlAreaBackColor(ctrl) );
}
// 
void TabCtrlStyle_VS2010_client::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	if( !ctrl->IsTabDisabled(tab) )
	{	const bool active = ctrl->IsActive();
		const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		const bool select = (ctrl->GetSelectedTab()==tab);
		const bool hover = (ctrl->GetTabUnderCursor()==tab);
			// 
		DrawTabBack(ctrl,dc,rect,top,active,select,hover);
	}
}
// 
void TabCtrlStyle_VS2010_client::DrawTabBack(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool top, bool active, bool select, bool hover)
{	CRect rc(rect);
		// 
	if(top)
	{	if(select)
		{	if(active)
			{	rc.top = (rc.top + rc.bottom)/2;
				FillSolidRect(dc,&rc,RGB(255,232,166));   // bottom path.
				rc.bottom = rc.top;
				rc.top = rect->top;
				DrawGradient(dc,&rc,false,RGB(255,252,242),RGB(255,243,207));   // top path.
					// left corner.
				dc->SetPixelV(rc.left,rc.top,RGB(94,109,133));
				dc->SetPixelV(rc.left+1,rc.top,RGB(195,198,199));
				dc->SetPixelV(rc.left,rc.top+1,RGB(195,197,197));
					// right corner.
				dc->SetPixelV(rc.right-1,rc.top,RGB(94,109,133));
				dc->SetPixelV(rc.right-2,rc.top,RGB(195,198,199));
				dc->SetPixelV(rc.right-1,rc.top+1,RGB(195,197,197));
			}
			else
			{	rc.top = (rc.top + rc.bottom)/2;
				FillSolidRect(dc,&rc,RGB(206,212,223));   // bottom path.
				rc.bottom = rc.top;
				rc.top = rect->top;
				DrawGradient(dc,&rc,false,RGB(251,252,252),RGB(215,220,228));   // top path.
					// left corner.
				dc->SetPixelV(rc.left,rc.top,RGB(93,109,135));
				dc->SetPixelV(rc.left+1,rc.top,RGB(192,198,206));
				dc->SetPixelV(rc.left,rc.top+1,RGB(190,196,204));
					// right corner.
				dc->SetPixelV(rc.right-1,rc.top,RGB(93,109,135));
				dc->SetPixelV(rc.right-2,rc.top,RGB(192,198,206));
				dc->SetPixelV(rc.right-1,rc.top+1,RGB(190,196,204));
			}
		}
		else if(hover)
		{		// draw border.
			CPen pen(PS_SOLID,1,RGB(155,167,183));
			CPen *pOldPen = dc->SelectObject(&pen);
			dc->MoveTo(rc.left,rc.bottom);
			dc->LineTo(rc.left,rc.top);
			dc->LineTo(rc.right-1,rc.top);
			dc->LineTo(rc.right-1,rc.bottom);
			dc->SelectObject(pOldPen);
				// draw back.
			rc.DeflateRect(1,1,1,0);
			DrawGradient(dc,&rc,false,RGB(111,119,118),RGB(79,95,116));
				// 
			rc.InflateRect(1,1,1,0);
				// left corner.
			dc->SetPixelV(rc.left,rc.top,RGB(50,65,93));
			dc->SetPixelV(rc.left+1,rc.top,RGB(120,135,157));
			dc->SetPixelV(rc.left,rc.top+1,RGB(117,130,150));
			dc->SetPixelV(rc.left+1,rc.top+1,RGB(115,126,139));
				// right corner.
			dc->SetPixelV(rc.right-1,rc.top,RGB(50,65,93));
			dc->SetPixelV(rc.right-2,rc.top,RGB(120,135,157));
			dc->SetPixelV(rc.right-1,rc.top+1,RGB(117,130,150));
			dc->SetPixelV(rc.right-2,rc.top+1,RGB(115,126,139));
		}
	}
	else	// bottom.
	{	if(select)
		{	if(active)
			{	rc.bottom = (rc.top + rc.bottom)/2;
				FillSolidRect(dc,&rc,RGB(255,232,166));   // top path.
				rc.top = rc.bottom;
				rc.bottom = rect->bottom;
				DrawGradient(dc,&rc,false,RGB(255,243,207),RGB(255,252,242));   // bottom path.
					// left corner.
				dc->SetPixelV(rc.left,rc.bottom-1,RGB(94,109,133));
				dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(195,198,199));
				dc->SetPixelV(rc.left,rc.bottom-2,RGB(195,197,197));
					// right corner.
				dc->SetPixelV(rc.right-1,rc.bottom-1,RGB(94,109,133));
				dc->SetPixelV(rc.right-2,rc.bottom-1,RGB(195,198,199));
				dc->SetPixelV(rc.right-1,rc.bottom-2,RGB(195,197,197));
			}
			else
			{	rc.bottom = (rc.top + rc.bottom)/2;
				FillSolidRect(dc,&rc,RGB(206,212,223));   // bottom path.
				rc.top = rc.bottom;
				rc.bottom = rect->bottom;
				DrawGradient(dc,&rc,false,RGB(215,220,228),RGB(251,252,252));   // top path.
					// left corner.
				dc->SetPixelV(rc.left,rc.bottom-1,RGB(93,109,135));
				dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(192,198,206));
				dc->SetPixelV(rc.left,rc.bottom-2,RGB(190,196,204));
					// right corner.
				dc->SetPixelV(rc.right-1,rc.bottom-1,RGB(93,109,135));
				dc->SetPixelV(rc.right-2,rc.bottom-1,RGB(192,198,206));
				dc->SetPixelV(rc.right-1,rc.bottom-2,RGB(190,196,204));
			}
		}
		else if(hover)
		{		// draw border.
			CPen pen(PS_SOLID,1,RGB(155,167,183));
			CPen *pOldPen = dc->SelectObject(&pen);
			dc->MoveTo(rc.left,rc.top);
			dc->LineTo(rc.left,rc.bottom-1);
			dc->LineTo(rc.right-1,rc.bottom-1);
			dc->LineTo(rc.right-1,rc.top);
			dc->SelectObject(pOldPen);
				// draw back.
			rc.DeflateRect(1,0,1,1);
			DrawGradient(dc,&rc,false,RGB(79,95,116),RGB(111,119,118));
				// 
			rc.InflateRect(1,0,1,1);
				// left corner.
			dc->SetPixelV(rc.left,rc.bottom-1,RGB(50,65,93));
			dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(120,135,157));
			dc->SetPixelV(rc.left,rc.bottom-2,RGB(117,130,150));
			dc->SetPixelV(rc.left+1,rc.bottom-2,RGB(115,126,139));
				// right corner.
			dc->SetPixelV(rc.right-1,rc.bottom-1,RGB(50,65,93));
			dc->SetPixelV(rc.right-2,rc.bottom-1,RGB(120,135,157));
			dc->SetPixelV(rc.right-1,rc.bottom-2,RGB(117,130,150));
			dc->SetPixelV(rc.right-2,rc.bottom-2,RGB(115,126,139));
		}
	}
}
// 
void TabCtrlStyle_VS2010_client::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
		TabCtrlStyle_base::DrawWindowsAreaBack(ctrl,dc,rect);
	else
	{	const bool active = ctrl->IsActive();
		const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		DrawWindowsAreaBack(ctrl,dc,rect,top,active);
	}
}
// 
void TabCtrlStyle_VS2010_client::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool top, bool active)
{	CRect rc;
	const TabCtrl::HTAB firstTab = (ctrl->GetNumberTabs()>0 ? ctrl->GetTabHandleByIndex(0) : nullptr);
	const bool selectFirstTab = (ctrl->GetSelectedTab()==firstTab);
	const bool scaleMode = (ctrl->GetBehavior()==TabCtrl::BehaviorScale);
	const COLORREF clr = (active ? RGB(255,232,166) : RGB(206,212,223));
		// 
		// draw top.
	rc.SetRect(rect->left,rect->top,rect->right,rect->top+4);
	FillSolidRect(dc,&rc,clr);
		// 
	if(!top || scaleMode)
	{		// left corner.
		if(!top || !selectFirstTab || (ctrl->GetNumberTabs()==1 && ctrl->IsHideSingleTab()))
			if(top && ctrl->GetTabUnderCursor()==firstTab)   // tab is highlight.
			{	if(active)
				{	dc->SetPixelV(rc.left,rc.top,RGB(175,180,180));
					dc->SetPixelV(rc.left+1,rc.top,RGB(205,194,152));
					dc->SetPixelV(rc.left,rc.top+1,RGB(227,214,171));
				}
				else
				{	dc->SetPixelV(rc.left,rc.top,RGB(165,176,191));
					dc->SetPixelV(rc.left+1,rc.top,RGB(170,179,193));
					dc->SetPixelV(rc.left,rc.top+1,RGB(192,199,212));
				}
			}
			else	// tab isn't highlight.
				if(active)
				{	dc->SetPixelV(rc.left,rc.top,RGB(86,95,105));
					dc->SetPixelV(rc.left+1,rc.top,RGB(198,188,149));
					dc->SetPixelV(rc.left,rc.top+1,RGB(195,184,144));
				}
				else
				{	dc->SetPixelV(rc.left,rc.top,RGB(76,91,116));
					dc->SetPixelV(rc.left+1,rc.top,RGB(163,173,190));
					dc->SetPixelV(rc.left,rc.top+1,RGB(160,169,185));
				}
			// right corner.
		if(active)
		{	dc->SetPixelV(rc.right-1,rc.top,RGB(100,106,109));
			dc->SetPixelV(rc.right-2,rc.top,RGB(205,192,148));
			dc->SetPixelV(rc.right-1,rc.top+1,RGB(195,184,144));
		}
		else
		{	dc->SetPixelV(rc.right-1,rc.top,RGB(87,100,124));
			dc->SetPixelV(rc.right-2,rc.top,RGB(168,176,192));
			dc->SetPixelV(rc.right-1,rc.top+1,RGB(160,169,185));
		}
	}
		// 
		// draw bottom.
	rc.SetRect(rect->left,rect->bottom-4,rect->right,rect->bottom);
	FillSolidRect(dc,&rc,clr);
		// 
	if(top || scaleMode)
	{		// left corner.
		if(top || !selectFirstTab || (ctrl->GetNumberTabs()==1 && ctrl->IsHideSingleTab()))
			if(!top && ctrl->GetTabUnderCursor()==firstTab)   // tab is highlight.
			{	if(active)
				{	dc->SetPixelV(rc.left,rc.bottom-1,RGB(175,180,180));
					dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(205,194,152));
					dc->SetPixelV(rc.left,rc.bottom-2,RGB(227,214,171));
				}
				else
				{	dc->SetPixelV(rc.left,rc.bottom-1,RGB(165,176,191));
					dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(170,179,193));
					dc->SetPixelV(rc.left,rc.bottom-2,RGB(192,199,212));
				}
			}
			else	// tab isn't highlight.
				if(active)
				{	dc->SetPixelV(rc.left,rc.bottom-1,RGB(86,95,105));
					dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(198,188,149));
					dc->SetPixelV(rc.left,rc.bottom-2,RGB(195,184,144));
				}
				else
				{	dc->SetPixelV(rc.left,rc.bottom-1,RGB(76,91,116));
					dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(163,173,190));
					dc->SetPixelV(rc.left,rc.bottom-2,RGB(160,169,185));
				}
			// right corner.
		if(active)
		{	dc->SetPixelV(rc.right-1,rc.bottom-1,RGB(100,106,109));
			dc->SetPixelV(rc.right-2,rc.bottom-1,RGB(205,192,148));
			dc->SetPixelV(rc.right-1,rc.bottom-2,RGB(195,184,144));
		}
		else
		{	dc->SetPixelV(rc.right-1,rc.bottom-1,RGB(87,100,124));
			dc->SetPixelV(rc.right-2,rc.bottom-1,RGB(168,176,192));
			dc->SetPixelV(rc.right-1,rc.bottom-2,RGB(160,169,185));
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2010_client::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed || (hover && pushed))
	{	CRect rc(rect);
		DrawRect(dc,&rc,RGB(229,195,101));
			// 
		rc.DeflateRect(1,1);
		FillSolidRect(dc,&rc,(hover && pushed ? RGB(255,232,166) : RGB(255,252,244)));
	}
}
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2010_client::GetBorderColor(TabCtrl const * /*ctrl*/)
{	return RGB(46,64,94);
}
// 
COLORREF TabCtrlStyle_VS2010_client::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(46,64,94);
}
// 
COLORREF TabCtrlStyle_VS2010_client::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	return (ctrl->GetSelectedTab()==tab ? RGB(13,0,5) : RGB(248,255,255));
}
// 
COLORREF TabCtrlStyle_VS2010_client::GetEmptyWndsAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(46,64,94);
}
// 
COLORREF TabCtrlStyle_VS2010_client::GetButtonCloseColor(TabCtrl const * /*ctrl*/, bool hover, bool pushed)
{	return ((hover || pushed) ? RGB(0,0,0) : RGB(206,212,221));
}
COLORREF TabCtrlStyle_VS2010_client::GetButtonMenuColor(TabCtrl const * /*ctrl*/, bool hover, bool pushed)
{	return ((hover || pushed) ? RGB(0,0,0) : RGB(206,212,221));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2010_client_custom1
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2010_client_custom1::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	if( !ctrl->IsTabDisabled(tab) )
	{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		const bool select = (ctrl->GetSelectedTab()==tab);
		const bool hover = (ctrl->GetTabUnderCursor()==tab);
		TabCtrlStyle_VS2010_client::DrawTabBack(ctrl,dc,rect,top,true,select,hover);
	}
}
// 
void TabCtrlStyle_VS2010_client_custom1::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
		TabCtrlStyle_base::DrawWindowsAreaBack(ctrl,dc,rect);
	else
	{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		TabCtrlStyle_VS2010_client::DrawWindowsAreaBack(ctrl,dc,rect,top,true);
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2010_client_custom2
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2010_client_custom2::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	if( !ctrl->IsTabDisabled(tab) )
	{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		const bool select = (ctrl->GetSelectedTab()==tab);
		const bool hover = (ctrl->GetTabUnderCursor()==tab);
		TabCtrlStyle_VS2010_client::DrawTabBack(ctrl,dc,rect,top,false,select,hover);
	}
}
// 
void TabCtrlStyle_VS2010_client_custom2::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
		TabCtrlStyle_base::DrawWindowsAreaBack(ctrl,dc,rect);
	else
	{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		TabCtrlStyle_VS2010_client::DrawWindowsAreaBack(ctrl,dc,rect,top,false);
	}
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2010_bars
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
int TabCtrlStyle_VS2010_bars::GetBorderWidth(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return 1;
}
// 
CRect TabCtrlStyle_VS2010_bars::GetControlAreaPadding(TabCtrl const * /*ctrl*/, TabCtrl::IRecalc * /*base*/)
{	return CRect(0,0,3,0);
}
// 
CRect TabCtrlStyle_VS2010_bars::GetTabPadding(TabCtrl const *ctrl, TabCtrl::IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? 
		CRect(5,1/*border*/+1,5,2) : CRect(5,2,5,1+1/*border*/));
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2010_bars::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	FillSolidRect(dc,rect, GetCtrlAreaBackColor(ctrl) );
}
// 
void TabCtrlStyle_VS2010_bars::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	if( !ctrl->IsTabDisabled(tab) )
	{	const bool top = (ctrl->GetLayout()==TabCtrl::LayoutTop);
		const bool select = (ctrl->GetSelectedTab()==tab);
		const bool hover = (ctrl->GetTabUnderCursor()==tab);
			// 
		CRect rc(rect);
			// 
		if(top)
		{	if(select)
			{	FillSolidRect(dc,&rc,RGB(255,255,255));   // back.
					// left corner.
				dc->SetPixelV(rc.left,rc.top,RGB(103,116,138));
				dc->SetPixelV(rc.left+1,rc.top,RGB(221,224,228));
				dc->SetPixelV(rc.left,rc.top+1,RGB(197,202,210));
					// right corner.
				dc->SetPixelV(rc.right-1,rc.top,RGB(103,116,138));
				dc->SetPixelV(rc.right-2,rc.top,RGB(221,224,228));
				dc->SetPixelV(rc.right-1,rc.top+1,RGB(197,202,210));
			}
			else if(hover)
			{		// draw border.
				CPen pen(PS_SOLID,1,RGB(155,167,183));
				CPen *pOldPen = dc->SelectObject(&pen);
				dc->MoveTo(rc.left,rc.top);
				dc->LineTo(rc.left,rc.bottom-1);
				dc->LineTo(rc.right-1,rc.bottom-1);
				dc->LineTo(rc.right-1,rc.top);
				dc->LineTo(rc.left,rc.top);
				dc->SelectObject(pOldPen);
					// draw back.
				rc.DeflateRect(1,1);
				DrawGradient(dc,&rc,false,RGB(76,92,116),RGB(111,119,118));
					// 
				rc.InflateRect(1,1);
					// left corner.
				dc->SetPixelV(rc.left,rc.top,RGB(50,65,93));
				dc->SetPixelV(rc.left+1,rc.top,RGB(120,135,157));
				dc->SetPixelV(rc.left,rc.top+1,RGB(117,130,150));
				dc->SetPixelV(rc.left+1,rc.top+1,RGB(115,126,139));
					// right corner.
				dc->SetPixelV(rc.right-1,rc.top,RGB(50,65,93));
				dc->SetPixelV(rc.right-2,rc.top,RGB(120,135,157));
				dc->SetPixelV(rc.right-1,rc.top+1,RGB(117,130,150));
				dc->SetPixelV(rc.right-2,rc.top+1,RGB(115,126,139));
			}
		}
		else	// bottom.
		{	if(select)
			{	FillSolidRect(dc,&rc,RGB(255,255,255));   // back.
					// left corner.
				dc->SetPixelV(rc.left,rc.bottom-1,RGB(103,116,138));
				dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(221,224,228));
				dc->SetPixelV(rc.left,rc.bottom-2,RGB(197,202,210));
					// right corner.
				dc->SetPixelV(rc.right-1,rc.bottom-1,RGB(103,116,138));
				dc->SetPixelV(rc.right-2,rc.bottom-1,RGB(221,224,228));
				dc->SetPixelV(rc.right-1,rc.bottom-2,RGB(197,202,210));
			}
			else if(hover)
			{		// draw border.
				CPen pen(PS_SOLID,1,RGB(155,167,183));
				CPen *pOldPen = dc->SelectObject(&pen);
				dc->MoveTo(rc.left,rc.top);
				dc->LineTo(rc.left,rc.bottom-1);
				dc->LineTo(rc.right-1,rc.bottom-1);
				dc->LineTo(rc.right-1,rc.top);
				dc->LineTo(rc.left,rc.top);
				dc->SelectObject(pOldPen);
					// draw back.
				rc.DeflateRect(1,1);
				DrawGradient(dc,&rc,false,RGB(111,119,118),RGB(76,92,116));
					// 
				rc.InflateRect(1,1);
					// left corner.
				dc->SetPixelV(rc.left,rc.bottom-1,RGB(50,65,93));
				dc->SetPixelV(rc.left+1,rc.bottom-1,RGB(120,135,157));
				dc->SetPixelV(rc.left,rc.bottom-2,RGB(117,130,150));
				dc->SetPixelV(rc.left+1,rc.bottom-2,RGB(115,126,139));
					// right corner.
				dc->SetPixelV(rc.right-1,rc.bottom-1,RGB(50,65,93));
				dc->SetPixelV(rc.right-2,rc.bottom-1,RGB(120,135,157));
				dc->SetPixelV(rc.right-1,rc.bottom-2,RGB(117,130,150));
				dc->SetPixelV(rc.right-2,rc.bottom-2,RGB(115,126,139));
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2010_bars::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed || (hover && pushed))
	{	CRect rc(rect);
		DrawRect(dc,&rc,RGB(229,195,101));
			// 
		rc.DeflateRect(1,1);
		FillSolidRect(dc,&rc,(hover && pushed ? RGB(255,232,166) : RGB(255,252,244)));
	}
}
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2010_bars::GetBorderColor(TabCtrl const * /*ctrl*/)
{	return RGB(46,64,94);
}
// 
COLORREF TabCtrlStyle_VS2010_bars::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(46,64,94);
}
// 
COLORREF TabCtrlStyle_VS2010_bars::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	return (ctrl->GetSelectedTab()==tab ? RGB(13,0,5) : RGB(248,255,255));
}
// 
COLORREF TabCtrlStyle_VS2010_bars::GetEmptyWndsAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(46,64,94);
}
// 
COLORREF TabCtrlStyle_VS2010_bars::GetButtonCloseColor(TabCtrl const * /*ctrl*/, bool hover, bool pushed)
{	return (hover || pushed ? RGB(0,0,0) : RGB(206,212,221));
}
COLORREF TabCtrlStyle_VS2010_bars::GetButtonMenuColor(TabCtrl const * /*ctrl*/, bool hover, bool pushed)
{	return (hover || pushed ? RGB(0,0,0) : RGB(206,212,221));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2019_client_base
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
int TabCtrlStyle_VS2019_client_base::GetBorderWidth(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return 0;
}
// 
CRect TabCtrlStyle_VS2019_client_base::GetControlAreaPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(1,0,1,2) : CRect(1,2,1,0));
}
// 
CRect TabCtrlStyle_VS2019_client_base::GetWindowsAreaPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(1,0,1,1) : CRect(1,1,1,0));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_client_base::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	FillSolidRect(dc,rect, GetCtrlAreaBackColor(ctrl) );
		// 
	const CRect rcCtrlAreaPadding = GetControlAreaPadding(ctrl,nullptr);
	CRect rc = (ctrl->GetLayout()==TabCtrl::LayoutTop ?
		CRect(rect->left+rcCtrlAreaPadding.left,rect->bottom-2,rect->right-rcCtrlAreaPadding.right,rect->bottom) :
		CRect(rect->left+rcCtrlAreaPadding.left,rect->top,rect->right-rcCtrlAreaPadding.right,rect->top+2));
	const COLORREF clrLine = (ctrl->GetSelectedTab() && (!ctrl->IsWatchCtrlActivity() || ctrl->IsActive()) ? 
		GetTabSelectedActiveBackColor(ctrl) : GetTabSelectedPassiveBackColor(ctrl));
	FillSolidRect(dc,&rc,clrLine);
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2019_client_base::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	CPen pen(PS_SOLID,1, GetTabSelectedPassiveBackColor(ctrl) );
	CPen *pOldPen = dc->SelectObject(&pen);
		// 
	const bool top = ctrl->GetLayout()==TabCtrl::LayoutTop;
	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
	{	CRect rc(rect);
		(top ? rc.DeflateRect(1,0,1,1) : rc.DeflateRect(1,1,1,0));
		FillSolidRect(dc,&rc, GetEmptyWndsAreaBackColor(ctrl) );
	}	
		// 
	if(top)
	{	dc->MoveTo(rect->left,rect->top);
		dc->LineTo(rect->left,rect->bottom-1);
		dc->LineTo(rect->right-1,rect->bottom-1);
		dc->LineTo(rect->right-1,rect->top-1);
	}
	else
	{	dc->MoveTo(rect->left,rect->bottom-1);
		dc->LineTo(rect->left,rect->top);
		dc->LineTo(rect->right-1,rect->top);
		dc->LineTo(rect->right-1,rect->bottom);
	}
		// 
	dc->SelectObject(pOldPen);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_base::GetTabSelectedBackColor(TabCtrl const *ctrl)
{	return (!ctrl->IsWatchCtrlActivity() || ctrl->IsActive() ? 
		GetTabSelectedActiveBackColor(ctrl) : GetTabSelectedPassiveBackColor(ctrl));
}
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_client_base::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	if(ctrl->GetSelectedTab()==tab)
		FillSolidRect(dc,rect, GetTabSelectedBackColor(ctrl) );
	else
		if(ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab))   // highlighted tab.
		{	FillSolidRect(dc,rect, GetTabHighlightedBackColor(ctrl) );
			(ctrl->GetLayout()==TabCtrl::LayoutTop ?
				DrawLine(dc,rect->left,rect->bottom-1,rect->right,rect->bottom-1, GetCtrlAreaBackColor(ctrl) ) :
				DrawLine(dc,rect->left,rect->top,rect->right,rect->top, GetCtrlAreaBackColor(ctrl) ));
		}
}
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_client_base::DrawTabContext(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn *rgn)
{	if(ctrl->GetSelectedTab()==tab)
	{	CRect rc(rect);
		(ctrl->GetLayout()==TabCtrl::LayoutTop ? rc.OffsetRect(0,1) : rc.OffsetRect(0,-1));
		TabCtrlStyle_base::DrawTabContext(ctrl,dc,tab,&rc,rgn);
	}
	else
		TabCtrlStyle_base::DrawTabContext(ctrl,dc,tab,rect,rgn);
}
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_client_base::DrawButtonFrame(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
		FillSolidRect(dc,rect, GetButtonFrameHighlightBackColor(ctrl) );
	else if(hover && pushed)
		FillSolidRect(dc,rect, GetTabSelectedActiveBackColor(ctrl) );
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2019_client_base::GetButtonFrameHighlightBackColor(TabCtrl const *ctrl)
{	return GetCtrlAreaBackColor(ctrl);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2019_client_light
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_light::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(238,238,242);
}
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_light::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	if( ctrl->IsTabDisabled(tab) )
		return RGB(109,109,109);
	if(ctrl->GetSelectedTab()==tab)
		return (!ctrl->IsWatchCtrlActivity() || ctrl->IsActive() ? RGB(255,255,255) : RGB(109,109,109));
	return (ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab) ?   // highlighted tab.
		RGB(255,255,255) : RGB(0,0,0));
}
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_light::GetButtonCloseColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return (ctrl->GetSelectedTab() ? GetButtonMenuColor(ctrl,hover,pushed) : RGB(160,160,160));
}
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_light::GetButtonMenuColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	if(!hover && !pushed)
		return RGB(113,113,113);
	return (hover!=pushed ? GetTabSelectedActiveBackColor(ctrl) : RGB(255,255,255));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_light::GetTabHighlightedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(28,151,234);
}
// 
COLORREF TabCtrlStyle_VS2019_client_light::GetTabSelectedActiveBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(0,122,204);
}
COLORREF TabCtrlStyle_VS2019_client_light::GetTabSelectedPassiveBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(204,206,219);
}
// 
COLORREF TabCtrlStyle_VS2019_client_light::GetButtonFrameHighlightBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(201,222,245);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2019_client_dark
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_dark::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(45,45,48);
}
// 
COLORREF TabCtrlStyle_VS2019_client_dark::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	return (!ctrl->IsTabDisabled(tab) ? RGB(255,255,255) : RGB(160,160,160));
}
// 
COLORREF TabCtrlStyle_VS2019_client_dark::GetButtonCloseColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return (ctrl->GetSelectedTab() ? GetButtonMenuColor(ctrl,hover,pushed) : RGB(160,160,160));
}
COLORREF TabCtrlStyle_VS2019_client_dark::GetButtonMenuColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	if(!hover && !pushed)
		return RGB(241,241,241);
	return (hover!=pushed ? GetTabSelectedActiveBackColor(ctrl) : RGB(255,255,255));
}
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_dark::GetTabHighlightedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(28,151,234);
}
// 
COLORREF TabCtrlStyle_VS2019_client_dark::GetTabSelectedActiveBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(0,122,204);
}
COLORREF TabCtrlStyle_VS2019_client_dark::GetTabSelectedPassiveBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(63,63,70);
}
// 
COLORREF TabCtrlStyle_VS2019_client_dark::GetButtonFrameHighlightBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(62,62,64);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2019_client_blue
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
int TabCtrlStyle_VS2019_client_blue::GetBorderWidth(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return 1;
}
//
CRect TabCtrlStyle_VS2019_client_blue::GetControlAreaPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(0,0,0,2) : CRect(0,2,0,0));
}
// 
CRect TabCtrlStyle_VS2019_client_blue::GetWindowsAreaPadding(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(0,0,0,0);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_client_blue::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
		FillSolidRect(dc,rect, GetEmptyWndsAreaBackColor(ctrl) );
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_blue::GetBorderColor(TabCtrl const * /*ctrl*/)
{	return RGB(93,107,153);
}
//
COLORREF TabCtrlStyle_VS2019_client_blue::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(93,107,153);
}
// 
COLORREF TabCtrlStyle_VS2019_client_blue::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	if( ctrl->IsTabDisabled(tab) )
		return RGB(197,203,224);
	if(ctrl->GetSelectedTab()==tab)
		return (!ctrl->IsWatchCtrlActivity() || ctrl->IsActive() ? RGB(30,30,30) : RGB(1,36,96));
	return (ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab) ?   // highlighted tab.
		RGB(33,52,73) : RGB(255,255,255));
}
// 
COLORREF TabCtrlStyle_VS2019_client_blue::GetButtonCloseColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return (ctrl->GetSelectedTab() ? GetButtonMenuColor(ctrl,hover,pushed) : RGB(197,203,224));
}
COLORREF TabCtrlStyle_VS2019_client_blue::GetButtonMenuColor(TabCtrl const * /*ctrl*/, bool hover, bool pushed)
{	return (!hover && !pushed ? RGB(217,224,248) : RGB(255,255,255));
}
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_client_blue::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn *rgn)
{	if(ctrl->GetSelectedTab()==tab)
		TabCtrlStyle_VS2019_client_base::DrawTabBack(ctrl,dc,tab,rect,rgn);
	else
	{	const bool highlighted = (ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab));   // highlighted tab.
		FillSolidRect(dc,rect, (highlighted ? GetTabHighlightedBackColor(ctrl) : GetTabNormalBackColor(ctrl)));
		DrawRect(dc,rect, GetCtrlAreaBackColor(ctrl) );
	}
}
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_client_blue::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC * /*dc*/, CRect const * /*rect*/, bool /*hover*/, bool /*pushed*/)
{
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_blue::GetTabHighlightedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(187,198,241);
}
// 
COLORREF TabCtrlStyle_VS2019_client_blue::GetTabSelectedActiveBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(245,204,132);
}
COLORREF TabCtrlStyle_VS2019_client_blue::GetTabSelectedPassiveBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(204,213,240);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_client_blue::GetTabNormalBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(59,79,129);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2019_bars_base
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
int TabCtrlStyle_VS2019_bars_base::GetBorderWidth(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return 0;
}
/////////////////////////////////////////////////////////////////////////////
//
CRect TabCtrlStyle_VS2019_bars_base::GetControlAreaPadding(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(0,0,0,0);
}
/////////////////////////////////////////////////////////////////////////////
//
CRect TabCtrlStyle_VS2019_bars_base::GetWindowsAreaPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(1,0,1,1) : CRect(1,1,1,0));
}
/////////////////////////////////////////////////////////////////////////////
//
CRect TabCtrlStyle_VS2019_bars_base::GetTabPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	Gdiplus::Bitmap *normal, *disable;
	ctrl->GetImages(&normal/*out*/,&disable/*out*/);
	return (normal || disable ? CRect(6,3,5,3) : CRect(6,4,5,4));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_bars_base::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	FillSolidRect(dc,rect, GetCtrlAreaBackColor(ctrl) );
		// 
	(ctrl->GetLayout()==TabCtrl::LayoutTop ?
		DrawLine(dc, rect->left,rect->bottom-1,rect->right,rect->bottom-1, GetTabBorderColor(ctrl) ) :
		DrawLine(dc, rect->left,rect->top,rect->right,rect->top, GetTabBorderColor(ctrl) ));
}
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_bars_base::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	CPen pen(PS_SOLID,1, GetTabBorderColor(ctrl) );
	CPen *pOldPen = dc->SelectObject(&pen);
		// 
	const bool top = ctrl->GetLayout()==TabCtrl::LayoutTop;
	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
	{	CRect rc(rect);
		(top ? rc.DeflateRect(1,0,1,1) : rc.DeflateRect(1,1,1,0));
		FillSolidRect(dc,&rc, GetEmptyWndsAreaBackColor(ctrl) );
	}	
		// 
	if(top)
	{	dc->MoveTo(rect->left,rect->top);
		dc->LineTo(rect->left,rect->bottom-1);
		dc->LineTo(rect->right-1,rect->bottom-1);
		dc->LineTo(rect->right-1,rect->top-1);
	}
	else
	{	dc->MoveTo(rect->left,rect->bottom-1);
		dc->LineTo(rect->left,rect->top);
		dc->LineTo(rect->right-1,rect->top);
		dc->LineTo(rect->right-1,rect->bottom);
	}
		// 
	dc->SelectObject(pOldPen);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2019_bars_base::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	CRect rc(rect);
		// 
	if(ctrl->GetSelectedTab()==tab)
	{	CPen pen(PS_SOLID,1, GetTabBorderColor(ctrl) );
		CPen *pOldPen = dc->SelectObject(&pen);
			// 
		if(ctrl->GetLayout()==TabCtrl::LayoutTop)
		{	++rc.bottom;
			FillSolidRect(dc,&rc, GetTabSelectedBackColor(ctrl) );
				// 
			dc->MoveTo(rect->left,rect->bottom-1);
			dc->LineTo(rect->left,rect->top);
			dc->LineTo(rect->right-1,rect->top);
			dc->LineTo(rect->right-1,rect->bottom);
		}
		else
		{	--rc.top;
			FillSolidRect(dc,&rc, GetTabSelectedBackColor(ctrl) );
				// 
			dc->MoveTo(rect->left,rect->top);
			dc->LineTo(rect->left,rect->bottom-1);
			dc->LineTo(rect->right-1,rect->bottom-1);
			dc->LineTo(rect->right-1,rect->top-1);
		}
			// 
		dc->SelectObject(pOldPen);
	}
	else
		if(ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab))   // highlighted tab.
		{	(ctrl->GetLayout()==TabCtrl::LayoutTop ? --rc.bottom : ++rc.top);
			FillSolidRect(dc,&rc, GetTabHighlightedBackColor(ctrl) );
		}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2019_bars_base::DrawButtonFrame(TabCtrl const *ctrl, CDC *dc, CRect const *rect, bool hover, bool pushed)
{	if(hover!=pushed)
		FillSolidRect(dc,rect, GetButtonFrameBackColorA(ctrl) );
	else if(hover && pushed)
		FillSolidRect(dc,rect, GetButtonFrameBackColorB(ctrl) );
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2019_bars_base::GetButtonFrameBackColorA(TabCtrl const *ctrl)
{	return GetCtrlAreaBackColor(ctrl);
}
// 
COLORREF TabCtrlStyle_VS2019_bars_base::GetButtonFrameBackColorB(TabCtrl const *ctrl)
{	return GetCtrlAreaBackColor(ctrl);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2019_bars_light
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_bars_light::GetTabBorderColor(TabCtrl const * /*ctrl*/)
{	return RGB(204,206,219);
}
// 
COLORREF TabCtrlStyle_VS2019_bars_light::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(238,238,242);
}
// 
COLORREF TabCtrlStyle_VS2019_bars_light::GetTabSelectedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(245,245,245);
}
// 
COLORREF TabCtrlStyle_VS2019_bars_light::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	if( ctrl->IsTabDisabled(tab) )
		return RGB(109,109,109);
	if(ctrl->GetSelectedTab()==tab)
		return (!ctrl->IsWatchCtrlActivity() || ctrl->IsActive() ? RGB(14,112,192) : RGB(109,109,109));
	return (ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab) ?   // highlighted tab.
		RGB(30,30,30) : RGB(68,68,68));
}
// 
COLORREF TabCtrlStyle_VS2019_bars_light::GetButtonCloseColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return (ctrl->GetSelectedTab() ? GetButtonMenuColor(ctrl,hover,pushed) : RGB(160,160,160));
}
COLORREF TabCtrlStyle_VS2019_bars_light::GetButtonMenuColor(TabCtrl const * /*ctrl*/, bool hover, bool pushed)
{	return (!hover && !pushed ? RGB(113,113,113) : RGB(0,122,204));
}
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2019_bars_light::GetTabHighlightedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(201,222,245);
}
// 
COLORREF TabCtrlStyle_VS2019_bars_light::GetButtonFrameBackColorA(TabCtrl const * /*ctrl*/)
{	return RGB(201,222,245);
}
COLORREF TabCtrlStyle_VS2019_bars_light::GetButtonFrameBackColorB(TabCtrl const * /*ctrl*/)
{	return RGB(159,200,244);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2019_bars_dark
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_bars_dark::GetTabBorderColor(TabCtrl const * /*ctrl*/)
{	return RGB(63,63,70);
}
//
COLORREF TabCtrlStyle_VS2019_bars_dark::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(45,45,48);
}
//
COLORREF TabCtrlStyle_VS2019_bars_dark::GetTabSelectedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(37,37,38);
}
//
COLORREF TabCtrlStyle_VS2019_bars_dark::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	if( ctrl->IsTabDisabled(tab) )
		return RGB(160,160,160);
	if(ctrl->GetSelectedTab()==tab)
		return (!ctrl->IsWatchCtrlActivity() || ctrl->IsActive() ? RGB(14,151,221) : RGB(160,160,160));
	return (ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab) ?   // highlighted tab.
		RGB(85,170,255) : RGB(208,208,208));
}
//
COLORREF TabCtrlStyle_VS2019_bars_dark::GetButtonCloseColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return (ctrl->GetSelectedTab() ? GetButtonMenuColor(ctrl,hover,pushed) : RGB(160,160,160));
}
COLORREF TabCtrlStyle_VS2019_bars_dark::GetButtonMenuColor(TabCtrl const * /*ctrl*/, bool hover, bool pushed)
{	return (!hover && !pushed ? RGB(208,208,208) : RGB(14,151,221));
}
/////////////////////////////////////////////////////////////////////////////
// 
COLORREF TabCtrlStyle_VS2019_bars_dark::GetTabHighlightedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(62,62,64);
}
// 
COLORREF TabCtrlStyle_VS2019_bars_dark::GetButtonFrameBackColorA(TabCtrl const * /*ctrl*/)
{	return RGB(62,62,64);
}
COLORREF TabCtrlStyle_VS2019_bars_dark::GetButtonFrameBackColorB(TabCtrl const * /*ctrl*/)
{	return RGB(79,79,81);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// TabCtrlStyle_VS2019_bars_blue
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
int TabCtrlStyle_VS2019_bars_blue::GetBorderWidth(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return 1;
}
//
CRect TabCtrlStyle_VS2019_bars_blue::GetControlAreaPadding(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(0,0,0,0);
}
// 
CRect TabCtrlStyle_VS2019_bars_blue::GetWindowsAreaPadding(TabCtrl const * /*ctrl*/, IRecalc * /*base*/)
{	return CRect(0,0,0,0);
}
// 
CRect TabCtrlStyle_VS2019_bars_blue::GetTabPadding(TabCtrl const *ctrl, IRecalc * /*base*/)
{	Gdiplus::Bitmap *normal, *disable;
	ctrl->GetImages(&normal/*out*/,&disable/*out*/);
	if(normal || disable)
		return CRect(6,3,5,3);
	return (ctrl->GetLayout()==TabCtrl::LayoutTop ? CRect(6,4,5,3) : CRect(6,3,5,4));
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_bars_blue::DrawControlAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	FillSolidRect(dc,rect, GetCtrlAreaBackColor(ctrl) );
}
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_bars_blue::DrawWindowsAreaBack(TabCtrl const *ctrl, CDC *dc, CRect const *rect)
{	if(ctrl->GetNumberTabs()==0 || ctrl->GetSelectedTab()==nullptr)
		FillSolidRect(dc,rect, GetEmptyWndsAreaBackColor(ctrl) );
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_bars_blue::GetBorderColor(TabCtrl const * /*ctrl*/)
{	return RGB(93,107,153);
}
//
COLORREF TabCtrlStyle_VS2019_bars_blue::GetCtrlAreaBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(93,107,153);
}
// 
COLORREF TabCtrlStyle_VS2019_bars_blue::GetTabSelectedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(247,249,254);
}
// 
COLORREF TabCtrlStyle_VS2019_bars_blue::GetTabTextColor(TabCtrl const *ctrl, TabCtrl::HTAB tab)
{	if( ctrl->IsTabDisabled(tab) )
		return RGB(197,203,224);
	if(ctrl->GetSelectedTab()==tab)
		return (!ctrl->IsWatchCtrlActivity() || ctrl->IsActive() ? RGB(30,30,30) : RGB(80,80,80));
	return (ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab) ?   // highlighted tab.
		RGB(30,30,30) : RGB(217,225,250));
}
// 
COLORREF TabCtrlStyle_VS2019_bars_blue::GetButtonCloseColor(TabCtrl const *ctrl, bool hover, bool pushed)
{	return (ctrl->GetSelectedTab() ? GetButtonMenuColor(ctrl,hover,pushed) : RGB(197,203,224));
}
COLORREF TabCtrlStyle_VS2019_bars_blue::GetButtonMenuColor(TabCtrl const * /*ctrl*/, bool hover, bool pushed)
{	return (!hover && !pushed ? RGB(217,224,248) : RGB(255,255,255));
}
/////////////////////////////////////////////////////////////////////////////
//
void TabCtrlStyle_VS2019_bars_blue::DrawTabBack(TabCtrl const *ctrl, CDC *dc, TabCtrl::HTAB tab, CRect const *rect, CRgn * /*rgn*/)
{	CRect rc(rect);
	const int number = ctrl->GetNumberTabs();
	if(number>1)
		if(tab==ctrl->GetTabHandleByIndex(0))
			--rc.right;
		else if(tab==ctrl->GetTabHandleByIndex(number-1))
			++rc.left;
		else
			rc.DeflateRect(1,0);
		// 
	if(ctrl->GetSelectedTab()==tab)
		FillSolidRect(dc,&rc, GetTabSelectedBackColor(ctrl) );
	else
	{	rc.DeflateRect(0,1);
		if(ctrl->GetTabUnderCursor()==tab && ctrl->GetPushedTab()==nullptr && !ctrl->IsTabDisabled(tab))   // highlighted tab.
			FillSolidRect(dc,&rc, GetTabHighlightedBackColor(ctrl) );
		else
			FillSolidRect(dc,&rc, GetTabNormalBackColor(ctrl) );
	}
}
/////////////////////////////////////////////////////////////////////////////
// 
void TabCtrlStyle_VS2019_bars_blue::DrawButtonFrame(TabCtrl const * /*ctrl*/, CDC * /*dc*/, CRect const * /*rect*/, bool /*hover*/, bool /*pushed*/)
{
}
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_bars_blue::GetTabHighlightedBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(187,198,241);
}
/////////////////////////////////////////////////////////////////////////////
//
COLORREF TabCtrlStyle_VS2019_bars_blue::GetTabNormalBackColor(TabCtrl const * /*ctrl*/)
{	return RGB(64,86,141);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



























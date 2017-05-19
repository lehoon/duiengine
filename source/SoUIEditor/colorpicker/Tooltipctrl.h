#pragma once

#include <SimpleWnd.h>

namespace DuiEngine{

typedef struct tagAFX_OLDTOOLINFO1 {
	UINT cbSize;
	UINT uFlags;
	HWND hwnd;
	UINT uId;
	RECT rect;
	HINSTANCE hinst;
	LPTSTR lpszText;
} AFX_OLDTOOLINFO1;

class CSimpleToolTip : public CSimpleWnd
{
public:
	BOOL Create(HWND hParent,DWORD dwExStyle,HINSTANCE hInst=NULL)
	{
		HWND hWnd=::CreateWindowEx(dwExStyle,TOOLTIPS_CLASS,NULL,WS_POPUP,0,0,0,0,hParent,0,hInst,0);
		if(!hWnd) return FALSE;
		return SubclassWindow(hWnd);
	}

	BOOL AddTool(HWND hWnd, LPCTSTR lpszText, LPCRECT lpRectTool,
		UINT_PTR nIDTool)
	{
		DUIASSERT(::IsWindow(m_hWnd));
		DUIASSERT(::IsWindow(hWnd));
		DUIASSERT(lpszText != NULL);
		// the toolrect and toolid must both be zero or both valid
		DUIASSERT((lpRectTool != NULL && nIDTool != 0) ||
			(lpRectTool == NULL) && (nIDTool == 0));

		TOOLINFO ti;
		FillInToolInfo(ti, hWnd, nIDTool);
		if (lpRectTool != NULL)
		{
			ti.rect=*lpRectTool;
		}

		ti.lpszText = (LPTSTR)lpszText;
		return (BOOL) ::SendMessage(m_hWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
	}

	void RelayEvent(LPMSG lpMsg)
	{ 
		DUIASSERT(::IsWindow(m_hWnd)); 
		::SendMessage(m_hWnd, TTM_RELAYEVENT, 0, (LPARAM)lpMsg); 
	}

protected:
	void FillInToolInfo(TOOLINFO& ti, HWND hwnd, UINT_PTR nIDTool) const
	{
		memset(&ti, 0, sizeof(TOOLINFO));
		ti.cbSize = sizeof(AFX_OLDTOOLINFO1);
		if (nIDTool == 0)
		{
			ti.hwnd = ::GetParent(hwnd);
			ti.uFlags = TTF_IDISHWND;
			ti.uId = (UINT_PTR)hwnd;
		}
		else
		{
			ti.hwnd = hwnd;
			ti.uFlags = 0;
			ti.uId = nIDTool;
		}
	}

};


}//end of namespace

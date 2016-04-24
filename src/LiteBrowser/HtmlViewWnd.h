#pragma once
#include "WebPage.h"
#include "WebHistory.h"

#define HTMLVIEWWND_CLASS L"HTMLVIEW_WINDOW"
#define WM_IMAGE_LOADED		(WM_USER + 1000)
#define WM_PAGE_LOADED		(WM_USER + 1001)

using namespace litehtml;
class CBrowserWnd;
class CHTMLViewWnd
{
	HWND _hWnd;
	HINSTANCE _hInst;
	int _top;
	int _left;
	int _max_top;
	int _max_left;
	context *_context;
	WebHistory _history;
	WebPage *_page;
	WebPage *_page_next;
	CRITICAL_SECTION _sync;
	Dib _dib;
	CBrowserWnd *_parent;
public:
	CHTMLViewWnd(HINSTANCE	hInst, context *ctx, CBrowserWnd *parent);
	virtual ~CHTMLViewWnd();

	void Create(int x, int y, int width, int height, HWND parent);
	void Open(LPCWSTR url, bool reload = FALSE);
	HWND Wnd() { return _hWnd; }
	void Refresh();
	void Back();
	void Forward();

	context *GetHtmlContext();
	void SetCaption();
	void Lock();
	void Unlock();
	bool IsValidPage(bool withLock = true);
	WebPage *GetPage(bool withLock = true);

	void Render(BOOL calcTime = FALSE, BOOL doRedraw = TRUE, int calcRepeat = 1);
	void GetClientRect(position &client) const;
	void ShowHash(std::wstring &hash);
	void UpdateHistory();

protected:
	virtual void OnCreate();
	virtual void OnPaint(Dib *dib, LPRECT rcDraw);
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();
	virtual void OnVScroll(int pos, int flags);
	virtual void OnHScroll(int pos, int flags);
	virtual void OnMouseWheel(int delta);
	virtual void OnKeyDown(UINT vKey);
	virtual void OnMouseMove(int x, int y);
	virtual void OnLButtonDown(int x, int y);
	virtual void OnLButtonUp(int x, int y);
	virtual void OnMouseLeave();
	virtual void OnPageReady();
	
	void Redraw(LPRECT rcDraw, BOOL update);
	void UpdateScroll();
	void UpdateCursor();
	void CreateDib(int width, int height);
	void ScrollTo(int newLeft, int newTop);

private:
	static LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

inline context *CHTMLViewWnd::GetHtmlContext()
{
	return _context;
}

inline void CHTMLViewWnd::Lock()
{
	EnterCriticalSection(&_sync);
}

inline void CHTMLViewWnd::Unlock()
{
	LeaveCriticalSection(&_sync);
}

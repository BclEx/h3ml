#pragma once

#define BROWSERWND_CLASS L"BROWSER_WINDOW"

class CHTMLViewWnd;
class CToolbarWnd;
class CBrowserWnd
{
	HWND				_hWnd;
	HINSTANCE			_hInst;
	CHTMLViewWnd*		_view;
#ifndef NO_TOOLBAR
	CToolbarWnd*		_toolbar;
#endif
	litehtml::context	_browserContext;
public:
	CBrowserWnd(HINSTANCE hInst);
	virtual ~CBrowserWnd();

	void Create();
	void Open(LPCWSTR path);

	void Back();
	void Forward();
	void Reload();
	void CalcTime(int calcRepeat = 1);
	void OnPageLoaded(LPCWSTR url);

protected:
	virtual void OnCreate();
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

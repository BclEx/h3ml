#pragma once

#define BROWSERWND_CLASS L"BROWSER_WINDOW"

using namespace litehtml;
class HtmlViewWnd;
class ToolbarWnd;
class BrowserWnd
{
	HWND _hWnd;
	HINSTANCE _hInst;
	HtmlViewWnd *_view;
#ifndef NO_TOOLBAR
	ToolbarWnd *_toolbar;
#endif
	context _browserContext;
public:
	BrowserWnd(HINSTANCE hInst);
	virtual ~BrowserWnd();

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

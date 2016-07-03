#include "Globals.h"
#include "BrowserWnd.h"
#include "HtmlViewWnd.h"
#include "ToolbarWnd.h"

BrowserWnd::BrowserWnd(HINSTANCE hInst)
{
	_hInst = hInst;
	_hWnd = NULL;
	_view = new HtmlViewWnd(hInst, &_browserContext, this);
#ifndef NO_TOOLBAR
	_toolbar = new ToolbarWnd(hInst, this);
#endif

	WNDCLASS wc;
	if (!GetClassInfo(_hInst, BROWSERWND_CLASS, &wc)) {
		ZeroMemory(&wc, sizeof(wc));
		wc.style = CS_DBLCLKS /*| CS_HREDRAW | CS_VREDRAW*/;
		wc.lpfnWndProc = (WNDPROC)BrowserWnd::WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _hInst;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = BROWSERWND_CLASS;
		RegisterClass(&wc);
	}

#ifndef LITEHTML_UTF8
	LPWSTR css = NULL;
	HRSRC hResource = ::FindResource(_hInst, L"master.css", L"CSS");
	if (hResource) {
		DWORD imageSize = ::SizeofResource(_hInst, hResource);
		if (imageSize) {
			LPCSTR resourceData = (LPCSTR)::LockResource(::LoadResource(_hInst, hResource));
			if (resourceData) {
				css = new WCHAR[imageSize * 3];
				int ret = MultiByteToWideChar(CP_UTF8, 0, resourceData, imageSize, css, imageSize * 3);
				css[ret] = 0;
			}
		}
	}
#else
	LPSTR css = NULL;
	HRSRC hResource = ::FindResource(_hInst, L"master.css", L"CSS");
	if (hResource) {
		DWORD imageSize = ::SizeofResource(_hInst, hResource);
		if (imageSize) {
			LPCSTR resourceData = (LPCSTR)::LockResource(::LoadResource(_hInst, hResource));
			if (resourceData) {
				css = new CHAR[imageSize + 1];
				lstrcpynA(css, resourceData, imageSize);
				css[imageSize] = 0;
			}
		}
	}
#endif
	if (css) {
		_browserContext.load_master_stylesheet(css);
		delete css;
	}
}

BrowserWnd::~BrowserWnd()
{
	if (_view) delete _view;
#ifndef NO_TOOLBAR
	if (_toolbar) delete _toolbar;
#endif
}

LRESULT CALLBACK BrowserWnd::WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	BrowserWnd *this_ = NULL;
	if (IsWindow(hWnd)) {
		this_ = (BrowserWnd *)GetProp(hWnd, TEXT("browser_this"));
		if (this_ && this_->_hWnd != hWnd)
			this_ = NULL;
	}
	if (this_ || uMessage == WM_CREATE) {
		switch (uMessage) {
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE: {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			this_ = (BrowserWnd *)(lpcs->lpCreateParams);
			SetProp(hWnd, TEXT("browser_this"), (HANDLE)this_);
			this_->_hWnd = hWnd;
			this_->OnCreate();
			break; }
		case WM_SIZE:
			this_->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemoveProp(hWnd, TEXT("browser_this"));
			this_->OnDestroy();
			delete this_;
			return 0;
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_INACTIVE)
				SetFocus(this_->_view->Wnd());
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void BrowserWnd::OnCreate()
{
	RECT rcClient;
	GetClientRect(_hWnd, &rcClient);
#ifndef NO_TOOLBAR
	_toolbar->Create(rcClient.left, rcClient.top, rcClient.right - rcClient.left, _hWnd);
	_view->Create(rcClient.left, rcClient.top + _toolbar->Height(), rcClient.right - rcClient.left, rcClient.bottom - rcClient.top - _toolbar->Height(), _hWnd);
#else
	_view->Create(rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, _hWnd);
#endif
	SetFocus(_view->Wnd());
}

void BrowserWnd::OnSize(int width, int height)
{
	RECT rcClient;
	GetClientRect(_hWnd, &rcClient);
#ifndef NO_TOOLBAR
	int toolbarHeight = _toolbar->SetWidth(rcClient.right - rcClient.left);
#else
	int toolbarHeight = 0;
#endif
	SetWindowPos(_view->Wnd(), NULL, rcClient.left, rcClient.top + toolbarHeight, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top - toolbarHeight, SWP_NOZORDER);
	UpdateWindow(_view->Wnd());
#ifndef NO_TOOLBAR
	SetWindowPos(_toolbar->Wnd(), NULL, rcClient.left, rcClient.top, rcClient.right - rcClient.left, toolbarHeight, SWP_NOZORDER);
	UpdateWindow(_toolbar->Wnd());
#endif
}

void BrowserWnd::OnDestroy()
{
}

void BrowserWnd::Create()
{
	_hWnd = CreateWindow(BROWSERWND_CLASS, L"Light HTML", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, _hInst, (LPVOID)this);
	ShowWindow(_hWnd, SW_SHOW);
}

void BrowserWnd::Open(LPCWSTR path)
{
	if (_view)
		_view->Open(path, true);
}

void BrowserWnd::Back()
{
	if (_view)
		_view->Back();
}

void BrowserWnd::Forward()
{
	if (_view)
		_view->Forward();
}

void BrowserWnd::Reload()
{
	if (_view)
		_view->Refresh();
}

void BrowserWnd::CalcTime(int calcRepeat)
{
	if (_view)
		_view->Render(TRUE, TRUE, calcRepeat);
}

void BrowserWnd::OnPageLoaded(LPCWSTR url)
{
	if (_view)
		SetFocus(_view->Wnd());
#ifndef NO_TOOLBAR
	_toolbar->OnPageLoaded(url);
#endif
}

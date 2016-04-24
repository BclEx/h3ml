#include "Globals.h"
#include "BrowserWnd.h"
#include "HtmlViewWnd.h"
#include "ToolbarWnd.h"

CBrowserWnd::CBrowserWnd(HINSTANCE hInst)
{
	_hInst = hInst;
	_hWnd = NULL;
	_view = new CHTMLViewWnd(hInst, &_browserContext, this);
#ifndef NO_TOOLBAR
	_toolbar = new CToolbarWnd(hInst, this);
#endif

	WNDCLASS wc;
	if (!GetClassInfo(_hInst, BROWSERWND_CLASS, &wc)) {
		ZeroMemory(&wc, sizeof(wc));
		wc.style = CS_DBLCLKS /*| CS_HREDRAW | CS_VREDRAW*/;
		wc.lpfnWndProc = (WNDPROC)CBrowserWnd::WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _hInst;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
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

CBrowserWnd::~CBrowserWnd()
{
	if (_view) delete _view;
#ifndef NO_TOOLBAR
	if (_toolbar) delete _toolbar;
#endif
}

LRESULT CALLBACK CBrowserWnd::WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	CBrowserWnd *this_ = NULL;
	if (IsWindow(hWnd)) {
		this_ = (CBrowserWnd *)GetProp(hWnd, TEXT("browser_this"));
		if (this_ && this_->_hWnd != hWnd)
			this_ = NULL;
	}
	if (this_ || uMessage == WM_CREATE) {
		switch (uMessage) {
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE: {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			this_ = (CBrowserWnd *)(lpcs->lpCreateParams);
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

void CBrowserWnd::OnCreate()
{
	RECT rcClient;
	GetClientRect(_hWnd, &rcClient);
#ifndef NO_TOOLBAR
	_toolbar->Create(rcClient.left, rcClient.top, rcClient.right - rcClient.left, m_hWnd);
	_view->Create(rcClient.left, rcClient.top + m_toolbar->height(), rcClient.right - rcClient.left, rcClient.bottom - rcClient.top - _toolbar->height(), _hWnd);
#else
	_view->Create(rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, _hWnd);
#endif
	SetFocus(_view->Wnd());
}

void CBrowserWnd::OnSize(int width, int height)
{
	RECT rcClient;
	GetClientRect(_hWnd, &rcClient);
#ifndef NO_TOOLBAR
	int toolbarHeight = _toolbar->set_width(rcClient.right - rcClient.left);
#else
	int toolbarHeight = 0;
#endif
	SetWindowPos(_view->Wnd(), NULL, rcClient.left, rcClient.top + toolbarHeight, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top - toolbarHeight, SWP_NOZORDER);
	UpdateWindow(_view->Wnd());
#ifndef NO_TOOLBAR
	SetWindowPos(_toolbar->Wnd(), NULL, rcClient.left, rcClient.top, rcClient.right - rcClient.left, toolbar_height, SWP_NOZORDER);
	UpdateWindow(_toolbar->Wnd());
#endif
}

void CBrowserWnd::OnDestroy()
{
}

void CBrowserWnd::Create()
{
	_hWnd = CreateWindow(BROWSERWND_CLASS, L"Light HTML", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, _hInst, (LPVOID)this);
	ShowWindow(_hWnd, SW_SHOW);
}

void CBrowserWnd::Open(LPCWSTR path)
{
	if (_view)
		_view->Open(path, true);
}

void CBrowserWnd::Back()
{
	if (_view)
		_view->Back();
}

void CBrowserWnd::Forward()
{
	if (_view)
		_view->Forward();
}

void CBrowserWnd::Reload()
{
	if (_view)
		_view->Refresh();
}

void CBrowserWnd::CalcTime(int calcRepeat)
{
	if (_view)
		_view->Render(TRUE, TRUE, calcRepeat);
}

void CBrowserWnd::OnPageLoaded(LPCWSTR url)
{
	if (_view)
		SetFocus(_view->Wnd());
#ifndef NO_TOOLBAR
	_toolbar->OnPageLoaded(url);
#endif
}

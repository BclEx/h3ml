#include "globals.h"
#include "HtmlViewWnd.h"
#include <WindowsX.h>
#include <algorithm>
#include <strsafe.h>
#include "BrowserWnd.h"

HtmlViewWnd::HtmlViewWnd(HINSTANCE hInst, context *ctx, BrowserWnd *parent)
{
	_parent = parent;
	_hInst = hInst;
	_hWnd = NULL;
	_top = 0;
	_left = 0;
	_max_top = 0;
	_max_left = 0;
	_context = ctx;
	_page = NULL;
	_page_next = NULL;
	InitializeCriticalSection(&_sync);
	WNDCLASS wc;
	if (!GetClassInfo(_hInst, HTMLVIEWWND_CLASS, &wc)) {
		ZeroMemory(&wc, sizeof(wc));
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC)HtmlViewWnd::WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _hInst;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = HTMLVIEWWND_CLASS;
		RegisterClass(&wc);
	}
}

HtmlViewWnd::~HtmlViewWnd()
{
	DeleteCriticalSection(&_sync);
}

LRESULT CALLBACK HtmlViewWnd::WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	HtmlViewWnd *this_ = NULL;
	if (IsWindow(hWnd)) {
		this_ = (HtmlViewWnd *)GetProp(hWnd, TEXT("htmlview_this"));
		if (this_ && this_->_hWnd != hWnd)
			this_ = NULL;
	}

	if (this_ || uMessage == WM_CREATE) {
		switch (uMessage) {
		case WM_PAGE_LOADED:
			this_->OnPageReady();
			return 0;
		case WM_IMAGE_LOADED:
			if (wParam)
				this_->Redraw(NULL, FALSE);
			else
				this_->Render();
			break;
		case WM_SETCURSOR:
			this_->UpdateCursor();
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE: {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			this_ = (HtmlViewWnd *)(lpcs->lpCreateParams);
			SetProp(hWnd, TEXT("htmlview_this"), (HANDLE)this_);
			this_->_hWnd = hWnd;
			this_->OnCreate();
			break; }
		case WM_PAINT: {
			RECT rcClient;
			::GetClientRect(hWnd, &rcClient);
			this_->CreateDib(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			this_->OnPaint(&this_->_dib, &ps.rcPaint);
			BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right - ps.rcPaint.left,
				ps.rcPaint.bottom - ps.rcPaint.top, this_->_dib, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
			EndPaint(hWnd, &ps);
			return 0; }
		case WM_SIZE:
			this_->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemoveProp(hWnd, TEXT("htmlview_this"));
			this_->OnDestroy();
			delete this_;
			return 0;
		case WM_VSCROLL:
			this_->OnVScroll(HIWORD(wParam), LOWORD(wParam));
			return 0;
		case WM_HSCROLL:
			this_->OnHScroll(HIWORD(wParam), LOWORD(wParam));
			return 0;
		case WM_MOUSEWHEEL:
			this_->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			return 0;
		case WM_KEYDOWN:
			this_->OnKeyDown((UINT)wParam);
			return 0;
		case WM_MOUSEMOVE: {
			TRACKMOUSEEVENT tme;
			ZeroMemory(&tme, sizeof(TRACKMOUSEEVENT));
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_QUERY;
			tme.hwndTrack = hWnd;
			TrackMouseEvent(&tme);
			if (!(tme.dwFlags & TME_LEAVE)) {
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hWnd;
				TrackMouseEvent(&tme);
			}
			this_->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0; }
		case WM_MOUSELEAVE:
			this_->OnMouseLeave();
			return 0;
		case WM_LBUTTONDOWN:
			this_->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
			this_->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void HtmlViewWnd::OnCreate()
{
}

void HtmlViewWnd::OnPaint(Dib *dib, LPRECT rcDraw)
{
	cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)dib->Bits(), CAIRO_FORMAT_ARGB32, dib->Width(), dib->Height(), dib->Width() * 4);
	cairo_t *cr = cairo_create(surface);

	cairo_rectangle(cr, rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
	cairo_clip(cr);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	Lock();
	WebPage *page = GetPage(false);
	if (page) {
		position clip(rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
		page->_doc->draw((uint_ptr)cr, -_left, -_top, &clip);
		page->Release();
	}
	Unlock();

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

void HtmlViewWnd::OnSize(int width, int height)
{
	Lock();
	WebPage *page = GetPage(false);
	Unlock();

	if (page) {
		page->_doc->media_changed();
		page->Release();
	}
	Render();
}

void HtmlViewWnd::OnDestroy()
{
}

void HtmlViewWnd::Create(int x, int y, int width, int height, HWND parent)
{
	_hWnd = CreateWindow(HTMLVIEWWND_CLASS, L"htmlview", WS_CHILD | WS_VISIBLE, x, y, width, height, parent, NULL, _hInst, (LPVOID)this);
}

void HtmlViewWnd::Open(LPCWSTR url, bool reload)
{
	std::wstring hash;
	std::wstring s_url = url;
	std::wstring::size_type hash_pos = s_url.find_first_of(L'#');
	if (hash_pos != std::wstring::npos) {
		hash = s_url.substr(hash_pos + 1);
		s_url.erase(hash_pos);
	}
	bool open_hash_only = false;

	Lock();
	if (_page) {
		if (_page->_url == s_url && !reload)
			open_hash_only = true;
		else
			_page->_http.Stop();
	}
	if (!open_hash_only) {
		if (_page_next) {
			_page_next->_http.Stop();
			_page_next->Release();
		}
		_page_next = new WebPage(this);
		_page_next->_hash = hash;
		_page_next->Load(s_url.c_str());
	}
	Unlock();

	if (open_hash_only) {
		ShowHash(hash);
		UpdateScroll();
		Redraw(NULL, FALSE);
		UpdateHistory();
	}
}

void HtmlViewWnd::Render(BOOL calcTime, BOOL doRedraw, int calcRepeat)
{
	if (!_hWnd)
		return;
	WebPage *page = GetPage();
	if (page) {
		RECT rcClient;
		::GetClientRect(_hWnd, &rcClient);
		int width = rcClient.right - rcClient.left;
		int height = rcClient.bottom - rcClient.top;

		if (calcTime) {
			if (calcRepeat <= 0) calcRepeat = 1;
			DWORD tic1 = GetTickCount();
			for (int i = 0; i < calcRepeat; i++)
				page->_doc->render(width);
			DWORD tic2 = GetTickCount();
			WCHAR msg[255];
			StringCchPrintf(msg, 255, L"Render time: %d msec", tic2 - tic1);
			MessageBox(_hWnd, msg, L"LiteBrowser", MB_ICONINFORMATION | MB_OK);
		}
		else
			page->_doc->render(width);

		_max_top = page->_doc->height() - height;
		if (_max_top < 0) _max_top = 0;
		_max_left = page->_doc->width() - width;
		if (_max_left < 0) _max_left = 0;
		if (doRedraw) {
			UpdateScroll();
			Redraw(NULL, FALSE);
		}

		page->Release();
	}
}

void HtmlViewWnd::Redraw(LPRECT rcDraw, BOOL update)
{
	if (_hWnd) {
		InvalidateRect(_hWnd, rcDraw, TRUE);
		if (update)
			UpdateWindow(_hWnd);
	}
}

void HtmlViewWnd::UpdateScroll()
{
	if (!IsValidPage()) {
		ShowScrollBar(_hWnd, SB_BOTH, FALSE);
		return;
	}

	if (_max_top > 0) {
		ShowScrollBar(_hWnd, SB_VERT, TRUE);
		RECT rcClient;
		::GetClientRect(_hWnd, &rcClient);
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = _max_top + (rcClient.bottom - rcClient.top);
		si.nPos = _top;
		si.nPage = rcClient.bottom - rcClient.top;
		SetScrollInfo(_hWnd, SB_VERT, &si, TRUE);
	}
	else
		ShowScrollBar(_hWnd, SB_VERT, FALSE);

	if (_max_left > 0) {
		ShowScrollBar(_hWnd, SB_HORZ, TRUE);
		RECT rcClient;
		::GetClientRect(_hWnd, &rcClient);
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = _max_left + (rcClient.right - rcClient.left);
		si.nPos = _left;
		si.nPage = rcClient.right - rcClient.left;
		SetScrollInfo(_hWnd, SB_HORZ, &si, TRUE);
	}
	else
		ShowScrollBar(_hWnd, SB_HORZ, FALSE);
}

void HtmlViewWnd::OnVScroll(int pos, int flags)
{
	RECT rcClient;
	::GetClientRect(_hWnd, &rcClient);
	int lineHeight = 16;
	int pageHeight = rcClient.bottom - rcClient.top - lineHeight;
	int newTop = _top;
	switch (flags) {
	case SB_LINEDOWN:
		newTop = _top + lineHeight;
		if (newTop > _max_top)
			newTop = _max_top;
		break;
	case SB_PAGEDOWN:
		newTop = _top + pageHeight;
		if (newTop > _max_top)
			newTop = _max_top;
		break;
	case SB_LINEUP:
		newTop = _top - lineHeight;
		if (newTop < 0)
			newTop = 0;
		break;
	case SB_PAGEUP:
		newTop = _top - pageHeight;
		if (newTop < 0)
			newTop = 0;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newTop = pos;
		if (newTop < 0)
			newTop = 0;
		if (newTop > _max_top)
			newTop = _max_top;
		break;
	}
	ScrollTo(_left, newTop);
}

void HtmlViewWnd::OnHScroll(int pos, int flags)
{
	RECT rcClient;
	::GetClientRect(_hWnd, &rcClient);

	int lineWidth = 16;
	int pageWidth = rcClient.right - rcClient.left - lineWidth;
	int newLeft = _left;
	switch (flags) {
	case SB_LINERIGHT:
		newLeft = _left + lineWidth;
		if (newLeft > _max_left)
			newLeft = _max_left;
		break;
	case SB_PAGERIGHT:
		newLeft = _left + pageWidth;
		if (newLeft > _max_left)
			newLeft = _max_left;
		break;
	case SB_LINELEFT:
		newLeft = _left - lineWidth;
		if (newLeft < 0)
			newLeft = 0;
		break;
	case SB_PAGELEFT:
		newLeft = _left - pageWidth;
		if (newLeft < 0)
			newLeft = 0;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newLeft = pos;
		if (newLeft < 0)
			newLeft = 0;
		if (newLeft > _max_left)
			newLeft = _max_left;
		break;
	}
	ScrollTo(newLeft, _top);
}

void HtmlViewWnd::OnMouseWheel(int delta)
{
	int lineHeight = 16;
	int newTop = _top - delta / WHEEL_DELTA * lineHeight * 3;
	if (newTop < 0)
		newTop = 0;
	if (newTop > _max_top)
		newTop = _max_top;
	if (newTop != _top)
		ScrollTo(_left, newTop);
}

void HtmlViewWnd::OnKeyDown(UINT vKey)
{
	switch (vKey) {
	case VK_F5: Refresh(); break;
	case VK_NEXT: OnVScroll(0, SB_PAGEDOWN); break;
	case VK_PRIOR: OnVScroll(0, SB_PAGEUP); break;
	case VK_DOWN: OnVScroll(0, SB_LINEDOWN); break;
	case VK_UP: OnVScroll(0, SB_LINEUP); break;
	case VK_HOME: ScrollTo(_left, 0); break;
	case VK_END: ScrollTo(_left, _max_top); break;
	case VK_LEFT: OnHScroll(0, SB_LINELEFT); break;
	case VK_RIGHT: OnHScroll(0, SB_LINERIGHT); break;
	}
}

void HtmlViewWnd::Refresh()
{
	WebPage *page = GetPage();
	if (page) {
		Open(page->_url.c_str(), true);
		page->Release();
	}
}

void HtmlViewWnd::SetCaption()
{
	WebPage *page = GetPage();
	if (!page)
		SetWindowText(GetParent(_hWnd), L"LiteBrowser");
	else {
		SetWindowText(GetParent(_hWnd), page->_caption.c_str());
		page->Release();
	}
}

void HtmlViewWnd::OnMouseMove(int x, int y)
{
	WebPage *page = GetPage();
	if (page) {
		position::vector redrawBoxes;
		if (page->_doc->on_mouse_over(x + _left, y + _top, x, y, redrawBoxes)) {
			for (position::vector::iterator box = redrawBoxes.begin(); box != redrawBoxes.end(); box++) {
				box->x -= _left;
				box->y -= _top;
				RECT rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				Redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(_hWnd);
			UpdateCursor();
		}
		page->Release();
	}
}

void HtmlViewWnd::OnMouseLeave()
{
	WebPage *page = GetPage();
	if (page) {
		position::vector redrawBoxes;
		if (page->_doc->on_mouse_leave(redrawBoxes)) {
			for (position::vector::iterator box = redrawBoxes.begin(); box != redrawBoxes.end(); box++) {
				box->x -= _left;
				box->y -= _top;
				RECT rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				Redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(_hWnd);
		}
		page->Release();
	}
}

void HtmlViewWnd::OnLButtonDown(int x, int y)
{
	WebPage *page = GetPage();
	if (page) {
		position::vector redrawBoxes;
		if (page->_doc->on_lbutton_down(x + _left, y + _top, x, y, redrawBoxes)) {
			for (position::vector::iterator box = redrawBoxes.begin(); box != redrawBoxes.end(); box++) {
				box->x -= _left;
				box->y -= _top;
				RECT rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				Redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(_hWnd);
		}
		page->Release();
	}
}

void HtmlViewWnd::OnLButtonUp(int x, int y)
{
	WebPage *page = GetPage();
	if (page) {
		position::vector redrawBoxes;
		if (page->_doc->on_lbutton_up(x + _left, y + _top, x, y, redrawBoxes)) {
			for (position::vector::iterator box = redrawBoxes.begin(); box != redrawBoxes.end(); box++) {
				box->x -= _left;
				box->y -= _top;
				RECT rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				Redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(_hWnd);
		}
		page->Release();
	}
}

void HtmlViewWnd::Back()
{
	std::wstring url;
	if (_history.Back(url))
		Open(url.c_str(), false);
}

void HtmlViewWnd::Forward()
{
	std::wstring url;
	if (_history.Forward(url))
		Open(url.c_str(), false);
}

void HtmlViewWnd::UpdateCursor()
{
	LPCWSTR defArrow = (_page_next ? IDC_APPSTARTING : IDC_ARROW);
	WebPage *page = GetPage();
	if (!page)
		SetCursor(LoadCursor(NULL, defArrow));
	else {
		if (page->_cursor == L"pointer")
			SetCursor(LoadCursor(NULL, IDC_HAND));
		else
			SetCursor(LoadCursor(NULL, defArrow));
		page->Release();
	}
}

void HtmlViewWnd::GetClientRect(position &client) const
{
	RECT rcClient;
	::GetClientRect(_hWnd, &rcClient);
	client.x = rcClient.left;
	client.y = rcClient.top;
	client.width = rcClient.right - rcClient.left;
	client.height = rcClient.bottom - rcClient.top;
}

bool HtmlViewWnd::IsValidPage(bool withLock)
{
	bool ret_val = true;
	if (withLock)
		Lock();
	if (!_page || _page && !_page->_doc)
		ret_val = false;
	if (withLock)
		Unlock();
	return ret_val;
}

WebPage *HtmlViewWnd::GetPage(bool withLock)
{
	WebPage *ret_val = NULL;
	if (withLock)
		Lock();
	if (IsValidPage(false)) {
		ret_val = _page;
		ret_val->AddRef();
	}
	if (withLock)
		Unlock();
	return ret_val;
}

void HtmlViewWnd::OnPageReady()
{
	std::wstring url;
	Lock();
	WebPage *page = _page_next;
	Unlock();

	std::wstring hash;
	bool isOk = false;
	Lock();
	if (_page_next) {
		if (_page)
			_page->Release();
		_page = _page_next;
		_page_next = NULL;
		isOk = true;
		hash = _page->_hash;
		url = _page->_url;
	}
	Unlock();

	if (isOk) {
		Render(FALSE, FALSE);
		_top = 0;
		_left = 0;
		ShowHash(hash);
		UpdateScroll();
		Redraw(NULL, FALSE);
		SetCaption();
		UpdateHistory();
		_parent->OnPageLoaded(url.c_str());
	}
}

void HtmlViewWnd::ShowHash(std::wstring &hash)
{
	WebPage *page = GetPage();
	if (page) {
		if (!hash.empty()) {
			tchar_t selector[255];
#ifndef LITEHTML_UTF8
			StringCchPrintf(selector, 255, L"#%s", hash.c_str());
#else
			LPSTR hashA = cairo_font::WcharToUtf8(hash.c_str());
			StringCchPrintfA(selector, 255, "#%s", hashA);
#endif
			element::ptr el = page->_doc->root()->select_one(selector);
			if (!el) {
#ifndef LITEHTML_UTF8
				StringCchPrintf(selector, 255, L"[name=%s]", hash.c_str());
#else
				StringCchPrintfA(selector, 255, "[name=%s]", hashA);
#endif
				el = page->_doc->root()->select_one(selector);
			}
			if (el) {
				position pos = el->get_placement();
				_top = pos.y;
			}
#ifdef LITEHTML_UTF8
			delete hashA;
#endif
		}
		else
			_top = 0;
		if (page->_hash != hash)
			page->_hash = hash;
		page->Release();
	}
}

void HtmlViewWnd::UpdateHistory()
{
	WebPage *page = GetPage();
	if (page) {
		std::wstring url;
		page->GetUrl(url);
		_history.UrlOpened(url);
		page->Release();
	}
}

void HtmlViewWnd::CreateDib(int width, int height)
{
	if (_dib.Width() < width || _dib.Height() < height) {
		_dib.Destroy();
		_dib.Create(width, height, true);
	}
}

void HtmlViewWnd::ScrollTo(int newLeft, int newTop)
{
	position client;
	GetClientRect(client);

	bool needRedraw = false;
	if (newTop != _top) {
		if (std::abs(newTop - _top) < client.height - client.height / 4) {
			RECT rcRedraw;
			if (newTop > _top) {
				int linesCount = newTop - _top;
				int rgbaToScroll = _dib.Width() * linesCount;
				int rgbaTotal = _dib.Width() * client.height;
				memmove(_dib.Bits(), _dib.Bits() + rgbaToScroll, (rgbaTotal - rgbaToScroll) * sizeof(RGBQUAD));
				rcRedraw.left = client.left();
				rcRedraw.right = client.right();
				rcRedraw.top = client.height - linesCount;
				rcRedraw.bottom = client.height;
			}
			else {
				int linesCount = _top - newTop;
				int rgbaToScroll = _dib.Width() * linesCount;
				int rgbaTotal = _dib.Width() * client.height;
				memmove(_dib.Bits() + rgbaToScroll, _dib.Bits(), (rgbaTotal - rgbaToScroll) * sizeof(RGBQUAD));
				rcRedraw.left = client.left();
				rcRedraw.right = client.right();
				rcRedraw.top = client.top();
				rcRedraw.bottom = linesCount;
			}

			int oldTop = _top;
			_top = newTop;
			OnPaint(&_dib, &rcRedraw);

			position::vector fixedBoxes;
			Lock();
			WebPage *page = GetPage(false);
			if (page) {
				page->_doc->get_fixed_boxes(fixedBoxes);
				page->Release();
			}
			Unlock();

			if (!fixedBoxes.empty()) {
				RECT rcFixed;
				RECT rcClient;
				rcClient.left = client.left();
				rcClient.right = client.right();
				rcClient.top = client.top();
				rcClient.bottom = client.bottom();
				for (position::vector::iterator iter = fixedBoxes.begin(); iter != fixedBoxes.end(); iter++) {
					rcRedraw.left = iter->left();
					rcRedraw.right = iter->right();
					rcRedraw.top = iter->top();
					rcRedraw.bottom = iter->bottom();
					if (IntersectRect(&rcFixed, &rcRedraw, &rcClient))
						OnPaint(&_dib, &rcFixed);
					rcRedraw.left = iter->left();
					rcRedraw.right = iter->right();
					rcRedraw.top = iter->top() + (oldTop - _top);
					rcRedraw.bottom = iter->bottom() + (oldTop - _top);
					if (IntersectRect(&rcFixed, &rcRedraw, &rcClient))
						OnPaint(&_dib, &rcFixed);
				}
			}

			HDC hdc = GetDC(_hWnd);
			BitBlt(hdc, client.left(), client.top(),
				client.width,
				client.height, _dib, 0, 0, SRCCOPY);
			ReleaseDC(_hWnd, hdc);

		}
		else
			needRedraw = true;

		_top = newTop;
		SetScrollPos(_hWnd, SB_VERT, _top, TRUE);
	}
	if (newLeft != _left) {
		_left = newLeft;
		SetScrollPos(_hWnd, SB_HORZ, _left, TRUE);
		needRedraw = true;
	}
	if (needRedraw)
		Redraw(NULL, TRUE);
}

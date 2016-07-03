#include "globals.h"
#include "ToolbarWnd.h"
#include <WindowsX.h>
#include "BrowserWnd.h"
#include "el_omnibox.h"

#ifdef LITEHTML_UTF8
#define str_cmp	strcmp
#else
#define str_cmp	lstrcmp
#endif

ToolbarWnd::ToolbarWnd(HINSTANCE hInst, BrowserWnd *parent)
{
	_inCapture = FALSE;
	_omnibox = nullptr;
	_parent = parent;
	_hInst = hInst;
	_hWnd = NULL;

	WNDCLASS wc;
	if (!GetClassInfo(_hInst, TOOLBARWND_CLASS, &wc)) {
		ZeroMemory(&wc, sizeof(wc));
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC)ToolbarWnd::WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _hInst;
		wc.hIcon = NULL;
		wc.hCursor = NULL/*LoadCursor(NULL, IDC_ARROW)*/;
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = TOOLBARWND_CLASS;

		RegisterClass(&wc);
	}

	_context.load_master_stylesheet(_t("html,div,body { display: block; } head,style { display: none; }"));
}
ToolbarWnd::~ToolbarWnd(void)
{
	if (_omnibox)
		_omnibox = nullptr;
}

LRESULT CALLBACK ToolbarWnd::WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	ToolbarWnd *pThis = NULL;
	if (IsWindow(hWnd)) {
		pThis = (ToolbarWnd *)GetProp(hWnd, TEXT("toolbar_this"));
		if (pThis && pThis->_hWnd != hWnd)
			pThis = NULL;
	}

	if (pThis || uMessage == WM_CREATE) {
		switch (uMessage) {
		case WM_EDIT_CAPTURE:
			if (wParam) {
				SetCapture(hWnd);
				pThis->_inCapture = TRUE;
			}
			else {
				ReleaseCapture();
				pThis->_inCapture = FALSE;
			}
			break;
		case WM_EDIT_ACTIONKEY:
			switch (wParam) {
			case VK_RETURN: {
				std::wstring url = pThis->_omnibox->get_url();
				pThis->_omnibox->select_all();
				pThis->_parent->Open(url.c_str());
				break;
			}
			}
			return 0;
		case WM_OMNIBOX_CLICKED:
			pThis->OnOmniboxClicked();
			break;
		case WM_UPDATE_CONTROL: {
			LPRECT rcDraw = (LPRECT)lParam;
			InvalidateRect(hWnd, rcDraw, FALSE);
			break;
		}
		case WM_SETCURSOR:
			pThis->UpdateCursor();
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE: {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			pThis = (ToolbarWnd *)(lpcs->lpCreateParams);
			SetProp(hWnd, TEXT("toolbar_this"), (HANDLE)pThis);
			pThis->_hWnd = hWnd;
			pThis->OnCreate();
			break;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			Dib dib;
			dib.BeginPaint(hdc, &ps.rcPaint);
			pThis->OnPaint(&dib, &ps.rcPaint);
			dib.EndPaint();

			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_KILLFOCUS:
			if (pThis->_omnibox && pThis->_omnibox->have_focus())
				pThis->_omnibox->KillFocus();
			break;
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemoveProp(hWnd, TEXT("toolbar_this"));
			pThis->OnDestroy();
			delete pThis;
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
			pThis->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		}
		case WM_MOUSELEAVE:
			pThis->OnMouseLeave();
			return 0;
		case WM_LBUTTONDOWN:
			pThis->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
			pThis->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_KEYDOWN:
			if (pThis->_omnibox && pThis->_omnibox->have_focus())
				if (pThis->_omnibox->OnKeyDown(wParam, lParam))
					return 0;
			break;
		case WM_KEYUP:
			if (pThis->_omnibox && pThis->_omnibox->have_focus())
				if (pThis->_omnibox->OnKeyUp(wParam, lParam))
					return 0;
			break;
		case WM_CHAR:
			if (pThis->_omnibox && pThis->_omnibox->have_focus())
				if (pThis->_omnibox->OnChar(wParam, lParam))
					return 0;
			break;
		}
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void ToolbarWnd::RenderToolbar(int width)
{
	if (_doc) {
		_doc->render(width);
		_omnibox->update_position();
	}
}

void ToolbarWnd::UpdateCursor()
{
	LPCWSTR defArrow = IDC_ARROW;
	if (_cursor == _t("pointer")) ::SetCursor(LoadCursor(NULL, IDC_HAND));
	else if (_cursor == _t("text")) ::SetCursor(LoadCursor(NULL, IDC_IBEAM));
	else ::SetCursor(LoadCursor(NULL, defArrow));
}

void ToolbarWnd::OnCreate()
{
}

void ToolbarWnd::OnPaint(Dib *dib, LPRECT rcDraw)
{
	if (_doc) {
		cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)dib->Bits(), CAIRO_FORMAT_ARGB32, dib->Width(), dib->Height(), dib->Width() * 4);
		cairo_t *cr = cairo_create(surface);

		POINT pt;
		GetWindowOrgEx(dib->Hdc(), &pt);
		if (pt.x != 0 || pt.y != 0)
			cairo_translate(cr, -pt.x, -pt.y);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_paint(cr);

		position clip(rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
		_doc->draw((uint_ptr)cr, 0, 0, &clip);

		cairo_destroy(cr);
		cairo_surface_destroy(surface);
	}
}

void ToolbarWnd::OnSize(int width, int height)
{
}

void ToolbarWnd::OnDestroy()
{
}

void ToolbarWnd::Create(int x, int y, int width, HWND parent)
{
#ifndef LITEHTML_UTF8
	LPWSTR html = NULL;

	HRSRC hResource = ::FindResource(_hInst, L"toolbar.html", RT_HTML);
	if (hResource) {
		DWORD imageSize = ::SizeofResource(_hInst, hResource);
		if (imageSize) {
			LPCSTR pResourceData = (LPCSTR)::LockResource(::LoadResource(_hInst, hResource));
			if (pResourceData) {
				html = new WCHAR[imageSize * 3];
				int ret = MultiByteToWideChar(CP_UTF8, 0, pResourceData, imageSize, html, imageSize * 3);
				html[ret] = 0;
			}
		}
	}
#else
	LPSTR html = NULL;

	HRSRC hResource = ::FindResource(_hInst, L"toolbar.html", RT_HTML);
	if (hResource) {
		DWORD imageSize = ::SizeofResource(_hInst, hResource);
		if (imageSize) {
			LPCSTR pResourceData = (LPCSTR)::LockResource(::LoadResource(_hInst, hResource));
			if (pResourceData) {
				html = new CHAR[imageSize + 1];
				lstrcpynA(html, pResourceData, imageSize);
				html[imageSize] = 0;
			}
		}
	}
#endif
	_hWnd = CreateWindow(TOOLBARWND_CLASS, L"toolbar", WS_CHILD | WS_VISIBLE, x, y, width, 1, parent, NULL, _hInst, (LPVOID) this);

	_doc = document::createFromString(html, this, &_context);
	delete html;
	RenderToolbar(width);
	MoveWindow(_hWnd, x, y, width, _doc->height(), TRUE);
}

void ToolbarWnd::MakeUrl(LPCWSTR url, LPCWSTR basepath, std::wstring& out)
{
	out = url;
}

cairo_container::image_ptr ToolbarWnd::GetImage(LPCWSTR url, bool redraw_on_ready)
{
	cairo_container::image_ptr img = cairo_container::image_ptr(new TxDib);
	if (!img->Load(FindResource(_hInst, url, RT_HTML), _hInst))
		img = nullptr;
	return img;
}

void ToolbarWnd::SetCaption(const tchar_t *caption)
{
}

void ToolbarWnd::SetBaseUrl(const tchar_t *base_url)
{
}

void ToolbarWnd::Link(std::shared_ptr<document>& doc, element::ptr el)
{
}

int ToolbarWnd::SetWidth(int width)
{
	if (_doc) {
		RenderToolbar(width);
		return _doc->height();
	}
	return 0;
}

void ToolbarWnd::OnPageLoaded(LPCWSTR url)
{
	if (_omnibox)
		_omnibox->set_url(url);
}

void ToolbarWnd::OnMouseMove(int x, int y)
{
	if (_doc) {
		BOOL process = TRUE;
		if (_omnibox)
			_omnibox->OnMouseMove(x, y);
		if (!_inCapture) {
			position::vector redraw_boxes;
			if (_doc->on_mouse_over(x, y, x, y, redraw_boxes)) {
				for (position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
					RECT rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRect(_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindow(_hWnd);
			}
		}
	}
	UpdateCursor();
}

void ToolbarWnd::OnMouseLeave()
{
	if (_doc) {
		position::vector redraw_boxes;
		if (_doc->on_mouse_leave(redraw_boxes)) {
			for (position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
				RECT rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				InvalidateRect(_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindow(_hWnd);
		}
	}
}

void ToolbarWnd::OnOmniboxClicked()
{
	SetFocus(_hWnd);
	_omnibox->SetFocus();
}

void ToolbarWnd::OnLButtonDown(int x, int y)
{
	if (_doc) {
		BOOL process = TRUE;
		if (_omnibox && _omnibox->OnLButtonDown(x, y))
			process = FALSE;
		if (process && !_inCapture) {
			position::vector redraw_boxes;
			if (_doc->on_lbutton_down(x, y, x, y, redraw_boxes)) {
				for (position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
					RECT rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRect(_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindow(_hWnd);
			}
		}
	}
}

void ToolbarWnd::OnLButtonUp(int x, int y)
{
	if (_doc) {
		BOOL process = TRUE;
		if (_omnibox && _omnibox->OnLButtonUp(x, y))
			process = FALSE;
		if (process && !_inCapture) {
			position::vector redraw_boxes;
			if (_doc->on_lbutton_up(x, y, x, y, redraw_boxes)) {
				for (position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
					RECT rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRect(_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindow(_hWnd);
			}
		}
	}
}

struct
{
	LPCWSTR	name;
	LPCWSTR	url;
} g_bookmarks[] =
{
	{L"DMOZ",					L"http://www.dmoz.org/"},
	{L"litehtml project",		L"https://github.com/litehtml/litehtml"},
	{L"litehtml website",		L"http://www.litehtml.com/"},
	{L"True Launch Bar",		L"http://www.truelaunchbar.com/"},
	{L"Tordex",					L"http://www.tordex.com/"},
	{L"True Paste",				L"http://www.truepaste.com/"},
	{L"Text Accelerator",		L"http://www.textaccelerator.com/"},
	{L"Wiki: Web Browser",		L"http://en.wikipedia.org/wiki/Web_browser"},
	{L"Wiki: Obama",			L"http://en.wikipedia.org/wiki/Obama"},
	{L"Code Project",			L"http://www.codeproject.com/"},

	{NULL,						NULL},
};

void ToolbarWnd::OnAnchorClick(const tchar_t *url, const element::ptr &el)
{
	if (!str_cmp(url, _t("back")))
		_parent->Back();
	else if (!str_cmp(url, _t("forward")))
		_parent->Forward();
	else if (!str_cmp(url, _t("reload")))
		_parent->Reload();
	else if (!str_cmp(url, _t("bookmarks"))) {
		position pos = el->get_placement();
		POINT pt;
		pt.x = pos.right();
		pt.y = pos.bottom();
		MapWindowPoints(_hWnd, NULL, &pt, 1);

		HMENU hMenu = CreatePopupMenu();
		for (int i = 0; g_bookmarks[i].url; i++)
			InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, i + 1, g_bookmarks[i].name);
		int ret = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, _hWnd, NULL);
		DestroyMenu(hMenu);

		if (ret)
			_parent->Open(g_bookmarks[ret - 1].url);
	}
	else if (!str_cmp(url, _t("settings"))) {
		position pos = el->get_placement();
		POINT pt;
		pt.x = pos.right();
		pt.y = pos.bottom();
		MapWindowPoints(_hWnd, NULL, &pt, 1);

		HMENU hMenu = CreatePopupMenu();
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 1, L"Calculate Render Time");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 3, L"Calculate Render Time (10)");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 4, L"Calculate Render Time (100)");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, L"");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 2, L"Exit");
		int ret = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, _hWnd, NULL);
		DestroyMenu(hMenu);

		switch (ret) {
		case 2: PostQuitMessage(0); break;
		case 1: _parent->CalcTime(); break;
		case 3: _parent->CalcTime(10); break;
		case 4: _parent->CalcTime(100); break;
		}
	}
}

void ToolbarWnd::SetCursor(const tchar_t *cursor)
{
	_cursor = cursor;
}

std::shared_ptr<element> ToolbarWnd::CreateElement(const tchar_t *tag_name, const string_map &attributes, const std::shared_ptr<document> &doc)
{
	if (!t_strcasecmp(tag_name, _t("input"))) {
		auto iter = attributes.find(_t("type"));
		if (iter != attributes.end()) {
			if (!t_strcasecmp(iter->second.c_str(), _t("text"))) {
				if (_omnibox)
					_omnibox = nullptr;
				_omnibox = std::make_shared<el_omnibox>(doc, _hWnd, this);
				return _omnibox;
			}
		}
	}
	return 0;
}

void ToolbarWnd::ImportCss(tstring &text, const tstring &url, tstring &baseurl)
{
}

void ToolbarWnd::GetClientRect(position &client) const
{
	RECT rcClient;
	::GetClientRect(_hWnd, &rcClient);
	client.x = rcClient.left;
	client.y = rcClient.top;
	client.width = rcClient.right - rcClient.left;
	client.height = rcClient.bottom - rcClient.top;
}

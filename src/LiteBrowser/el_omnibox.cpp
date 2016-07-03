#include "globals.h"
#include "el_omnibox.h"
#include <Richedit.h>
#include <strsafe.h>

el_omnibox::el_omnibox(const std::shared_ptr<document> &doc, HWND parent, cairo_container *container) : html_tag(doc), _edit(parent, container)
{
	_hWndParent = parent;
	_haveFocus = FALSE;
}

el_omnibox::~el_omnibox()
{
}

void el_omnibox::update_position()
{
	position pos = element::get_placement();
	RECT rcPos;
	rcPos.left = pos.left();
	rcPos.right = pos.right();
	rcPos.top = pos.top();
	rcPos.bottom = pos.bottom();
	_edit.setRect(&rcPos);
}

void el_omnibox::set_url(LPCWSTR url)
{
	_edit.setText(url);
}

void el_omnibox::draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip)
{
	html_tag::draw(hdc, x, y, clip);
	_edit.draw((cairo_t*)hdc);
}

void el_omnibox::parse_styles(bool is_reparse)
{
	html_tag::parse_styles(is_reparse);
	_edit.setFont((cairo_font*)get_font(), get_color(_t("color"), true));
}

void el_omnibox::set_parent(HWND parent)
{
	_hWndParent = parent;
	_edit.set_parent(parent);
}

void el_omnibox::on_click()
{
	if (!_haveFocus)
		SendMessage(_hWndParent, WM_OMNIBOX_CLICKED, 0, 0);
}

void el_omnibox::SetFocus()
{
	_edit.showCaret();
	_edit.setSelection(0, -1);
	_haveFocus = TRUE;
}

void el_omnibox::KillFocus()
{
	_edit.setSelection(0, 0);
	_edit.hideCaret();
	_haveFocus = FALSE;
}

std::wstring el_omnibox::get_url()
{
	std::wstring str = _edit.getText();

	if (!PathIsURL(str.c_str())) {
		DWORD sz = (DWORD)str.length() + 32;
		LPWSTR outUrl = new WCHAR[sz];
		HRESULT res = UrlApplyScheme(str.c_str(), outUrl, &sz, URL_APPLY_DEFAULT);
		if (res == E_POINTER) {
			delete outUrl;
			LPWSTR outUrl = new WCHAR[sz];
			if (UrlApplyScheme(str.c_str(), outUrl, &sz, URL_APPLY_DEFAULT) == S_OK)
				str = outUrl;
		}
		else if (res == S_OK)
			str = outUrl;
		delete outUrl;
	}
	return str;
}

BOOL el_omnibox::OnLButtonDown(int x, int y)
{
	if (have_focus()) {
		position pos = element::get_placement();
		if (_edit.in_capture() || pos.is_point_inside(x, y)) {
			_edit.OnLButtonDown(x, y);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL el_omnibox::OnLButtonUp(int x, int y)
{
	if (have_focus()) {
		position pos = element::get_placement();
		if (_edit.in_capture() || pos.is_point_inside(x, y)) {
			_edit.OnLButtonUp(x, y);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL el_omnibox::OnLButtonDblClick(int x, int y)
{
	if (have_focus()) {
		position pos = element::get_placement();
		if (pos.is_point_inside(x, y)) {
			_edit.OnLButtonDblClick(x, y);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL el_omnibox::OnMouseMove(int x, int y)
{
	if (have_focus())
	{
		position pos = element::get_placement();
		if (_edit.in_capture() || pos.is_point_inside(x, y)) {
			_edit.OnMouseMove(x, y);
			return TRUE;
		}
	}
	return FALSE;
}

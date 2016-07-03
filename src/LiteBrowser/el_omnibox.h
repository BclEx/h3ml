#pragma once
#include "sl_edit.h"

#define WM_OMNIBOX_CLICKED	(WM_USER + 10002)

using namespace litehtml;
class el_omnibox : public html_tag
{
	SingleLineEditCtrl _edit;
	HWND _hWndParent;
	BOOL _haveFocus;
public:
	el_omnibox(const std::shared_ptr<document> &doc, HWND parent, cairo_container *container);
	~el_omnibox();

	virtual void draw(uint_ptr hdc, int x, int y, const position *clip);
	virtual void parse_styles(bool is_reparse);
	virtual void on_click();

	BOOL have_focus() { return _haveFocus; }
	void update_position();
	void set_url(LPCWSTR url);
	std::wstring get_url();
	void set_parent(HWND parent);
	void SetFocus();
	void KillFocus();
	void select_all()
	{
		_edit.setSelection(0, -1);
	}
	BOOL OnKeyDown(WPARAM wParam, LPARAM lParam)
	{
		return _edit.OnKeyDown(wParam, lParam);
	}
	BOOL OnKeyUp(WPARAM wParam, LPARAM lParam)
	{
		return _edit.OnKeyUp(wParam, lParam);
	}
	BOOL OnChar(WPARAM wParam, LPARAM lParam)
	{
		return _edit.OnChar(wParam, lParam);
	}
	BOOL OnLButtonDown(int x, int y);
	BOOL OnLButtonUp(int x, int y);
	BOOL OnLButtonDblClick(int x, int y);
	BOOL OnMouseMove(int x, int y);
};

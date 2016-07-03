#pragma once
#include "TxThread.h"
#include "../containers/cairo/cairo_container.h"
#include "../containers/cairo/cairo_font.h"

#define WM_UPDATE_CONTROL	(WM_USER + 2001)
#define WM_EDIT_ACTIONKEY	(WM_USER + 2003)
#define WM_EDIT_CAPTURE		(WM_USER + 2004)

using namespace litehtml;
class SingleLineEditCtrl : public TxThread
{
private:
	cairo_container *_container;
	HWND			_parent;
	std::wstring	_text;
	cairo_font		*_hFont;
	web_color		_textColor;
	int				_lineHeight;
	int				_caretPos;
	int				_leftPos;
	BOOL			_caretIsCreated;
	int				_selStart;
	int				_selEnd;
	BOOL			_inCapture;
	int				_startCapture;
	int				_width;
	int				_height;
	int				_caretX;
	BOOL			_showCaret;
	RECT			_rcText;

public:
	SingleLineEditCtrl(HWND parent, cairo_container *container);
	virtual ~SingleLineEditCtrl(void);

	BOOL	OnKeyDown(WPARAM wParam, LPARAM lParam);
	BOOL	OnKeyUp(WPARAM wParam, LPARAM lParam);
	BOOL	OnChar(WPARAM wParam, LPARAM lParam);
	void	OnLButtonDown(int x, int y);
	void	OnLButtonUp(int x, int y);
	void	OnLButtonDblClick(int x, int y);
	void	OnMouseMove(int x, int y);
	void	setRect(LPRECT rcText);
	void	setText(LPCWSTR text);
	LPCWSTR getText() { return _text.c_str(); }
	void	setFont(cairo_font *font, web_color &color);
	void	draw(cairo_t* cr);
	void	setSelection(int start, int end);
	void	replaceSel(LPCWSTR text);
	void	hideCaret();
	void	showCaret();
	void	set_parent(HWND parent);
	BOOL	in_capture() { return _inCapture; }

	virtual DWORD ThreadProc();
private:
	void	UpdateCarret();
	void	UpdateControl();
	void	delSelection();
	void	createCaret();
	void	destroyCaret();
	void	setCaretPos(int pos);
	void	fillSelRect(cairo_t *cr, LPRECT rcFill);
	int		getCaretPosXY(int x, int y);

	void	drawText(cairo_t *cr, LPCWSTR text, int cbText, LPRECT rcText, web_color textColor);
	void	getTextExtentPoint(LPCWSTR text, int cbText, LPSIZE sz);
	void	set_color(cairo_t *cr, web_color color) { cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0); }
};

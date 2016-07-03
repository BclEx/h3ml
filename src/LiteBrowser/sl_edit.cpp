#include "globals.h"
#include "sl_edit.h"
#include "ctrl_container.h"

SingleLineEditCtrl::SingleLineEditCtrl(HWND parent, cairo_container *container) : _textColor(0, 0, 0)
{
	_parent = parent;
	_container = container;
	_leftPos = 0;
	_parent = _parent;
	_caretPos = 0;
	_caretIsCreated = FALSE;
	_selStart = -1;
	_selEnd = -1;
	_inCapture = FALSE;
	_startCapture = -1;
	_width = 0;
	_height = 0;
	_caretX = 0;
	_showCaret = TRUE;
	_hFont = NULL;
	_lineHeight = 0;
}

SingleLineEditCtrl::~SingleLineEditCtrl(void)
{
	Stop();
}

BOOL SingleLineEditCtrl::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	UINT key = (UINT)wParam;
	switch (key)
	{
	case 'A': //A
		if (GetKeyState(VK_CONTROL) & 0x8000)
			setSelection(0, -1);
	case 'C': //C
		if (GetKeyState(VK_CONTROL) & 0x8000) {
			if (OpenClipboard(_parent)) {
				EmptyClipboard();
				std::wstring strCopy;
				if (_selStart >= 0)
				{
					int start = min(_selStart, _selEnd);
					int end = max(_selStart, _selEnd);
					strCopy = _text.substr(start, end - start);
				}
				else
					strCopy = _text;
				HGLOBAL hText = GlobalAlloc(GHND, (strCopy.length() + 1) * sizeof(TCHAR));
				LPWSTR text = (LPWSTR)GlobalLock((HGLOBAL)hText);
				lstrcpy(text, strCopy.c_str());
				GlobalUnlock(hText);
				SetClipboardData(CF_UNICODETEXT, hText);
				CloseClipboard();
			}
			return 0;
		}
		break;
	case 0x58: //X
		if (GetKeyState(VK_CONTROL) & 0x8000) {
			if (OpenClipboard(_parent)) {
				EmptyClipboard();
				std::wstring strCopy;
				if (_selStart >= 0) {
					int start = min(_selStart, _selEnd);
					int end = max(_selStart, _selEnd);
					strCopy = _text.substr(start, end - start);
					delSelection();
				}
				else {
					strCopy = _text;
					setText(L"");
				}
				HGLOBAL hText = GlobalAlloc(GHND, (strCopy.length() + 1) * sizeof(TCHAR));
				LPWSTR text = (LPWSTR)GlobalLock((HGLOBAL)hText);
				lstrcpy(text, strCopy.c_str());
				GlobalUnlock(hText);
				SetClipboardData(CF_UNICODETEXT, hText);
				CloseClipboard();
			}
			return 0;
		}
		break;
	case 0x56: //V
		if (GetKeyState(VK_CONTROL) & 0x8000) {
			if (OpenClipboard(_parent)) {
				HANDLE hText = GetClipboardData(CF_UNICODETEXT);
				if (hText) {
					LPWSTR text = (LPWSTR)GlobalLock((HGLOBAL)hText);
					replaceSel(text);
					_caretPos += lstrlen(text);
					GlobalUnlock(hText);
					UpdateCarret();
				}
				CloseClipboard();
			}
			return 0;
		}
		break;
	case 0x2D: //Insert
		if (GetKeyState(VK_SHIFT) & 0x8000) {
			if (OpenClipboard(_parent)) {
				HANDLE hText = GetClipboardData(CF_UNICODETEXT);
				if (hText) {
					LPWSTR text = (LPWSTR)GlobalLock((HGLOBAL)hText);
					replaceSel(text);
					_caretPos += lstrlen(text);
					GlobalUnlock(hText);
					UpdateCarret();
				}
				CloseClipboard();
			}
			return 0;
		}
		break;
	case VK_RETURN:
		::PostMessage(_parent, WM_EDIT_ACTIONKEY, VK_RETURN, 0);
		return 0;
	case VK_ESCAPE:
		::PostMessage(_parent, WM_EDIT_ACTIONKEY, VK_ESCAPE, 0);
		return 0;
	case VK_BACK:
		Stop();
		if (_text.length() && _caretPos > 0) {
			if (_selStart < 0) {
				_text.erase(_caretPos - 1, 1);
				_caretPos--;
				UpdateCarret();
			}
			else
				delSelection();
		}
		return 0;
	case VK_DELETE:
		Stop();
		if (_selStart < 0) {
			if (_caretPos < (int)_text.length()) {
				_text.erase(_caretPos, 1);
				UpdateControl();
			}
		}
		else
			delSelection();
		return 0;
	case VK_UP:
	case VK_LEFT:
		Stop();
		if (_caretPos > 0) {
			int oldCaretPos = _caretPos;
			if (GetKeyState(VK_CONTROL) & 0x8000) {
				int newPos = _caretPos - 1;
				for (; newPos > 0; newPos--) {
					if (!_istalnum(_text[newPos]))
						break;
				}
				_caretPos = newPos;
			}
			else
				_caretPos--;
			if (GetKeyState(VK_SHIFT) & 0x8000) {
				if (_selStart >= 0)
					setSelection(_selStart, _caretPos);
				else
					setSelection(oldCaretPos, _caretPos);
			}
			else
				setSelection(-1, 0);
			UpdateCarret();
		}
		else
			if (!(GetKeyState(VK_SHIFT) & 0x8000))
				setSelection(-1, 0);
		return 0;
	case VK_DOWN:
	case VK_RIGHT:
		Stop();
		if (_caretPos < (int)_text.length()) {
			int oldCaretPos = _caretPos;
			if (GetKeyState(VK_CONTROL) & 0x8000) {
				int newPos = _caretPos + 1;
				for (; newPos < (int)_text.length(); newPos++)
					if (!_istalnum(_text[newPos]))
						break;
				_caretPos = newPos;
			}
			else
				_caretPos++;
			if (GetKeyState(VK_SHIFT) & 0x8000) {
				if (_selStart >= 0)
					setSelection(_selStart, _caretPos);
				else
					setSelection(oldCaretPos, _caretPos);
			}
			else
				setSelection(-1, 0);
			UpdateCarret();
		}
		else
			if (!(GetKeyState(VK_SHIFT) & 0x8000))
				setSelection(-1, 0);
		return 0;
	case VK_HOME: {
		int oldCaretPos = _caretPos;
		_caretPos = 0;
		UpdateCarret();

		if (GetKeyState(VK_SHIFT) & 0x8000) {
			if (_selStart >= 0)
				setSelection(_selStart, _caretPos);
			else
				setSelection(oldCaretPos, _caretPos);
		}
		else
			setSelection(-1, 0);
		return 0;
	}
	case VK_END: {
		int oldCaretPos = _caretPos;
		_caretPos = (int)_text.length();
		UpdateCarret();

		if (GetKeyState(VK_SHIFT) & 0x8000) {
			if (_selStart >= 0)
				setSelection(_selStart, _caretPos);
			else
				setSelection(oldCaretPos, _caretPos);
		}
		else
			setSelection(-1, 0);
		return 0;
	}
	}
	return TRUE;
}

BOOL SingleLineEditCtrl::OnChar(WPARAM wParam, LPARAM lParam)
{
	WCHAR ch = (WCHAR)wParam;
	if (ch > 13 && ch != 27 && !(GetKeyState(VK_CONTROL) & 0x8000)) {
		delSelection();
		_text.insert(_text.begin() + _caretPos, ch);
		_caretPos++;
		UpdateCarret();
	}
	return TRUE;
}

void SingleLineEditCtrl::setRect(LPRECT rcText)
{
	_width = rcText->right - rcText->left;
	_height = rcText->bottom - rcText->top;
	_rcText = *rcText;
	_rcText.top = rcText->top + _height / 2 - _lineHeight / 2;
	_rcText.bottom = _rcText.top + _lineHeight;
	//createCaret();
}

void SingleLineEditCtrl::draw(cairo_t *cr)
{
	int selStart = min(_selStart, _selEnd);
	int selEnd = max(_selStart, _selEnd);

	RECT rcText = _rcText;

	if (_selStart >= 0) {
		if (selStart < _leftPos)
			selStart = _leftPos;

		int left = 0;
		// draw left side of the text
		if (selStart > 0) {
			if (selStart > _leftPos) {
				rcText.left = _rcText.left + left;
				SIZE sz = { 0, 0 };
				getTextExtentPoint(_text.c_str() + _leftPos, selStart - _leftPos, &sz);
				drawText(cr, _text.c_str() + _leftPos, selStart - _leftPos, &rcText, _textColor);
				left += sz.cx;
			}
		}
		// draw the selection
		if (selStart < selEnd) {
			SIZE sz = { 0, 0 };
			getTextExtentPoint(_text.c_str() + selStart, selEnd - selStart, &sz);
			rcText = _rcText;
			RECT rcFill;
			rcFill.left = _rcText.left + left;
			rcFill.right = rcFill.left + sz.cx;
			rcFill.top = _rcText.top + (_rcText.bottom - _rcText.top) / 2 - _lineHeight / 2;
			rcFill.bottom = rcFill.top + _lineHeight;
			if (rcFill.right > _rcText.right)
				rcFill.right = _rcText.right;
			fillSelRect(cr, &rcFill);

			rcText.left = _rcText.left + left;
			rcText.right = rcText.left + sz.cx;
			if (rcText.right > _rcText.right)
				rcText.right = _rcText.right;
			COLORREF clr = GetSysColor(COLOR_HIGHLIGHTTEXT);
			drawText(cr, _text.c_str() + selStart, selEnd - selStart, &rcText, litehtml::web_color(GetRValue(clr), GetGValue(clr), GetBValue(clr)));

			left += sz.cx;
		}
		// draw the right side of the text
		if (selEnd <= _text.length()) {
			rcText.left = _rcText.left + left;
			rcText.right = _rcText.right;
			if (rcText.left < rcText.right)
				drawText(cr, _text.c_str() + selEnd, -1, &rcText, _textColor);
		}
	}
	else
		drawText(cr, _text.c_str() + _leftPos, -1, &rcText, _textColor);

	if (_showCaret && _caretIsCreated) {
		cairo_save(cr);

		int caretWidth = GetSystemMetrics(SM_CXBORDER);
		int caretHeight = _lineHeight;
		int top = _rcText.top + (_rcText.bottom - rcText.top) / 2 - caretHeight / 2;

		cairo_set_source_rgba(cr, _textColor.red / 255.0, _textColor.green / 255.0, _textColor.blue / 255.0, _textColor.alpha / 255.0);
		cairo_rectangle(cr, _rcText.left + _caretX, top, caretWidth, caretHeight);
		cairo_fill(cr);

		cairo_restore(cr);
	}
}

void SingleLineEditCtrl::setFont(cairo_font *font, web_color &color)
{
	_hFont = font;
	_textColor = color;
	_lineHeight = font->Metrics().height;
}

void SingleLineEditCtrl::UpdateCarret()
{
	if (_caretPos < _leftPos) _leftPos = _caretPos;

	SIZE sz = { 0, 0 };
	getTextExtentPoint(_text.c_str() + _leftPos, _caretPos - _leftPos, &sz);

	_caretX = sz.cx;

	while (_caretX > _width - 2 && _leftPos < _text.length()) {
		_leftPos++;
		getTextExtentPoint(_text.c_str() + _leftPos, _caretPos - _leftPos, &sz);
		_caretX = sz.cx;
	}

	if (_caretX < 0) _caretX = 0;

	_showCaret = TRUE;
	UpdateControl();
}

void SingleLineEditCtrl::UpdateControl()
{
	if (_parent)
		SendMessage(_parent, WM_UPDATE_CONTROL, 0, 0);
}

void SingleLineEditCtrl::delSelection()
{
	if (_selStart < 0) return;
	int start = min(_selStart, _selEnd);
	int end = max(_selStart, _selEnd);

	_text.erase(start, end - start);

	_caretPos = start;
	_selStart = -1;
	UpdateCarret();
	UpdateControl();
}

void SingleLineEditCtrl::setSelection(int start, int end)
{
	_selStart = start;
	_selEnd = end;
	if (_selEnd < 0) _selEnd = (int)_text.length();
	if (_selStart >= 0) {
		_caretPos = _selEnd;
		if (_caretPos < 0)
			_caretPos = (int)_text.length();
		else
			if (_caretPos > (int)_text.length())
				_caretPos = (int)_text.length();
		if (_selStart > (int)_text.length())
			_selStart = (int)_text.length();
		if (_selEnd > (int)_text.length())
			_selEnd = (int)_text.length();
		if (_selEnd == _selStart)
			_selStart = -1;
	}
	UpdateCarret();
}

void SingleLineEditCtrl::replaceSel(LPCWSTR text)
{
	delSelection();
	_text.insert(_caretPos, text);
}

void SingleLineEditCtrl::createCaret()
{
	_caretIsCreated = TRUE;
	Run();
}

void SingleLineEditCtrl::destroyCaret()
{
	_caretIsCreated = FALSE;
}

void SingleLineEditCtrl::setCaretPos(int pos)
{
	_caretPos = pos;
	UpdateCarret();
}

void SingleLineEditCtrl::fillSelRect(cairo_t* cr, LPRECT rcFill)
{
	cairo_save(cr);

	COLORREF clr = GetSysColor(COLOR_HIGHLIGHT);
	litehtml::web_color color(GetRValue(clr), GetGValue(clr), GetBValue(clr));

	cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
	cairo_rectangle(cr, rcFill->left, rcFill->top, rcFill->right - rcFill->left, rcFill->bottom - rcFill->top);
	cairo_fill(cr);

	cairo_restore(cr);
}

int SingleLineEditCtrl::getCaretPosXY(int x, int y)
{
	int pos = -1;
	int w = 0;

	for (int i = 1; i < (int)_text.length(); i++) {
		SIZE sz;
		getTextExtentPoint(_text.c_str(), i, &sz);
		if (x > w && x < w + (sz.cx - w) / 2) {
			pos = i - 1;
			break;
		}
		else if (x >= w + (sz.cx - w) / 2 && x <= sz.cx) {
			pos = i;
			break;
		}
		w = sz.cx;
	}
	if (pos < 0)
		pos = (x > 0 ? (int)_text.length() : 0);
	return pos;
}

void SingleLineEditCtrl::setText(LPCWSTR text)
{
	_caretPos = 0;
	_text = text;
	_selStart = -1;
	_selEnd = -1;
	_leftPos = 0;

	SIZE sz;
	getTextExtentPoint(_text.c_str(), -1, &sz);
	_lineHeight = sz.cy;

	UpdateControl();
}

void SingleLineEditCtrl::drawText(cairo_t *cr, LPCWSTR text, int cbText, LPRECT rcText, web_color textColor)
{
	std::wstring str;
	if (cbText < 0)
		str = text;
	else
		str.append(text, cbText);

	position pos;
	pos.x = rcText->left;
	pos.y = rcText->top;
	pos.width = rcText->right - rcText->left;
	pos.height = rcText->bottom - rcText->top;

#ifndef LITEHTML_UTF8
	_container->DrawText((uint_ptr)cr, str.c_str(), (uint_ptr)_hFont, textColor, pos);
#else
	LPSTR str_utf8 = cairo_font::WcharToUtf8(str.c_str());
	_container->DrawText((uint_ptr)cr, str_utf8, (uint_ptr)_hFont, textColor, pos);
	delete str_utf8;
#endif
}

void SingleLineEditCtrl::getTextExtentPoint(LPCWSTR text, int cbText, LPSIZE sz)
{
	std::wstring str;
	if (cbText < 0)
		str = text;
	else
		str.append(text, cbText);
#ifndef LITEHTML_UTF8
	sz->cx = _container->TextWidth(str.c_str(), (uint_ptr)_hFont);
#else
	LPSTR str_utf8 = cairo_font::WcharToUtf8(str.c_str());
	sz->cx = _container->TextWidth(str_utf8, (uint_ptr)_hFont);
	delete str_utf8;
#endif
	sz->cy = _hFont->Metrics().height;
}

DWORD SingleLineEditCtrl::ThreadProc()
{
	_showCaret = TRUE;
	UINT blinkTime = GetCaretBlinkTime();
	if (!blinkTime)	blinkTime = 500;
	while (!WaitForStop(blinkTime)) {
		_showCaret = (_showCaret ? FALSE : TRUE);
		UpdateControl();
	}
	_showCaret = TRUE;
	return 0;
}

BOOL SingleLineEditCtrl::OnKeyUp(WPARAM wParam, LPARAM lParam)
{
	UINT key = (UINT)wParam;
	switch (key) {
	case VK_BACK: Run(); return 0;
	case VK_DELETE: Run(); return 0;
	case VK_DOWN: Run(); return 0;
	case VK_UP: Run(); return 0;
	case VK_LEFT: Run(); return 0;
	case VK_RIGHT: Run(); return 0;
	}
	return TRUE;
}

void SingleLineEditCtrl::OnLButtonDown(int x, int y)
{
	_caretPos = getCaretPosXY(x - _rcText.left, y - _rcText.top);
	setSelection(-1, _caretPos);
	SendMessage(_parent, WM_EDIT_CAPTURE, TRUE, 0);
	_inCapture = TRUE;
	_startCapture = _caretPos;
}

void SingleLineEditCtrl::OnLButtonUp(int x, int y)
{
	if (_inCapture) {
		SendMessage(_parent, WM_EDIT_CAPTURE, FALSE, 0);
		_inCapture = FALSE;
	}
}

void SingleLineEditCtrl::OnLButtonDblClick(int x, int y)
{
	int pos = getCaretPosXY(x - _rcText.left, y - _rcText.top);
	int start = pos;
	int end = pos;
	for (; start > 0; start--)
		if (!_istalnum(_text[start])) {
			start++;
			break;
		}
	for (; end < (int)_text.length(); end++)
		if (!_istalnum(_text[end]))
			break;
	if (start < end)
		setSelection(start, end);
}

void SingleLineEditCtrl::OnMouseMove(int x, int y)
{
	if (_inCapture) {
		_caretPos = getCaretPosXY(x - _rcText.left, y - _rcText.top);
		setSelection(_startCapture, _caretPos);
	}
}

void SingleLineEditCtrl::hideCaret()
{
	Stop();
	_showCaret = FALSE;
	UpdateControl();
}

void SingleLineEditCtrl::showCaret()
{
	_showCaret = TRUE;
	createCaret();
	UpdateControl();
}

void SingleLineEditCtrl::set_parent(HWND parent)
{
	_parent = parent;
}

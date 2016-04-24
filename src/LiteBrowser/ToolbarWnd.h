#pragma once
#include "..\containers\cairo\cairo_container.h"
#include "Dib.h"
#include "el_omnibox.h"

#define TOOLBARWND_CLASS L"TOOLBAR_WINDOW"

class CBrowserWnd;
class CToolbarWnd : public cairo_container
{
	HWND _hWnd;
	HINSTANCE _hInst;
	litehtml::context _context;
	litehtml::document::ptr	_doc;
	CBrowserWnd *_parent;
	std::shared_ptr<el_omnibox>	_omnibox;
	litehtml::tstring _cursor;
	BOOL _inCapture;
public:
	CToolbarWnd(HINSTANCE hInst, CBrowserWnd *parent);
	virtual ~CToolbarWnd();

	void Create(int x, int y, int width, HWND parent);
	HWND Wnd() { return _hWnd; }
	int Height() { return (_doc ? _doc->height() : 0); }
	int SetWidth(int width);
	void OnPageLoaded(LPCWSTR url);

	// cairo_container members
	virtual void MakeUrl(LPCWSTR url, LPCWSTR basepath, std::wstring &out);
	virtual cairo_container::image_ptr GetImage(LPCWSTR url, bool redrawOnReady);

	// litehtml::document_container members
	virtual	void SetCaption(const litehtml::tchar_t *caption);
	virtual	void SetBaseUrl(const litehtml::tchar_t *baseUrl);
	virtual	void Link(std::shared_ptr<litehtml::document> &doc, litehtml::element::ptr el);
	virtual void ImportCss(litehtml::tstring &text, const litehtml::tstring &url, litehtml::tstring &baseUrl);
	virtual	void OnAnchorClick(const litehtml::tchar_t *url, const litehtml::element::ptr &el);
	virtual	void SetCursor(const litehtml::tchar_t *cursor);
	virtual std::shared_ptr<litehtml::element> CreateElement(const litehtml::tchar_t *tagName, const litehtml::string_map &attributes, const std::shared_ptr<litehtml::document> &doc);

protected:
	virtual void OnCreate();
	virtual void OnPaint(Dib *dib, LPRECT rcDraw);
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();
	virtual void OnMouseMove(int x, int y);
	virtual void OnLButtonDown(int x, int y);
	virtual void OnLButtonUp(int x, int y);
	virtual void OnMouseLeave();
	virtual void OnOmniboxClicked();

	virtual void GetClientRect(litehtml::position &client) const;

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	void RenderToolbar(int width);
	void UpdateCursor();
};

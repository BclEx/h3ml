#pragma once
#include "..\containers\cairo\cairo_container.h"
#include "Dib.h"
#include "el_omnibox.h"

#define TOOLBARWND_CLASS L"TOOLBAR_WINDOW"

using namespace litehtml;
class BrowserWnd;
class ToolbarWnd : public cairo_container
{
	HWND _hWnd;
	HINSTANCE _hInst;
	context _context;
	document::ptr _doc;
	BrowserWnd *_parent;
	std::shared_ptr<el_omnibox>	_omnibox;
	tstring _cursor;
	BOOL _inCapture;
public:
	ToolbarWnd(HINSTANCE hInst, BrowserWnd *parent);
	virtual ~ToolbarWnd();

	void Create(int x, int y, int width, HWND parent);
	HWND Wnd() { return _hWnd; }
	int Height() { return (_doc ? _doc->height() : 0); }
	int SetWidth(int width);
	void OnPageLoaded(LPCWSTR url);

	// cairo_container members
	virtual void MakeUrl(LPCWSTR url, LPCWSTR basepath, std::wstring &out);
	virtual cairo_container::image_ptr GetImage(LPCWSTR url, bool redrawOnReady);

	// document_container members
	virtual	void SetCaption(const tchar_t *caption);
	virtual	void SetBaseUrl(const tchar_t *baseUrl);
	virtual	void Link(std::shared_ptr<document> &doc, element::ptr el);
	virtual void ImportCss(tstring &text, const tstring &url, tstring &baseUrl);
	virtual	void OnAnchorClick(const tchar_t *url, const element::ptr &el);
	virtual	void SetCursor(const tchar_t *cursor);
	virtual std::shared_ptr<element> CreateElement(const tchar_t *tagName, const string_map &attributes, const std::shared_ptr<document> &doc);

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

	virtual void GetClientRect(position &client) const;

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	void RenderToolbar(int width);
	void UpdateCursor();
};

#pragma once
#include "../containers/cairo/cairo_container.h"
#include "../containers/cairo/cairo_font.h"
#include "Http.h"

using namespace litehtml;
class CHTMLViewWnd;
class WebPage : public cairo_container
{
	CHTMLViewWnd * _parent;
	LONG _refCount;
public:
	Http _http;
	std::wstring _url;
	document::ptr _doc;
	std::wstring _caption;
	std::wstring _cursor;
	std::wstring _base_path;
	HANDLE _hWaitDownload;
	std::wstring _waited_file;
	std::wstring _hash;
public:
	WebPage(CHTMLViewWnd *parent);
	virtual ~WebPage();

	void Load(LPCWSTR url);
	void OnDocumentLoaded(LPCWSTR file, LPCWSTR encoding, LPCWSTR realUrl);
	void OnImageLoaded(LPCWSTR file, LPCWSTR url, bool redrawOnly);
	void OnDocumentError(DWORD dwError, LPCWSTR errMsg);
	void OnWaitedFinished(DWORD dwError, LPCWSTR file);
	void AddRef();
	void Release();
	void GetUrl(std::wstring &url);

	// document_container members
	virtual	void SetCaption(const tchar_t *caption);
	virtual	void SetBaseUrl(const tchar_t *baseUrl);
	virtual void ImportCss(tstring &text, const tstring &url, tstring &baseUrl);
	virtual	void OnAnchorClick(const tchar_t *url, const element::ptr &el);
	virtual	void SetCursor(const tchar_t *cursor);

	virtual void MakeUrl(LPCWSTR url, LPCWSTR basepath, std::wstring &out);
	virtual cairo_container::image_ptr GetImage(LPCWSTR url, bool redrawOnReady);
	virtual void GetClientRect(position &client) const;
private:
	LPWSTR LoadTextFile(LPCWSTR path, bool isHtml, LPCWSTR defEncoding = L"UTF-8");
	unsigned char *LoadUtf8File(LPCWSTR path, bool isHtml, LPCWSTR defEncoding = L"UTF-8");
	BOOL DownloadAndWait(LPCWSTR url);
};

enum WebFileType
{
	WebFile_Document,
	WebFile_Image_Redraw,
	WebFile_Image_Rerender,
	WebFile_Waited,
};

class WebFile : public HttpRequest
{
	WCHAR _file[MAX_PATH];
	WebPage *_page;
	WebFileType _type;
	HANDLE _hFile;
	LPVOID _data;
	std::wstring _realUrl;
public:
	WebFile(WebPage *page, WebFileType type, LPVOID data = NULL);
	virtual ~WebFile();

	virtual void OnFinish(DWORD dwError, LPCWSTR errMsg);
	virtual void OnData(LPCBYTE data, DWORD len, ULONG64 downloaded, ULONG64 total);
	virtual void OnHeadersReady(HINTERNET hRequest);
};

#include "globals.h"
#include "WebPage.h"
#include "HtmlViewWnd.h"

WebPage::WebPage(CHTMLViewWnd *parent)
{
	_refCount = 1;
	_parent = parent;
	_http.Open(L"LiteBrowser/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS);
}

WebPage::~WebPage()
{
	_http.Stop();
}

void WebPage::SetCaption(const tchar_t *caption)
{
#ifndef LITEHTML_UTF8
	_caption = caption;
#else
	LPWSTR captionW = cairo_font::utf8_to_wchar(caption);
	_caption = captionW;
	delete captionW;
#endif
}

void WebPage::SetBaseUrl(const tchar_t *baseUrl)
{
#ifndef LITEHTML_UTF8
	if (baseUrl) {
		if (PathIsRelative(baseUrl) && !PathIsURL(baseUrl))
			make_url(baseUrl, _url.c_str(), _base_path);
		else
			_base_path = baseUrl;
	}
	else
		_base_path = _url;
#else
	LPWSTR bu = cairo_font::utf8_to_wchar(baseUrl);
	if (bu) {
		if(PathIsRelative(bu) && !PathIsURL(bu))
			make_url(bu, _url.c_str(), m_base_path);
		else
			_base_path = bu;
	}
	else
		_base_path = _url;
#endif
}

void WebPage::MakeUrl(LPCWSTR url, LPCWSTR basepath, std::wstring &out)
{
	if (PathIsRelative(url) && !PathIsURL(url)) {
		if (basepath && basepath[0]) {
			DWORD dl = lstrlen(url) + lstrlen(basepath) + 1;
			LPWSTR abs_url = new WCHAR[dl];
			HRESULT res = UrlCombine(basepath, url, abs_url, &dl, 0);
			if (res == E_POINTER) {
				delete abs_url;
				abs_url = new WCHAR[dl + 1];
				if (UrlCombine(basepath, url, abs_url, &dl, 0) == S_OK)
					out = abs_url;
			}
			else if (res == S_OK)
				out = abs_url;
			delete abs_url;
		}
		else {
			DWORD dl = lstrlen(url) + (DWORD)_base_path.length() + 1;
			LPWSTR abs_url = new WCHAR[dl];
			HRESULT res = UrlCombine(_base_path.c_str(), url, abs_url, &dl, 0);
			if (res == E_POINTER) {
				delete abs_url;
				abs_url = new WCHAR[dl + 1];
				if (UrlCombine(_base_path.c_str(), url, abs_url, &dl, 0) == S_OK)
					out = abs_url;
			}
			else if (res == S_OK)
				out = abs_url;
			delete abs_url;
		}
	}
	else {
		if (PathIsURL(url))
			out = url;
		else {
			DWORD dl = lstrlen(url) + 1;
			LPWSTR abs_url = new WCHAR[dl];
			HRESULT res = UrlCreateFromPath(url, abs_url, &dl, 0);
			if (res == E_POINTER) {
				delete abs_url;
				abs_url = new WCHAR[dl + 1];
				if (UrlCreateFromPath(url, abs_url, &dl, 0) == S_OK)
					out = abs_url;
			}
			else if (res == S_OK)
				out = abs_url;
			delete abs_url;
		}
	}
	if (out.substr(0, 8) == L"file:///")
		out.erase(5, 1);
	if (out.substr(0, 7) == L"file://")
		out.erase(0, 7);
}

void WebPage::ImportCss(tstring &text, const tstring &url, tstring &baseurl)
{
	std::wstring css_url;
	t_make_url(url.c_str(), baseurl.c_str(), css_url);
	if (DownloadAndWait(css_url.c_str())) {
#ifndef LITEHTML_UTF8
		LPWSTR css = LoadTextFile(_waited_file.c_str(), false, L"UTF-8");
		if (css) {
			baseurl = css_url;
			text = css;
			delete css;
		}
#else
		LPSTR css = (LPSTR)load_utf8_file(_waited_file.c_str(), false, L"UTF-8");
		if (css) {
			LPSTR css_urlA = cairo_font::wchar_to_utf8(css_url.c_str());
			baseurl = css_urlA;
			text = css;
			delete css;
			delete css_urlA;
		}
#endif
	}
}

void WebPage::OnAnchorClick(const tchar_t *url, const element::ptr &el)
{
	std::wstring anchor;
	t_make_url(url, NULL, anchor);
	_parent->Open(anchor.c_str());
}

void WebPage::SetCursor(const tchar_t *cursor)
{
#ifndef LITEHTML_UTF8
	_cursor = cursor;
#else
	LPWSTR v = cairo_font::utf8_to_wchar(cursor);
	if (v) {
		_cursor = v;
		delete v;
	}
#endif
}

cairo_container::image_ptr WebPage::GetImage(LPCWSTR url, bool redrawOnReady)
{
	cairo_container::image_ptr img;
	if (PathIsURL(url)) {
		if (redrawOnReady)
			_http.DownloadFile(url, new WebFile(this, WebFile_Image_Redraw));
		else
			_http.DownloadFile(url, new WebFile(this, WebFile_Image_Rerender));
	}
	else {
		img = cairo_container::image_ptr(new CTxDib);
		if (!img->Load(url))
			img = nullptr;
	}
	return img;
}

void WebPage::Load(LPCWSTR url)
{
	_url = url;
	_base_path = _url;
	if (PathIsURL(url))
		_http.DownloadFile(url, new WebFile(this, WebFile_Document));
	else
		OnDocumentLoaded(url, L"UTF-8", NULL);
}

void WebPage::OnDocumentLoaded(LPCWSTR file, LPCWSTR encoding, LPCWSTR realUrl)
{
	if (realUrl)
		_url = realUrl;

#ifdef LITEHTML_UTF8
	litehtml::byte *htmlText = LoadUtf8File(file, true);
	if (!htmlText) {
		LPCSTR txt = "<h1>Something Wrong</h1>";
		htmlText = new litehtml::byte[lstrlenA(txt) + 1];
		lstrcpyA((LPSTR)htmlText, txt);
	}
	_doc = document::createFromUTF8((const char *)htmlText, this, _parent->GetHtmlContext());
	delete html_text;
#else
	LPWSTR htmlText = LoadTextFile(file, true, encoding);
	if (!htmlText) {
		LPCWSTR txt = L"<h1>Something Wrong</h1>";
		htmlText = new WCHAR[lstrlen(txt) + 1];
		lstrcpy(htmlText, txt);
	}
	_doc = document::createFromString(htmlText, this, _parent->GetHtmlContext());
	delete htmlText;
#endif
	PostMessage(_parent->Wnd(), WM_PAGE_LOADED, 0, 0);
}

LPWSTR WebPage::LoadTextFile(LPCWSTR path, bool isHtml, LPCWSTR defEncoding)
{
	char *utf8 = (char *)LoadUtf8File(path, isHtml, defEncoding);
	if (utf8) {
		int sz = lstrlenA(utf8);
		LPWSTR strW = new WCHAR[sz + 1];
		MultiByteToWideChar(CP_UTF8, 0, utf8, -1, strW, sz + 1);
		delete utf8;
		return strW;
	}
	return NULL;
}

void WebPage::OnDocumentError(DWORD dwError, LPCWSTR errMsg)
{
#ifdef LITEHTML_UTF8
	std::string txt = "<h1>Something Wrong</h1>";
	if (errMsg) {
		LPSTR errMsg_utf8 = cairo_font::wchar_to_utf8(errMsg);
		txt += "<p>";
		txt += errMsg_utf8;
		txt += "</p>";
		delete errMsg_utf8;
	}
	_doc = document::createFromUTF8((const char*)txt.c_str(), this, _parent->GetHtmlContext());
#else
	std::wstring txt = L"<h1>Something Wrong</h1>";
	if (errMsg) {
		txt += L"<p>";
		txt += errMsg;
		txt += L"</p>";
	}
	_doc = document::createFromString(txt.c_str(), this, _parent->GetHtmlContext());
#endif
	PostMessage(_parent->Wnd(), WM_PAGE_LOADED, 0, 0);
}

void WebPage::OnImageLoaded(LPCWSTR file, LPCWSTR url, bool redrawOnly)
{
	cairo_container::image_ptr img = cairo_container::image_ptr(new CTxDib);
	if (img->Load(file)) {
		cairo_container::add_image(std::wstring(url), img);
		if (_doc)
			PostMessage(_parent->Wnd(), WM_IMAGE_LOADED, (WPARAM)(redrawOnly ? 1 : 0), 0);
	}
}

BOOL WebPage::DownloadAndWait(LPCWSTR url)
{
	if (PathIsURL(url)) {
		_waited_file = L"";
		_hWaitDownload = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (_http.DownloadFile(url, new WebFile(this, WebFile_Waited))) {
			WaitForSingleObject(_hWaitDownload, INFINITE);
			CloseHandle(_hWaitDownload);
			return (_waited_file.empty() ? FALSE : TRUE);
		}
	}
	else {
		_waited_file = url;
		return TRUE;
	}
	return FALSE;
}

void WebPage::OnWaitedFinished(DWORD dwError, LPCWSTR file)
{
	if (dwError)
		_waited_file = L"";
	else
		_waited_file = file;
	SetEvent(_hWaitDownload);
}

void WebPage::GetClientRect(position &client) const
{
	_parent->GetClientRect(client);
}

void WebPage::AddRef()
{
	InterlockedIncrement(&_refCount);
}

void WebPage::Release()
{
	LONG lRefCount;
	lRefCount = InterlockedDecrement(&_refCount);
	if (lRefCount == 0)
		delete this;
}

void WebPage::GetUrl(std::wstring &url)
{
	url = _url;
	if (!_hash.empty()) {
		url += "#";
		url += _hash;
	}
}

unsigned char *WebPage::LoadUtf8File(LPCWSTR path, bool isHtml, LPCWSTR defEncoding)
{
	unsigned char *ret = NULL;
	HANDLE fl = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fl != INVALID_HANDLE_VALUE) {
		DWORD size = GetFileSize(fl, NULL);
		ret = new unsigned char[size + 1];

		DWORD cbRead = 0;
		if (size >= 3) {
			ReadFile(fl, ret, 3, &cbRead, NULL);
			if (ret[0] == '\xEF' && ret[1] == '\xBB' && ret[2] == '\xBF') {
				ReadFile(fl, ret, size - 3, &cbRead, NULL);
				ret[cbRead] = 0;
			}
			else {
				ReadFile(fl, ret + 3, size - 3, &cbRead, NULL);
				ret[cbRead + 3] = 0;
			}
		}
		CloseHandle(fl);
	}

	// try to convert encoding
	if (isHtml) {
		std::wstring encoding;
		char *begin = StrStrIA((LPSTR)ret, "<meta");
		while (begin && encoding.empty()) {
			char *end = StrStrIA(begin, ">");
			char *s1 = StrStrIA(begin, "Content-Type");
			if (s1 && s1 < end) {
				s1 = StrStrIA(begin, "charset");
				if (s1) {
					s1 += strlen("charset");
					while (!isalnum(s1[0]) && s1 < end)
						s1++;
					while ((isalnum(s1[0]) || s1[0] == '-') && s1 < end) {
						encoding += s1[0];
						s1++;
					}
				}
			}
			if (encoding.empty())
				begin = StrStrIA(begin + strlen("<meta"), "<meta");
		}
		if (encoding.empty() && defEncoding)
			encoding = defEncoding;
		if (!encoding.empty())
			if (!StrCmpI(encoding.c_str(), L"UTF-8"))
				encoding.clear();
		if (!encoding.empty()) {
			CoInitialize(NULL);

			IMultiLanguage *ml = NULL;
			HRESULT hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (LPVOID *)&ml);	

			MIMECSETINFO charset_src = {0};
			MIMECSETINFO charset_dst = {0};

			BSTR bstrCharSet = SysAllocString(encoding.c_str());
			ml->GetCharsetInfo(bstrCharSet, &charset_src);
			SysFreeString(bstrCharSet);

			bstrCharSet = SysAllocString(L"utf-8");
			ml->GetCharsetInfo(bstrCharSet, &charset_dst);
			SysFreeString(bstrCharSet);

			DWORD dwMode = 0;
			UINT szDst = (UINT)strlen((LPSTR)ret) * 4;
			LPSTR dst = new char[szDst];

			if(ml->ConvertString(&dwMode, charset_src.uiInternetEncoding, charset_dst.uiInternetEncoding, (LPBYTE)ret, NULL, (LPBYTE)dst, &szDst) == S_OK) {
				dst[szDst] = 0;
				delete ret;
				ret = (unsigned char *)dst;
			}
			else
				delete dst;
			CoUninitialize();
		}
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////

WebFile::WebFile(WebPage *page, WebFileType type, LPVOID data)
{
	_data = data;
	_page = page;
	_type = type;
	WCHAR path[MAX_PATH];
	GetTempPath(MAX_PATH, path);
	GetTempFileName(path, L"lbr", 0, _file);
	_hFile = CreateFile(_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	_page->AddRef();
}

WebFile::~WebFile()
{
	if (_hFile) {
		CloseHandle(_hFile);
		_hFile = NULL;
	}
	if (_type != WebFile_Waited)
		DeleteFile(_file);
	if (_page)
		_page->Release();
}

void WebFile::OnFinish(DWORD dwError, LPCWSTR errMsg)
{
	if (_hFile) {
		CloseHandle(_hFile);
		_hFile = NULL;
	}
	if (dwError) {
		std::wstring fileName = _file;
		switch (_type) {
		case WebFile_Document:
			_page->OnDocumentError(dwError, errMsg);
			break;
		case WebFile_Waited:
			_page->OnWaitedFinished(dwError, _file);
			break;
		}
	}
	else {
		switch (_type) {
		case WebFile_Document:
			_page->OnDocumentLoaded(_file, L"UTF-8", _realUrl.empty() ? NULL : _realUrl.c_str());
			break;
		case WebFile_Image_Redraw:
			_page->OnImageLoaded(_file, _url.c_str(), true);
			break;
		case WebFile_Image_Rerender:
			_page->OnImageLoaded(_file, _url.c_str(), false);
			break;
		case WebFile_Waited:
			_page->OnWaitedFinished(dwError, _file);
			break;
		}		
	}
}

void WebFile::OnData(LPCBYTE data, DWORD len, ULONG64 downloaded, ULONG64 total)
{
	if (_hFile) {
		DWORD cbWritten = 0;
		WriteFile(_hFile, data, len, &cbWritten, NULL);
	}
}

void WebFile::OnHeadersReady(HINTERNET hRequest)
{
	WCHAR buf[2048];
	DWORD len = sizeof(buf);
	if (WinHttpQueryOption(_hRequest, WINHTTP_OPTION_URL, buf, &len))
		_realUrl = buf;
}

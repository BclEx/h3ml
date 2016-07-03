#include "cairo_font.h"

cairo_font::cairo_font(IMLangFontLink2 *fl, HFONT hFont, int size)
{
	Init();
	_font_link = fl;
	if (_font_link)
		_font_link->AddRef();
	_size = size;
	SetFont(hFont);
}

cairo_font::cairo_font(IMLangFontLink2 *fl, LPCWSTR facename, int size, int weight, BOOL italic, BOOL strikeout, BOOL underline)
{
	Init();
	_size = size;
	_font_link = fl;
	if (_font_link)
		_font_link->AddRef();

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	if (!lstrcmpi(facename, L"monospace")) wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Courier New");
	else if (!lstrcmpi(facename, L"serif")) wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Times New Roman");
	else if (!lstrcmpi(facename, L"sans-serif")) wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Arial");
	else if (!lstrcmpi(facename, L"fantasy")) wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Impact");
	else if (!lstrcmpi(facename, L"cursive")) wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Comic Sans MS");
	else wcscpy_s(lf.lfFaceName, LF_FACESIZE, facename);

	lf.lfHeight = -size;
	lf.lfWeight = weight;
	lf.lfItalic = italic;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfStrikeOut = strikeout;
	lf.lfUnderline = underline;

	HFONT fnt = CreateFontIndirect(&lf);
	SetFont(fnt);
}

cairo_font::~cairo_font()
{
	if (_font_face)
		cairo_font_face_destroy(_font_face);
	for (size_t i = 0; i < _linked_fonts.size(); i++)
	{
		if (_linked_fonts[i]->hFont)
			_font_link->ReleaseFont(_linked_fonts[i]->hFont);
		if (_linked_fonts[i]->font_face)
			cairo_font_face_destroy(_linked_fonts[i]->font_face);
	}
	_linked_fonts.clear();
	if (_font_link)
		_font_link->AddRef();
	if (_hFont)
		DeleteObject(_hFont);
}

void cairo_font::ShowText(cairo_t *cr, int x, int y, const tchar_t *str)
{
	Lock();
	text_chunk::vector chunks;
	SplitText(str, chunks);
	cairo_set_font_size(cr, _size);
	cairo_move_to(cr, x, y);
	for (size_t i = 0; i < chunks.size(); i++)
	{
		if (chunks[i]->font)
			cairo_set_font_face(cr, chunks[i]->font->font_face);
		else
			cairo_set_font_face(cr, _font_face);
		cairo_show_text(cr, chunks[i]->text);
	}
	Unlock();

	if (_bUnderline)
	{
		int tw = TextWidth(cr, chunks);

		Lock();
		cairo_set_line_width(cr, 1);
		cairo_move_to(cr, x, y + 1.5);
		cairo_line_to(cr, x + tw, y + 1.5);
		cairo_stroke(cr);
		Unlock();
	}
	if (_bStrikeOut)
	{
		int tw = TextWidth(cr, chunks);

		cairo_font_metrics fm;
		GetMetrics(cr, &fm);

		int ln_y = y - fm.x_height / 2;

		Lock();
		cairo_set_line_width(cr, 1);
		cairo_move_to(cr, x, (double)ln_y - 0.5);
		cairo_line_to(cr, x + tw, (double)ln_y - 0.5);
		cairo_stroke(cr);
		Unlock();
	}

	FreeTextChunks(chunks);
}

void cairo_font::SplitText(const tchar_t *src, text_chunk::vector &chunks)
{
	wchar_t* str;
#ifdef LITEHTML_UTF8
	str = cairo_font::Utf8ToWchar(src);
	wchar_t* str_start = str;
#else
	str = (wchar_t*)src;
#endif

	int cch = lstrlen(str);

	HDC hdc = GetDC(NULL);
	SelectObject(hdc, _hFont);
	HRESULT hr = S_OK;
	while (cch > 0)
	{
		DWORD dwActualCodePages;
		long cchActual;
		if (_font_link)
			hr = _font_link->GetStrCodePages(str, cch, _font_code_pages, &dwActualCodePages, &cchActual);
		else
			hr = S_FALSE;

		if (hr != S_OK)
			break;

		text_chunk *chk = new text_chunk;

		int sz = WideCharToMultiByte(CP_UTF8, 0, str, cchActual, chk->text, 0, NULL, NULL) + 1;
		chk->text = new CHAR[sz];
		sz = WideCharToMultiByte(CP_UTF8, 0, str, cchActual, chk->text, sz, NULL, NULL);
		chk->text[sz] = 0;
		chk->font = NULL;

		if (!(dwActualCodePages & _font_code_pages))
		{
			for (linked_font::vector::iterator i = _linked_fonts.begin(); i != _linked_fonts.end(); i++)
			{
				if ((*i)->code_pages == dwActualCodePages)
				{
					chk->font = (*i);
					break;
				}
			}
			if (!chk->font)
			{
				linked_font *lkf = new linked_font;
				lkf->code_pages = dwActualCodePages;
				lkf->hFont = NULL;
				_font_link->MapFont(hdc, dwActualCodePages, 0, &lkf->hFont);
				if (lkf->hFont)
				{
					lkf->font_face = CreateFontFace(lkf->hFont);
					_linked_fonts.push_back(lkf);
				}
				else
				{
					delete lkf;
					lkf = NULL;
				}
				chk->font = lkf;
			}
		}

		chunks.push_back(chk);

		cch -= cchActual;
		str += cchActual;
	}

	if (hr != S_OK)
	{
		text_chunk *chk = new text_chunk;

		int sz = WideCharToMultiByte(CP_UTF8, 0, str, -1, chk->text, 0, NULL, NULL) + 1;
		chk->text = new CHAR[sz];
		sz = WideCharToMultiByte(CP_UTF8, 0, str, -1, chk->text, sz, NULL, NULL);
		chk->text[sz] = 0;
		chk->font = NULL;
		chunks.push_back(chk);
	}

	ReleaseDC(NULL, hdc);

#ifdef LITEHTML_UTF8
	delete str_start;
#endif
}

void cairo_font::FreeTextChunks(text_chunk::vector &chunks)
{
	for (size_t i = 0; i < chunks.size(); i++)
		delete chunks[i];
	chunks.clear();
}

cairo_font_face_t *cairo_font::CreateFontFace(HFONT fnt)
{
	LOGFONT lf;
	GetObject(fnt, sizeof(LOGFONT), &lf);
	return cairo_win32_font_face_create_for_logfontw(&lf);
}

int cairo_font::TextWidth(cairo_t* cr, const tchar_t *str)
{
	text_chunk::vector chunks;
	SplitText(str, chunks);

	int ret = TextWidth(cr, chunks);

	FreeTextChunks(chunks);
	return (int)ret;
}

int cairo_font::TextWidth(cairo_t *cr, text_chunk::vector &chunks)
{
	Lock();
	cairo_set_font_size(cr, _size);
	double ret = 0;
	for (size_t i = 0; i < chunks.size(); i++)
	{
		if (chunks[i]->font)
			cairo_set_font_face(cr, chunks[i]->font->font_face);
		else
			cairo_set_font_face(cr, _font_face);
		cairo_text_extents_t ext;
		cairo_text_extents(cr, chunks[i]->text, &ext);
		ret += ext.x_advance;
	}
	Unlock();
	return (int)ret;
}

void cairo_font::GetMetrics(cairo_t *cr, cairo_font_metrics *fm)
{
	Lock();
	cairo_set_font_face(cr, _font_face);
	cairo_set_font_size(cr, _size);
	cairo_font_extents_t ext;
	cairo_font_extents(cr, &ext);

	cairo_text_extents_t tex;
	cairo_text_extents(cr, "x", &tex);

	fm->ascent = (int)ext.ascent;
	fm->descent = (int)ext.descent;
	fm->height = (int)(ext.ascent + ext.descent);
	fm->x_height = (int)tex.height;
	Unlock();
}

void cairo_font::SetFont(HFONT hFont)
{
	Clear();
	_hFont = hFont;
	_font_face = CreateFontFace(_hFont);
	_font_code_pages = 0;
	if (_font_link) {
		HDC hdc = GetDC(NULL);
		SelectObject(hdc, _hFont);
		_font_link->GetFontCodePages(hdc, _hFont, &_font_code_pages);
		ReleaseDC(NULL, hdc);
	}
	LOGFONT lf;
	GetObject(_hFont, sizeof(LOGFONT), &lf);
	_bUnderline = lf.lfUnderline;
	_bStrikeOut = lf.lfStrikeOut;
}

void cairo_font::Clear()
{
	if (_font_face) {
		cairo_font_face_destroy(_font_face);
		_font_face = NULL;
	}
	for (size_t i = 0; i < _linked_fonts.size(); i++) {
		if (_linked_fonts[i]->hFont && _font_link)
			_font_link->ReleaseFont(_linked_fonts[i]->hFont);
		if (_linked_fonts[i]->font_face)
			cairo_font_face_destroy(_linked_fonts[i]->font_face);
	}
	_linked_fonts.clear();
	if (_hFont) {
		DeleteObject(_hFont);
		_hFont = NULL;
	}
}

void cairo_font::Init()
{
	_hFont = NULL;
	_font_face = NULL;
	_font_link = NULL;
	_font_code_pages = 0;
	_size = 0;
	_bUnderline = FALSE;
	_bStrikeOut = FALSE;
}

wchar_t *cairo_font::Utf8ToWchar(const char *src)
{
	if (!src) return NULL;
	int len = (int)strlen(src);
	wchar_t *ret = new wchar_t[len + 1];
	MultiByteToWideChar(CP_UTF8, 0, src, -1, ret, len + 1);
	return ret;
}

char *cairo_font::WcharToUtf8(const wchar_t *src)
{
	if (!src) return NULL;
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
	char *ret = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, src, -1, ret, len, NULL, NULL);
	return ret;
}
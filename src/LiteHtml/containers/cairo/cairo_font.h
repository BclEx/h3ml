#pragma once

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <mlang.h>
#include <vector>
#include <cairo.h>
#include <cairo-win32.h>
#include <litehtml.h>
using namespace litehtml;

struct linked_font
{
	typedef std::vector<linked_font *> vector;

	DWORD code_pages;
	HFONT hFont;
	cairo_font_face_t *font_face;
};

struct text_chunk
{
	typedef std::vector<text_chunk *> vector;

	char *text;
	linked_font *font;

	~text_chunk()
	{
		if (text)
			delete text;
	}
};

struct cairo_font_metrics
{
	int height;
	int ascent;
	int descent;
	int x_height;
};

class cairo_font
{
	HFONT _hFont;
	cairo_font_face_t *_font_face;
	IMLangFontLink2 *_font_link;
	DWORD _font_code_pages;
	linked_font::vector	_linked_fonts;
	int _size;
	BOOL _bUnderline;
	BOOL _bStrikeOut;
	cairo_font_metrics _metrics;
public:
	// fonts are not thread safe :(
	// you have to declare and initialize cairo_font::m_sync before the first using.
	static CRITICAL_SECTION	_sync;

	cairo_font(IMLangFontLink2 *fl, HFONT hFont, int size);
	cairo_font(IMLangFontLink2 *fl, LPCWSTR facename, int size, int weight, BOOL italic, BOOL strikeout, BOOL underline);

	void Init();
	~cairo_font();

	void ShowText(cairo_t *cr, int x, int y, const tchar_t *);
	int TextWidth(cairo_t *cr, const tchar_t *str);
	void LoadMetrics(cairo_t *cr);
	cairo_font_metrics &Metrics();
	static wchar_t *Utf8ToWchar(const char *src);
	static char *WcharToUtf8(const wchar_t *src);
private:
	void SplitText(const tchar_t *str, text_chunk::vector &chunks);
	void FreeTextChunks(text_chunk::vector &chunks);
	cairo_font_face_t *CreateFontFace(HFONT fnt);
	void SetFont(HFONT hFont);
	void Clear();
	int TextWidth(cairo_t *cr, text_chunk::vector &chunks);
	void Lock();
	void Unlock();
	int Round(double val);
	void GetMetrics(cairo_t *cr, cairo_font_metrics *fm);
};

inline void cairo_font::Lock()
{
	EnterCriticalSection(&_sync);
}

inline void cairo_font::Unlock()
{
	LeaveCriticalSection(&_sync);
}

inline int cairo_font::Round(double val)
{
	int int_val = (int)val;
	if (val - int_val >= 0.5)
		int_val++;
	return int_val;
}

inline cairo_font_metrics &cairo_font::Metrics()
{
	return _metrics;
}

inline void cairo_font::LoadMetrics(cairo_t *cr)
{
	GetMetrics(cr, &_metrics);
}

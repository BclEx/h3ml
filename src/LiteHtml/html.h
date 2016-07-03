#pragma once
#include <stdlib.h>
#include <string>
#include <ctype.h>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include <sstream>
#include "os_types.h"
#include "types.h"
#include "background.h"
#include "borders.h"
#include "html_tag.h"
#include "web_color.h"
#include "media_query.h"

namespace litehtml
{
	struct list_marker
	{
		tstring image;
		const tchar_t *baseurl;
		list_style_type marker_type;
		web_color color;
		position pos;
	};

	// call back interface to draw text, images and other elements
	class document_container
	{
	public:
		virtual uint_ptr CreateFont(const tchar_t *faceName, int size, int weight, font_style italic, unsigned int decoration, font_metrics *fm) = 0;
		virtual void DeleteFont(uint_ptr hFont) = 0;
		virtual int TextWidth(const tchar_t *text, uint_ptr hFont) = 0;
		virtual void DrawText(uint_ptr hdc, const tchar_t *text, uint_ptr hFont, web_color color, const position &pos) = 0;
		virtual int PtToPx(int pt) = 0;
		virtual int GetDefaultFontSize() const = 0;
		virtual const tchar_t *GetDefaultFontName() const = 0;
		virtual void DrawListMarker(uint_ptr hdc, const list_marker &marker) = 0;
		virtual void LoadImage(const tchar_t *src, const tchar_t *baseurl, bool redraw_on_ready) = 0;
		virtual void GetImageSize(const tchar_t *src, const tchar_t *baseurl, size &sz) = 0;
		virtual void DrawBackground(uint_ptr hdc, const background_paint &bg) = 0;
		virtual void DrawBorders(uint_ptr hdc, const borders &borders, const position &draw_pos, bool root) = 0;

		virtual	void SetCaption(const tchar_t *caption) = 0;
		virtual	void SetBaseUrl(const tchar_t *base_url) = 0;
		virtual void Link(const std::shared_ptr<document> &doc, const element::ptr &el) = 0;
		virtual void OnAnchorClick(const tchar_t *url, const element::ptr &el) = 0;
		virtual	void SetCursor(const tchar_t *cursor) = 0;
		virtual	void TransformText(tstring &text, text_transform tt) = 0;
		virtual void ImportCss(tstring &text, const tstring &url, tstring &baseurl) = 0;
		virtual void SetClip(const position &pos, const border_radiuses &bdr_radius, bool valid_x, bool valid_y) = 0;
		virtual void DelClip() = 0;
		virtual void GetClientRect(position &client) const = 0;
		virtual std::shared_ptr<element> CreateElement(const tchar_t *tag_name, const string_map &attributes, const std::shared_ptr<document> &doc) = 0;

		virtual void GetMediaFeatures(media_features &media) const = 0;
		virtual void GetLanguage(tstring &language, tstring &culture) const = 0;
	};

	void trim(tstring &s);
	void lcase(tstring &s);
	int value_index(const tstring &val, const tstring &strings, int defValue = -1, tchar_t delim = _t(';'));
	bool value_in_list(const tstring &val, const tstring &strings, tchar_t delim = _t(';'));
	tstring::size_type find_close_bracket(const tstring &s, tstring::size_type off, tchar_t open_b = _t('('), tchar_t close_b = _t(')'));
	void split_string(const tstring &str, string_vector &tokens, const tstring &delims, const tstring &delims_preserve = _t(""), const tstring &quote = _t("\""));
	void join_string(tstring &str, const string_vector &tokens, const tstring &delims);

	inline int round_f(float val)
	{
		int int_val = (int)val;
		if (val - int_val >= 0.5)
			int_val++;
		return int_val;
	}

	inline int round_d(double val)
	{
		int int_val = (int)val;
		if (val - int_val >= 0.5)
			int_val++;
		return int_val;
	}
}

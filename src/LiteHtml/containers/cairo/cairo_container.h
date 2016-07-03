#pragma once
#include <windows.h>
#include <mlang.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <mlang.h>
#include <vector>
#include <cairo.h>
#include <cairo-win32.h>
#include <litehtml.h>
#include <Dib.h>
#include <TxDib.h>
using namespace litehtml;

#ifdef LITEHTML_UTF8
#define t_make_url MakeUrlUtf8
#else
#define t_make_url MakeUrl
#endif

struct cairo_clip_box
{
	typedef std::vector<cairo_clip_box> vector;
	position box;
	border_radiuses radius;

	cairo_clip_box(const position &vBox, border_radiuses vRad)
	{
		box = vBox;
		radius = vRad;
	}

	cairo_clip_box(const cairo_clip_box &val)
	{
		box = val.box;
		radius = val.radius;
	}
	cairo_clip_box &operator=(const cairo_clip_box &val)
	{
		box = val.box;
		radius = val.radius;
		return *this;
	}
};

class cairo_container : public document_container
{
public:
	typedef std::shared_ptr<TxDib> image_ptr;
	typedef std::map<std::wstring, image_ptr> images_map;

protected:
	cairo_surface_t *_temp_surface;
	cairo_t *_temp_cr;
	images_map _images;
	cairo_clip_box::vector _clips;
	IMLangFontLink2 *_font_link;
	CRITICAL_SECTION _img_sync;
public:
	cairo_container();
	virtual ~cairo_container();

	virtual uint_ptr CreateFont(const tchar_t *faceName, int size, int weight, font_style italic, unsigned int decoration, font_metrics *fm) override;
	virtual void DeleteFont(uint_ptr hFont) override;
	virtual int TextWidth(const tchar_t *text, uint_ptr hFont) override;
	virtual void DrawText(uint_ptr hdc, const tchar_t *text, uint_ptr hFont, web_color color, const position &pos) override;

	virtual int PtToPx(int pt) override;
	virtual int GetDefaultFontSize() const override;
	virtual const tchar_t *GetDefaultFontName() const override;
	virtual void DrawListMarker(uint_ptr hdc, const list_marker &marker) override;
	virtual void LoadImage(const tchar_t *src, const tchar_t *baseurl, bool redraw_on_ready) override;
	virtual void GetImageSize(const tchar_t *src, const tchar_t *baseurl, size &sz) override;
	virtual void DrawImage(uint_ptr hdc, const tchar_t *src, const tchar_t *baseurl, const position &pos);
	virtual void DrawBackground(uint_ptr hdc, const background_paint &bg) override;
	virtual void DrawBorders(uint_ptr hdc, const borders &borders, const position &draw_pos, bool root) override;

	virtual	void TransformText(tstring &text, text_transform tt) override;
	virtual void SetClip(const position &pos, const border_radiuses &bdr_radius, bool valid_x, bool valid_y) override;
	virtual void DelClip() override;
	virtual std::shared_ptr<element> CreateElement(const tchar_t *tag_name, const string_map &attributes, const std::shared_ptr<document> &doc) override;
	virtual void GetMediaFeatures(media_features &media) const override;
	virtual void GetLanguage(tstring &language, tstring &culture) const override;
	virtual void Link(const std::shared_ptr<document> &doc, const element::ptr &el) override;

	virtual void MakeUrl(LPCWSTR url, LPCWSTR basepath, std::wstring &out) = 0;
	virtual image_ptr GetImage(LPCWSTR url, bool redraw_on_ready) = 0;
	void ClearImages();
	void AddImage(std::wstring &url, image_ptr &img);
	void RemoveImage(std::wstring &url);
	void MakeUrlUtf8(const char *url, const char *basepath, std::wstring &out);

protected:
	virtual void DrawEllipse(cairo_t *cr, int x, int y, int width, int height, const web_color &color, double line_width);
	virtual void FillEllipse(cairo_t *cr, int x, int y, int width, int height, const web_color &color);
	virtual void RoundedRectangle(cairo_t *cr, const position &pos, const border_radiuses &radius);

	void SetColor(cairo_t *cr, web_color color) { cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0); }
private:
	Dib *GetDib(uint_ptr hdc) { return (Dib *)hdc; }
	void ApplyClip(cairo_t *cr);
	bool AddPathArc(cairo_t *cr, double x, double y, double rx, double ry, double a1, double a2, bool neg);

	void DrawTxdib(cairo_t *cr, TxDib *bmp, int x, int y, int cx, int cy);
	void LockImagesCache();
	void UnlockImagesCache();
};

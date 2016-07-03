#include "cairo_container.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "cairo_font.h"

cairo_container::cairo_container()
{
	_temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2, 2);
	_temp_cr = cairo_create(_temp_surface);
	_font_link = NULL;
	CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_ALL, IID_IMLangFontLink2, (void **)&_font_link);
	InitializeCriticalSection(&_img_sync);
}

cairo_container::~cairo_container()
{
	ClearImages();
	if (_font_link)
		_font_link->Release();
	cairo_surface_destroy(_temp_surface);
	cairo_destroy(_temp_cr);
	DeleteCriticalSection(&_img_sync);
}

uint_ptr cairo_container::CreateFont(const tchar_t *faceName, int size, int weight, font_style italic, unsigned int decoration, font_metrics *fm)
{
	std::wstring fnt_name = L"sans-serif";

	string_vector fonts;
	split_string(faceName, fonts, _t(","));
	if (!fonts.empty()) {
		trim(fonts[0]);
#ifdef LITEHTML_UTF8
		wchar_t *f = cairo_font::Utf8ToWchar(fonts[0].c_str());
		fnt_name = f;
		delete f;
#else
		fnt_name = fonts[0];
		if (fnt_name.front() == '"')
			fnt_name.erase(0, 1);
		if (fnt_name.back() == '"')
			fnt_name.erase(fnt_name.length() - 1, 1);
#endif
	}

	cairo_font *fnt = new cairo_font(_font_link,
		fnt_name.c_str(),
		size,
		weight,
		(italic == fontStyleItalic) ? TRUE : FALSE,
		(decoration & font_decoration_linethrough) ? TRUE : FALSE,
		(decoration & font_decoration_underline) ? TRUE : FALSE);

	cairo_save(_temp_cr);
	fnt->LoadMetrics(_temp_cr);

	if (fm)
	{
		fm->ascent = fnt->Metrics().ascent;
		fm->descent = fnt->Metrics().descent;
		fm->height = fnt->Metrics().height;
		fm->x_height = fnt->Metrics().x_height;
		if (italic == fontStyleItalic || decoration)
			fm->draw_spaces = true;
		else
			fm->draw_spaces = false;
	}

	cairo_restore(_temp_cr);

	return (uint_ptr)fnt;
}

void cairo_container::DeleteFont(uint_ptr hFont)
{
	cairo_font *fnt = (cairo_font *)hFont;
	if (fnt)
		delete fnt;
}

int cairo_container::TextWidth(const tchar_t *text, uint_ptr hFont)
{
	cairo_font *fnt = (cairo_font *)hFont;

	cairo_save(_temp_cr);
	int ret = fnt->TextWidth(_temp_cr, text);
	cairo_restore(_temp_cr);
	return ret;
}

void cairo_container::DrawText(uint_ptr hdc, const tchar_t *text, uint_ptr hFont, web_color color, const position &pos)
{
	if (hFont) {
		cairo_font *fnt = (cairo_font *)hFont;
		cairo_t *cr = (cairo_t *)hdc;
		cairo_save(cr);

		ApplyClip(cr);

		int x = pos.left();
		int y = pos.bottom() - fnt->Metrics().descent;

		SetColor(cr, color);
		fnt->ShowText(cr, x, y, text);

		cairo_restore(cr);
	}
}

int cairo_container::PtToPx(int pt)
{
	HDC dc = GetDC(NULL);
	int ret = MulDiv(pt, GetDeviceCaps(dc, LOGPIXELSY), 72);
	ReleaseDC(NULL, dc);
	return ret;
}

int cairo_container::GetDefaultFontSize() const
{
	return 16;
}

void cairo_container::DrawListMarker(uint_ptr hdc, const list_marker &marker)
{
	if (!marker.image.empty()) {
		std::wstring url;
		t_make_url(marker.image.c_str(), marker.baseurl, url);

		LockImagesCache();
		images_map::iterator img_i = _images.find(url.c_str());
		if (img_i != _images.end())
			if (img_i->second)
				DrawTxdib((cairo_t *)hdc, img_i->second.get(), marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height);
		UnlockImagesCache();
	}
	else {
		switch (marker.marker_type) {
		case list_style_type_circle: {
			DrawEllipse((cairo_t*)hdc, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, marker.color, 0.5);
			break;
		}
		case list_style_type_disc: {
			FillEllipse((cairo_t*)hdc, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, marker.color);
			break;
		}
		case list_style_type_square:
			if (hdc) {
				cairo_t *cr = (cairo_t *)hdc;
				cairo_save(cr);

				cairo_new_path(cr);
				cairo_rectangle(cr, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height);

				SetColor(cr, marker.color);
				cairo_fill(cr);
				cairo_restore(cr);
			}
			break;
		}
	}
}

void cairo_container::LoadImage(const tchar_t *src, const tchar_t *baseurl, bool redraw_on_ready)
{
	std::wstring url;
	t_make_url(src, baseurl, url);
	LockImagesCache();
	if (_images.find(url.c_str()) == _images.end()) {
		UnlockImagesCache();
		image_ptr img = GetImage(url.c_str(), redraw_on_ready);
		LockImagesCache();
		_images[url] = img;
		UnlockImagesCache();
	}
	else
		UnlockImagesCache();
}

void cairo_container::GetImageSize(const tchar_t *src, const tchar_t *baseurl, size &sz)
{
	std::wstring url;
	t_make_url(src, baseurl, url);

	sz.width = 0;
	sz.height = 0;

	LockImagesCache();
	images_map::iterator img = _images.find(url.c_str());
	if (img != _images.end())
		if (img->second) {
			sz.width = img->second->GetWidth();
			sz.height = img->second->GetHeight();
		}
	UnlockImagesCache();
}

void cairo_container::DrawImage(uint_ptr hdc, const tchar_t *src, const tchar_t *baseurl, const position &pos)
{
	cairo_t *cr = (cairo_t *)hdc;
	cairo_save(cr);
	ApplyClip(cr);

	std::wstring url;
	t_make_url(src, baseurl, url);
	LockImagesCache();
	images_map::iterator img = _images.find(url.c_str());
	if (img != _images.end())
		if (img->second)
			DrawTxdib(cr, img->second.get(), pos.x, pos.y, pos.width, pos.height);
	UnlockImagesCache();
	cairo_restore(cr);
}

void cairo_container::DrawBackground(uint_ptr hdc, const background_paint &bg)
{
	cairo_t *cr = (cairo_t *)hdc;
	cairo_save(cr);
	ApplyClip(cr);

	RoundedRectangle(cr, bg.border_box, bg.border_radius);
	cairo_clip(cr);

	cairo_rectangle(cr, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height);
	cairo_clip(cr);

	if (bg.color.alpha) {
		SetColor(cr, bg.color);
		cairo_paint(cr);
	}

	std::wstring url;
	t_make_url(bg.image.c_str(), bg.baseurl.c_str(), url);

	LockImagesCache();
	images_map::iterator img_i = _images.find(url.c_str());
	if (img_i != _images.end() && img_i->second)
	{
		image_ptr bgbmp = img_i->second;

		image_ptr new_img;
		if (bg.image_size.width != bgbmp->GetWidth() || bg.image_size.height != bgbmp->GetHeight()) {
			new_img = image_ptr(new TxDib);
			bgbmp->Resample(bg.image_size.width, bg.image_size.height, new_img.get());
			bgbmp = new_img;
		}

		cairo_surface_t *img = cairo_image_surface_create_for_data((unsigned char*)bgbmp->GetBits(), CAIRO_FORMAT_ARGB32, bgbmp->GetWidth(), bgbmp->GetHeight(), bgbmp->GetWidth() * 4);
		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(img);
		cairo_matrix_t flib_m;
		cairo_matrix_init(&flib_m, 1, 0, 0, -1, 0, 0);
		cairo_matrix_translate(&flib_m, -bg.position_x, -bg.position_y);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
		cairo_pattern_set_matrix(pattern, &flib_m);

		switch (bg.repeat) {
		case background_repeat_no_repeat:
			DrawTxdib(cr, bgbmp.get(), bg.position_x, bg.position_y, bgbmp->GetWidth(), bgbmp->GetHeight());
			break;

		case background_repeat_repeat_x:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg.clip_box.left(), bg.position_y, bg.clip_box.width, bgbmp->GetHeight());
			cairo_fill(cr);
			break;

		case background_repeat_repeat_y:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg.position_x, bg.clip_box.top(), bgbmp->GetWidth(), bg.clip_box.height);
			cairo_fill(cr);
			break;

		case background_repeat_repeat:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg.clip_box.left(), bg.clip_box.top(), bg.clip_box.width, bg.clip_box.height);
			cairo_fill(cr);
			break;
		}

		cairo_pattern_destroy(pattern);
		cairo_surface_destroy(img);
	}
	UnlockImagesCache();
	cairo_restore(cr);
}

bool cairo_container::AddPathArc(cairo_t *cr, double x, double y, double rx, double ry, double a1, double a2, bool neg)
{
	if (rx > 0 && ry > 0) {
		cairo_save(cr);

		cairo_translate(cr, x, y);
		cairo_scale(cr, 1, ry / rx);
		cairo_translate(cr, -x, -y);

		if (neg)
			cairo_arc_negative(cr, x, y, rx, a1, a2);
		else
			cairo_arc(cr, x, y, rx, a1, a2);

		cairo_restore(cr);
		return true;
	}
	return false;
}

void cairo_container::DrawBorders(uint_ptr hdc, const borders &borders, const position &draw_pos, bool root)
{
	cairo_t *cr = (cairo_t *)hdc;
	cairo_save(cr);
	ApplyClip(cr);

	cairo_new_path(cr);

	int bdr_top = 0;
	int bdr_bottom = 0;
	int bdr_left = 0;
	int bdr_right = 0;

	if (borders.top.width != 0 && borders.top.style > border_style_hidden)
		bdr_top = (int)borders.top.width;
	if (borders.bottom.width != 0 && borders.bottom.style > border_style_hidden)
		bdr_bottom = (int)borders.bottom.width;
	if (borders.left.width != 0 && borders.left.style > border_style_hidden)
		bdr_left = (int)borders.left.width;
	if (borders.right.width != 0 && borders.right.style > border_style_hidden)
		bdr_right = (int)borders.right.width;

	// draw right border
	if (bdr_right) {
		SetColor(cr, borders.right.color);

		double r_top = (double)borders.radius.top_right_x;
		double r_bottom = (double)borders.radius.bottom_right_x;

		if (r_top) {
			double end_angle = 2.0 * M_PI;
			double start_angle = end_angle - M_PI / 2.0 / ((double)bdr_top / (double)bdr_right + 0.5);
			if (!AddPathArc(cr,
				draw_pos.right() - r_top,
				draw_pos.top() + r_top,
				r_top - bdr_right,
				r_top - bdr_right + (bdr_right - bdr_top),
				end_angle,
				start_angle, true))
				cairo_move_to(cr, draw_pos.right() - bdr_right, draw_pos.top() + bdr_top);
			if (!AddPathArc(cr,
				draw_pos.right() - r_top,
				draw_pos.top() + r_top,
				r_top,
				r_top,
				start_angle,
				end_angle, false))
				cairo_line_to(cr, draw_pos.right(), draw_pos.top());
		}
		else {
			cairo_move_to(cr, draw_pos.right() - bdr_right, draw_pos.top() + bdr_top);
			cairo_line_to(cr, draw_pos.right(), draw_pos.top());
		}

		if (r_bottom) {
			cairo_line_to(cr, draw_pos.right(), draw_pos.bottom() - r_bottom);

			double start_angle = 0;
			double end_angle = start_angle + M_PI / 2.0 / ((double)bdr_bottom / (double)bdr_right + 0.5);
			if (!AddPathArc(cr,
				draw_pos.right() - r_bottom,
				draw_pos.bottom() - r_bottom,
				r_bottom,
				r_bottom,
				start_angle,
				end_angle, false))
				cairo_line_to(cr, draw_pos.right(), draw_pos.bottom());
			if (!AddPathArc(cr,
				draw_pos.right() - r_bottom,
				draw_pos.bottom() - r_bottom,
				r_bottom - bdr_right,
				r_bottom - bdr_right + (bdr_right - bdr_bottom),
				end_angle,
				start_angle, true))
				cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.bottom() - bdr_bottom);
		}
		else {
			cairo_line_to(cr, draw_pos.right(), draw_pos.bottom());
			cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.bottom() - bdr_bottom);
		}

		cairo_fill(cr);
	}

	// draw bottom border
	if (bdr_bottom) {
		SetColor(cr, borders.bottom.color);

		double r_left = borders.radius.bottom_left_x;
		double r_right = borders.radius.bottom_right_x;

		if (r_left) {
			double start_angle = M_PI / 2.0;
			double end_angle = start_angle + M_PI / 2.0 / ((double)bdr_left / (double)bdr_bottom + 0.5);
			if (!AddPathArc(cr,
				draw_pos.left() + r_left,
				draw_pos.bottom() - r_left,
				r_left - bdr_bottom + (bdr_bottom - bdr_left),
				r_left - bdr_bottom,
				start_angle,
				end_angle, false))
				cairo_move_to(cr, draw_pos.left() + bdr_left, draw_pos.bottom() - bdr_bottom);
			if (!AddPathArc(cr,
				draw_pos.left() + r_left,
				draw_pos.bottom() - r_left,
				r_left,
				r_left,
				end_angle,
				start_angle, true))
				cairo_line_to(cr, draw_pos.left(), draw_pos.bottom());
		}
		else {
			cairo_move_to(cr, draw_pos.left(), draw_pos.bottom());
			cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.bottom() - bdr_bottom);
		}

		if (r_right) {
			cairo_line_to(cr, draw_pos.right() - r_right, draw_pos.bottom());

			double end_angle = M_PI / 2.0;
			double start_angle = end_angle - M_PI / 2.0 / ((double)bdr_right / (double)bdr_bottom + 0.5);
			if (!AddPathArc(cr,
				draw_pos.right() - r_right,
				draw_pos.bottom() - r_right,
				r_right,
				r_right,
				end_angle,
				start_angle, true))
				cairo_line_to(cr, draw_pos.right(), draw_pos.bottom());
			if (!AddPathArc(cr,
				draw_pos.right() - r_right,
				draw_pos.bottom() - r_right,
				r_right - bdr_bottom + (bdr_bottom - bdr_right),
				r_right - bdr_bottom,
				start_angle,
				end_angle, false))
				cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.bottom() - bdr_bottom);
		}
		else {
			cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.bottom() - bdr_bottom);
			cairo_line_to(cr, draw_pos.right(), draw_pos.bottom());
		}

		cairo_fill(cr);
	}

	// draw top border
	if (bdr_top) {
		SetColor(cr, borders.top.color);

		double r_left = borders.radius.top_left_x;
		double r_right = borders.radius.top_right_x;

		if (r_left) {
			double end_angle = M_PI * 3.0 / 2.0;
			double start_angle = end_angle - M_PI / 2.0 / ((double)bdr_left / (double)bdr_top + 0.5);
			if (!AddPathArc(cr,
				draw_pos.left() + r_left,
				draw_pos.top() + r_left,
				r_left,
				r_left,
				end_angle,
				start_angle, true))
				cairo_move_to(cr, draw_pos.left(), draw_pos.top());
			if (!AddPathArc(cr,
				draw_pos.left() + r_left,
				draw_pos.top() + r_left,
				r_left - bdr_top + (bdr_top - bdr_left),
				r_left - bdr_top,
				start_angle,
				end_angle, false))
				cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.top() + bdr_top);
		}
		else {
			cairo_move_to(cr, draw_pos.left(), draw_pos.top());
			cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.top() + bdr_top);
		}

		if (r_right) {
			cairo_line_to(cr, draw_pos.right() - r_right, draw_pos.top() + bdr_top);

			double start_angle = M_PI * 3.0 / 2.0;
			double end_angle = start_angle + M_PI / 2.0 / ((double)bdr_right / (double)bdr_top + 0.5);
			if (!AddPathArc(cr,
				draw_pos.right() - r_right,
				draw_pos.top() + r_right,
				r_right - bdr_top + (bdr_top - bdr_right),
				r_right - bdr_top,
				start_angle,
				end_angle, false))
				cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.top() + bdr_top);
			if (!AddPathArc(cr,
				draw_pos.right() - r_right,
				draw_pos.top() + r_right,
				r_right,
				r_right,
				end_angle,
				start_angle, true))
				cairo_line_to(cr, draw_pos.right(), draw_pos.top());
		}
		else {
			cairo_line_to(cr, draw_pos.right() - bdr_right, draw_pos.top() + bdr_top);
			cairo_line_to(cr, draw_pos.right(), draw_pos.top());
		}

		cairo_fill(cr);
	}

	// draw left border
	if (bdr_left)
	{
		SetColor(cr, borders.left.color);

		double r_top = borders.radius.top_left_x;
		double r_bottom = borders.radius.bottom_left_x;

		if (r_top) {
			double start_angle = M_PI;
			double end_angle = start_angle + M_PI / 2.0 / ((double)bdr_top / (double)bdr_left + 0.5);
			if (!AddPathArc(cr,
				draw_pos.left() + r_top,
				draw_pos.top() + r_top,
				r_top - bdr_left,
				r_top - bdr_left + (bdr_left - bdr_top),
				start_angle,
				end_angle, false))
				cairo_move_to(cr, draw_pos.left() + bdr_left, draw_pos.top() + bdr_top);
			if (!AddPathArc(cr,
				draw_pos.left() + r_top,
				draw_pos.top() + r_top,
				r_top,
				r_top,
				end_angle,
				start_angle, true))
				cairo_line_to(cr, draw_pos.left(), draw_pos.top());
		}
		else {
			cairo_move_to(cr, draw_pos.left() + bdr_left, draw_pos.top() + bdr_top);
			cairo_line_to(cr, draw_pos.left(), draw_pos.top());
		}

		if (r_bottom) {
			cairo_line_to(cr, draw_pos.left(), draw_pos.bottom() - r_bottom);

			double end_angle = M_PI;
			double start_angle = end_angle - M_PI / 2.0 / ((double)bdr_bottom / (double)bdr_left + 0.5);
			if (!AddPathArc(cr,
				draw_pos.left() + r_bottom,
				draw_pos.bottom() - r_bottom,
				r_bottom,
				r_bottom,
				end_angle,
				start_angle, true))
				cairo_line_to(cr, draw_pos.left(), draw_pos.bottom());
			if (!AddPathArc(cr,
				draw_pos.left() + r_bottom,
				draw_pos.bottom() - r_bottom,
				r_bottom - bdr_left,
				r_bottom - bdr_left + (bdr_left - bdr_bottom),
				start_angle,
				end_angle, false))
				cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.bottom() - bdr_bottom);
		}
		else {
			cairo_line_to(cr, draw_pos.left(), draw_pos.bottom());
			cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.bottom() - bdr_bottom);
		}

		cairo_fill(cr);
	}
	cairo_restore(cr);
}

void cairo_container::SetClip(const position& pos, const border_radiuses& bdr_radius, bool valid_x, bool valid_y)
{
	position clip_pos = pos;
	position client_pos;
	GetClientRect(client_pos);
	if (!valid_x) {
		clip_pos.x = client_pos.x;
		clip_pos.width = client_pos.width;
	}
	if (!valid_y) {
		clip_pos.y = client_pos.y;
		clip_pos.height = client_pos.height;
	}
	_clips.emplace_back(clip_pos, bdr_radius);
}

void cairo_container::DelClip()
{
	if (!_clips.empty())
		_clips.pop_back();
}

void cairo_container::ApplyClip(cairo_t *cr)
{
	for (const auto &clip_box : _clips) {
		RoundedRectangle(cr, clip_box.box, clip_box.radius);
		cairo_clip(cr);
	}
}

void cairo_container::DrawEllipse(cairo_t *cr, int x, int y, int width, int height, const web_color &color, double line_width)
{
	if (!cr) return;
	cairo_save(cr);

	ApplyClip(cr);

	cairo_new_path(cr);

	cairo_translate(cr, x + width / 2.0, y + height / 2.0);
	cairo_scale(cr, width / 2.0, height / 2.0);
	cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);

	SetColor(cr, color);
	cairo_set_line_width(cr, line_width);
	cairo_stroke(cr);

	cairo_restore(cr);
}

void cairo_container::FillEllipse(cairo_t* cr, int x, int y, int width, int height, const web_color& color)
{
	if (!cr) return;
	cairo_save(cr);

	ApplyClip(cr);

	cairo_new_path(cr);

	cairo_translate(cr, x + width / 2.0, y + height / 2.0);
	cairo_scale(cr, width / 2.0, height / 2.0);
	cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);

	SetColor(cr, color);
	cairo_fill(cr);

	cairo_restore(cr);
}

void cairo_container::ClearImages()
{
	LockImagesCache();
	_images.clear();
	UnlockImagesCache();
}

const tchar_t *cairo_container::GetDefaultFontName() const
{
	return _t("Times New Roman");
}

void cairo_container::DrawTxdib(cairo_t *cr, TxDib *bmp, int x, int y, int cx, int cy)
{
	cairo_save(cr);

	cairo_matrix_t flib_m;
	cairo_matrix_init(&flib_m, 1, 0, 0, -1, 0, 0);

	cairo_surface_t *img = NULL;

	TxDib rbmp;

	if (cx != bmp->GetWidth() || cy != bmp->GetHeight()) {
		bmp->Resample(cx, cy, &rbmp);
		img = cairo_image_surface_create_for_data((unsigned char *)rbmp.GetBits(), CAIRO_FORMAT_ARGB32, rbmp.GetWidth(), rbmp.GetHeight(), rbmp.GetWidth() * 4);
		cairo_matrix_translate(&flib_m, 0, -rbmp.GetHeight());
		cairo_matrix_translate(&flib_m, x, -y);
	}
	else
	{
		img = cairo_image_surface_create_for_data((unsigned char *)bmp->GetBits(), CAIRO_FORMAT_ARGB32, bmp->GetWidth(), bmp->GetHeight(), bmp->GetWidth() * 4);
		cairo_matrix_translate(&flib_m, 0, -bmp->GetHeight());
		cairo_matrix_translate(&flib_m, x, -y);
	}

	cairo_transform(cr, &flib_m);
	cairo_set_source_surface(cr, img, 0, 0);
	cairo_paint(cr);

	cairo_restore(cr);
	cairo_surface_destroy(img);
}

void cairo_container::RoundedRectangle(cairo_t *cr, const position &pos, const border_radiuses &radius)
{
	cairo_new_path(cr);
	if (radius.top_left_x)
		cairo_arc(cr, pos.left() + radius.top_left_x, pos.top() + radius.top_left_x, radius.top_left_x, M_PI, M_PI * 3.0 / 2.0);
	else
		cairo_move_to(cr, pos.left(), pos.top());

	cairo_line_to(cr, pos.right() - radius.top_right_x, pos.top());

	if (radius.top_right_x)
		cairo_arc(cr, pos.right() - radius.top_right_x, pos.top() + radius.top_right_x, radius.top_right_x, M_PI * 3.0 / 2.0, 2.0 * M_PI);

	cairo_line_to(cr, pos.right(), pos.bottom() - radius.bottom_right_x);

	if (radius.bottom_right_x)
		cairo_arc(cr, pos.right() - radius.bottom_right_x, pos.bottom() - radius.bottom_right_x, radius.bottom_right_x, 0, M_PI / 2.0);

	cairo_line_to(cr, pos.left() - radius.bottom_left_x, pos.bottom());

	if (radius.bottom_left_x)
		cairo_arc(cr, pos.left() + radius.bottom_left_x, pos.bottom() - radius.bottom_left_x, radius.bottom_left_x, M_PI / 2.0, M_PI);
}

void cairo_container::RemoveImage(std::wstring &url)
{
	LockImagesCache();
	images_map::iterator i = _images.find(url);
	if (i != _images.end())
		_images.erase(i);
	UnlockImagesCache();
}

void cairo_container::AddImage(std::wstring &url, image_ptr &img)
{
	LockImagesCache();
	images_map::iterator i = _images.find(url);
	if (i != _images.end()) {
		if (img)
			i->second = img;
		else
			_images.erase(i);
	}
	UnlockImagesCache();
}

void cairo_container::LockImagesCache()
{
	EnterCriticalSection(&_img_sync);
}

void cairo_container::UnlockImagesCache()
{
	LeaveCriticalSection(&_img_sync);
}

std::shared_ptr<element> cairo_container::CreateElement(const tchar_t *tag_name, const string_map &attributes, const std::shared_ptr<document> &doc)
{
	return 0;
}

void cairo_container::GetMediaFeatures(media_features &media) const
{
	position client;
	GetClientRect(client);
	HDC hdc = GetDC(NULL);

	media.type = media_type_screen;
	media.width = client.width;
	media.height = client.height;
	media.color = 8;
	media.monochrome = 0;
	media.color_index = 256;
	media.resolution = GetDeviceCaps(hdc, LOGPIXELSX);
	media.device_width = GetDeviceCaps(hdc, HORZRES);
	media.device_height = GetDeviceCaps(hdc, VERTRES);

	ReleaseDC(NULL, hdc);
}

void cairo_container::GetLanguage(tstring &language, tstring &culture) const
{
	language = _t("en");
	culture = _t("");
}

void cairo_container::MakeUrlUtf8(const char *url, const char *basepath, std::wstring &out)
{
	wchar_t *urlW = cairo_font::Utf8ToWchar(url);
	wchar_t *basepathW = cairo_font::Utf8ToWchar(basepath);
	MakeUrl(urlW, basepathW, out);

	if (urlW) delete urlW;
	if (basepathW) delete basepathW;
}

void cairo_container::TransformText(tstring &text, text_transform tt)
{
	if (text.empty()) return;

#ifndef LITEHTML_UTF8
	switch (tt) {
	case text_transform_capitalize:
		if (!text.empty())
			text[0] = (WCHAR)CharUpper((LPWSTR)text[0]);
		break;
	case text_transform_uppercase:
		for (size_t i = 0; i < text.length(); i++)
			text[i] = (WCHAR)CharUpper((LPWSTR)text[i]);
		break;
	case text_transform_lowercase:
		for (size_t i = 0; i < text.length(); i++)
			text[i] = (WCHAR)CharLower((LPWSTR)text[i]);
		break;
	}
#else
	LPWSTR txt = cairo_font::Utf8ToWchar(text.c_str());
	switch (tt) {
	case text_transform_capitalize:
		CharUpperBuff(txt, 1);
		break;
	case text_transform_uppercase:
		CharUpperBuff(txt, lstrlen(txt));
		break;
	case text_transform_lowercase:
		CharLowerBuff(txt, lstrlen(txt));
		break;
	}
	LPSTR txtA = cairo_font::WcharToUtf8(txt);
	text = txtA;
	delete txtA;
	delete txt;
#endif
}

void cairo_container::Link(const std::shared_ptr<document> &doc, const element::ptr &el)
{
}

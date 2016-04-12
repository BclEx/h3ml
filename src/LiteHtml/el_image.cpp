#include "html.h"
#include "el_image.h"
#include "document.h"
using namespace litehtml;

el_image::el_image(const std::shared_ptr<document> &doc) : html_tag(doc)
{
	_display = display_inline_block;
}
el_image::~el_image() { }

void el_image::get_content_size(size &sz, int max_width)
{
	get_document()->container()->get_image_size(_src.c_str(), 0, sz);
}

int el_image::line_height() const
{
	return height();
}

bool el_image::is_replaced() const
{
	return true;
}

int el_image::render(int x, int y, int max_width, bool second_pass)
{
	int parent_width = max_width;
	calc_outlines(parent_width);
	_pos.move_to(x, y);
	document::ptr doc = get_document();
	size sz;
	doc->container()->get_image_size(_src.c_str(), 0, sz);
	_pos.width = sz.width;
	_pos.height	= sz.height;

	if (_css_height.is_predefined() && _css_width.is_predefined())
	{
		_pos.height	= sz.height;
		_pos.width = sz.width;
		// check for max-height
		if (!_css_max_width.is_predefined())
		{
			int max_width = doc->cvt_units(_css_max_width, _font_size, parent_width);
			if (_pos.width > max_width)
				_pos.width = max_width;
			_pos.height = (sz.width ? (int)((float)_pos.width * (float)sz.height / (float)sz.width) : sz.height);
		}
		// check for max-height
		if (!_css_max_height.is_predefined())
		{
			int max_height = doc->cvt_units(_css_max_height, _font_size);
			if (_pos.height > max_height)
				_pos.height = max_height;
			_pos.width = (sz.height ? (int)(_pos.height * (float)sz.width / (float)sz.height) : sz.width);
		}
	}
	else if (!_css_height.is_predefined() && _css_width.is_predefined()) {
		if (!get_predefined_height(_pos.height))
			_pos.height = (int)_css_height.val();
		// check for max-height
		if (!_css_max_height.is_predefined()) {
			int max_height = doc->cvt_units(_css_max_height, _font_size);
			if (_pos.height > max_height)
				_pos.height = max_height;
		}
		_pos.width = (sz.height ? (int)(_pos.height * (float)sz.width / (float)sz.height) : sz.width);
	} else if(_css_height.is_predefined() && !_css_width.is_predefined())
	{
		_pos.width = (int)_css_width.calc_percent(parent_width);

		// check for max-width
		if (!_css_max_width.is_predefined()) {
			int max_width = doc->cvt_units(_css_max_width, _font_size, parent_width);
			if(_pos.width > max_width)
				_pos.width = max_width;
		}
		_pos.height = (sz.width ? (int)((float)_pos.width * (float)sz.height / (float)sz.width) : sz.height);
	}
	else {
		_pos.width = (int)_css_width.calc_percent(parent_width);
		_pos.height	= 0;
		if (!get_predefined_height(_pos.height))
			_pos.height = (int)_css_height.val();
		// check for max-height
		if (!_css_max_height.is_predefined())
		{
			int max_height = doc->cvt_units(_css_max_height, _font_size);
			if (_pos.height > max_height)
				_pos.height = max_height;
		}
		// check for max-height
		if (!_css_max_width.is_predefined())
		{
			int max_width = doc->cvt_units(_css_max_width, _font_size, parent_width);
			if (_pos.width > max_width)
				_pos.width = max_width;
		}
	}
	calc_auto_margins(parent_width);
	_pos.x	+= content_margins_left();
	_pos.y += content_margins_top();
	return _pos.width + content_margins_left() + content_margins_right();
}

void el_image::parse_attributes()
{
	_src = get_attr(_t("src"), _t(""));
	const tchar_t *attr_height = get_attr(_t("height"));
	if (attr_height)
		_style.add_property(_t("height"), attr_height, 0, false);
	const tchar_t *attr_width = get_attr(_t("width"));
	if (attr_width)
		_style.add_property(_t("width"), attr_width, 0, false);
}

void el_image::draw(uint_ptr hdc, int x, int y, const position *clip)
{
	position pos = _pos;
	pos.x += x;
	pos.y += y;
	position el_pos = pos;
	el_pos += _padding;
	el_pos += _borders;

	// draw standard background here
	if (el_pos.does_intersect(clip)) {
		const background *bg = get_background();
		if (bg) {
			background_paint bg_paint;
			init_background_paint(pos, bg_paint, bg);
			get_document()->container()->draw_background(hdc, bg_paint);
		}
	}

	// draw image as background
	if (pos.does_intersect(clip)) {
		background_paint bg;
		bg.image = _src;
		bg.clip_box = pos;
		bg.origin_box = pos;
		bg.border_box = pos;
		bg.border_box += _padding;
		bg.border_box += _borders;
		bg.repeat = background_repeat_no_repeat;
		bg.image_size.width = pos.width;
		bg.image_size.height = pos.height;
		bg.border_radius = _css_borders.radius.calc_percents(bg.border_box.width, bg.border_box.height);
		bg.position_x = pos.x;
		bg.position_y = pos.y;
		get_document()->container()->draw_background(hdc, bg);
	}

	// draw borders
	if (el_pos.does_intersect(clip)) {
		position border_box = pos;
		border_box += _padding;
		border_box += _borders;
		borders bdr = _css_borders;
		bdr.radius = _css_borders.radius.calc_percents(border_box.width, border_box.height);
		get_document()->container()->draw_borders(hdc, bdr, border_box, have_parent() ? false : true);
	}
}

void el_image::parse_styles(bool is_reparse /*= false*/)
{
	html_tag::parse_styles(is_reparse);
	if (!_src.empty()) {
		if (!_css_height.is_predefined() && !_css_width.is_predefined())
			get_document()->container()->load_image(_src.c_str(), 0, true);
		else
			get_document()->container()->load_image(_src.c_str(), 0, false);
	}
}

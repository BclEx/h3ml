#include "html.h"
#include "el_text.h"
#include "document.h"
using namespace litehtml;

el_text::el_text(const tchar_t *text, const std::shared_ptr<document> &doc) : element(doc)
{
	if (text)
		_text = text;
	_text_transform	= text_transform_none;
	_use_transformed = false;
	_draw_spaces = true;
}
el_text::~el_text() { }

void el_text::get_content_size(size &sz, int max_width)
{
	sz = _size;
}

void el_text::get_text(tstring &text)
{
	text += _text;
}

const tchar_t *el_text::get_style_property(const tchar_t *name, bool inherited, const tchar_t *def /*= nullptr*/)
{
	if (inherited) {
		element::ptr el_parent = parent();
		if (el_parent)
			return el_parent->get_style_property(name, inherited, def);
	}
	return def;
}

void el_text::parse_styles(bool is_reparse)
{
	_text_transform	= (text_transform)value_index(get_style_property(_t("text-transform"), true, _t("none")), text_transfor_strings, text_transform_none);
	if (_text_transform != text_transform_none) {
		_transformed_text = _text;
		_use_transformed = true;
		get_document()->container()->TransformText(_transformed_text, _text_transform);
	}

	if (is_white_space()) {
		_transformed_text = _t(" ");
		_use_transformed = true;
	}
	else {
		if (_text == _t("\t")) {
			_transformed_text = _t("    ");
			_use_transformed = true;
		}
		if (_text == _t("\n") || _text == _t("\r")) {
			_transformed_text = _t("");
			_use_transformed = true;
		}
	}

	font_metrics fm;
	uint_ptr font = 0;
	element::ptr el_parent = parent();
	if (el_parent)
		font = el_parent->get_font(&fm);
	if (is_break()) {
		_size.height = 0;
		_size.width	= 0;
	}
	else {
		_size.height = fm.height;
		_size.width	= get_document()->container()->TextWidth(_use_transformed ? _transformed_text.c_str() : _text.c_str(), font);
	}
	_draw_spaces = fm.draw_spaces;
}

int el_text::get_base_line()
{
	element::ptr el_parent = parent();
	return (el_parent ? el_parent->get_base_line() : 0);
}

void el_text::draw(uint_ptr hdc, int x, int y, const position *clip)
{
	if (is_white_space() && !_draw_spaces)
		return;
	position pos = _pos;
	pos.x += x;
	pos.y += y;
	if (pos.does_intersect(clip)) {
		element::ptr el_parent = parent();
		if (el_parent) {
			document::ptr doc = get_document();
			uint_ptr font = el_parent->get_font();
			web_color color = el_parent->get_color(_t("color"), true, doc->get_def_color());
			doc->container()->DrawText(hdc, _use_transformed ? _transformed_text.c_str() : _text.c_str(), font, color, pos);
		}
	}
}

int el_text::line_height() const
{
	element::ptr el_parent = parent();
	return (el_parent ? el_parent->line_height() : 0);
}

uint_ptr el_text::get_font(font_metrics *fm /*= nullptr*/)
{
	element::ptr el_parent = parent();
	return (el_parent ? el_parent->get_font(fm) : 0);
}

style_display el_text::get_display() const
{
	return display_inline_text;
}

white_space el_text::get_white_space() const
{
	element::ptr el_parent = parent();
	return (el_parent ? el_parent->get_white_space() : white_space_normal);
}

element_position el_text::get_element_position(css_offsets* offsets) const
{
	element::ptr p = parent();
	while (p && p->get_display() == display_inline) {
		if (p->get_element_position() == element_position_relative) {
			if (offsets)
				*offsets = p->get_css_offsets();
			return element_position_relative;
		}
		p = p->parent();
	}
	return element_position_static;
}

css_offsets el_text::get_css_offsets() const
{
	element::ptr p = parent();
	while (p && p->get_display() == display_inline) {
		if (p->get_element_position() == element_position_relative)
			return p->get_css_offsets();
		p = p->parent();
	}
	return css_offsets();
}

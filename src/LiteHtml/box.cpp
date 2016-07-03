#include "html.h"
#include "box.h"
#include "html_tag.h"
using namespace litehtml;

box_type block_box::get_type()
{
	return box_block;
}

int block_box::height()
{
	return _element->height();
}

int block_box::width()
{
	return _element->width();
}

void block_box::add_element(const element::ptr &el)
{
	_element = el;
	el->_box = this;
}

void block_box::finish(bool last_box)
{
	if (!_element) return;
	_element->apply_relative_shift(_box_right - _box_left);
}

bool block_box::can_hold(const element::ptr &el, white_space ws)
{
	return !(_element || el->is_inline_box());
}

bool block_box::is_empty()
{
	return !(_element);
}

int block_box::baseline()
{
	return (_element ? _element->get_base_line() : 0);
}

void block_box::get_elements(elements_vector &els)
{
	els.push_back(_element);
}

int block_box::top_margin()
{
	return (_element && _element->collapse_top_margin() ? _element->_margins.top : 0);
}

int block_box::bottom_margin()
{
	return (_element && _element->collapse_bottom_margin() ? _element->_margins.bottom : 0);
}

void block_box::y_shift(int shift)
{
	_box_top += shift;
	if (_element)
		_element->_pos.y += shift;
}

void block_box::new_width(int left, int right, elements_vector &els)
{
}

//////////////////////////////////////////////////////////////////////////

box_type line_box::get_type()
{
	return box_line;
}

int line_box::height()
{
	return _height;
}

int line_box::width()
{
	return _width;
}

void line_box::add_element(const element::ptr &el)
{
	el->_skip = false;
	el->_box = 0;
	bool add = true;
	if ((_items.empty() && el->is_white_space()) || el->is_break())
		el->_skip = true;
	else if (el->is_white_space()) {
		if (have_last_space()) {
			add = false;
			el->_skip = true;
		}
	}

	if (add)
	{
		el->_box = this;
		_items.push_back(el);
		if (!el->_skip)
		{
			int el_shift_left = el->get_inline_shift_left();
			int el_shift_right = el->get_inline_shift_right();
			el->_pos.x = _box_left + _width + el_shift_left + el->content_margins_left();
			el->_pos.y = _box_top + el->content_margins_top();
			_width += el->width() + el_shift_left + el_shift_right;
		}
	}
}

void line_box::finish(bool last_box)
{
	if (is_empty() || (!is_empty() && last_box && is_break_only())) {
		_height = 0;
		return;
	}

	for (auto i = _items.rbegin(); i != _items.rend(); i++) {
		if ((*i)->is_white_space() || (*i)->is_break()) {
			if (!(*i)->_skip) {
				(*i)->_skip = true;
				_width -= (*i)->width();
			}
		} else
			break;
	}

	int base_line = _font_metrics.base_line();
	int line_height = _line_height;

	int add_x = 0;
	switch (_text_align) {
	case text_align_right:
		if (_width < (_box_right - _box_left))
			add_x = (_box_right - _box_left) - _width;
		break;
	case text_align_center:
		if (_width < (_box_right - _box_left))
			add_x = ((_box_right - _box_left) - _width) / 2;
		break;
	default:
		add_x = 0;
	}

	_height = 0;
	// find line box baseline and line-height
	for (const auto &el : _items) {
		if (el->get_display() == display_inline_text) {
			font_metrics fm;
			el->get_font(&fm);
			base_line = std::max(base_line, fm.base_line());
			line_height = std::max(line_height, el->line_height());
			_height = std::max(_height, fm.height);
		}
		el->_pos.x += add_x;
	}
	if (_height)
		base_line += (line_height - _height) / 2;
	_height = line_height;
	int y1	= 0;
	int y2	= _height;
	for (const auto &el : _items) {
		if (el->get_display() == display_inline_text) {
			font_metrics fm;
			el->get_font(&fm);
			el->_pos.y = _height - base_line - fm.ascent;
		}
		else {
			switch (el->get_vertical_align()) {
			case va_super:
			case va_sub:
			case va_baseline: el->_pos.y = _height - base_line - el->height() + el->get_base_line() + el->content_margins_top(); break;
			case va_top: el->_pos.y = y1 + el->content_margins_top(); break;
			case va_text_top: el->_pos.y = _height - base_line - _font_metrics.ascent + el->content_margins_top(); break;
			case va_middle: el->_pos.y = _height - base_line - _font_metrics.x_height / 2 - el->height() / 2 + el->content_margins_top(); break;
			case va_bottom: el->_pos.y = y2 - el->height() + el->content_margins_top(); break;
			case va_text_bottom: el->_pos.y = _height - base_line + _font_metrics.descent - el->height() + el->content_margins_top(); break;
			}
			y1 = std::min(y1, el->top());
			y2 = std::max(y2, el->bottom());
		}
	}

	css_offsets offsets;
	for (const auto &el : _items) {
		el->_pos.y -= y1;
		el->_pos.y += _box_top;
		if (el->get_display() != display_inline_text)
			switch (el->get_vertical_align()) {
			case va_top:
				el->_pos.y = _box_top + el->content_margins_top();
				break;
			case va_bottom:
				el->_pos.y = _box_top + (y2 - y1) - el->height() + el->content_margins_top();
				break;
			case va_baseline:
				//TODO: process vertical align "baseline"
				break;
			case va_middle:
				//TODO: process vertical align "middle"
				break;
			case va_sub:
				//TODO: process vertical align "sub"
				break;
			case va_super:
				//TODO: process vertical align "super"
				break;
			case va_text_bottom:
				//TODO: process vertical align "text-bottom"
				break;
			case va_text_top:
				//TODO: process vertical align "text-top"
				break;
		}
		el->apply_relative_shift(_box_right - _box_left);
	}
	_height = y2 - y1;
	_baseline = (base_line - y1) - (_height - line_height);
}

bool line_box::can_hold(const element::ptr &el, white_space ws)
{
	if (!el->is_inline_box()) return false;
	if (el->is_break())
		return false;
	if (ws == white_space_nowrap || ws == white_space_pre)
		return true;
	if (_box_left + _width + el->width() + el->get_inline_shift_left() + el->get_inline_shift_right() > _box_right)
		return false;
	return true;
}

bool line_box::have_last_space()
{
	bool ret = false;
	for (auto i = _items.rbegin(); i != _items.rend() && !ret; i++) {
		if ((*i)->is_white_space() || (*i)->is_break())
			ret = true;
		else
			break;
	}
	return ret;
}

bool line_box::is_empty()
{
	if (_items.empty()) return true;
	for (auto i = _items.rbegin(); i != _items.rend(); i++)
		if(!(*i)->_skip || (*i)->is_break())
			return false;
	return true;
}

int line_box::baseline()
{
	return _baseline;
}

void line_box::get_elements( elements_vector &els )
{
	els.insert(els.begin(), _items.begin(), _items.end());
}

int line_box::top_margin()
{
	return 0;
}

int line_box::bottom_margin()
{
	return 0;
}

void line_box::y_shift( int shift )
{
	_box_top += shift;
	for (auto &el : _items)
		el->_pos.y += shift;
}

bool line_box::is_break_only()
{
	if (_items.empty()) return true;
	if (_items.front()->is_break()) {
		for (auto &el : _items)
			if(!el->_skip)
				return false;
		return true;
	}
	return false;
}

void line_box::new_width(int left, int right, elements_vector &els)
{
	int add = left - _box_left;
	if (add) {
		_box_left = left;
		_box_right = right;
		_width = 0;
		auto remove_begin = _items.end();
		for (auto i = _items.begin() + 1; i != _items.end(); i++) {
			element::ptr el = (*i);
			if (!el->_skip) {
				if (_box_left + _width + el->width() + el->get_inline_shift_right() + el->get_inline_shift_left() > _box_right) {
					remove_begin = i;
					break;
				}
				else {
					el->_pos.x += add;
					_width += el->width() + el->get_inline_shift_right() + el->get_inline_shift_left();
				}
			}
		}
		if (remove_begin != _items.end()) {
			els.insert(els.begin(), remove_begin, _items.end());
			_items.erase(remove_begin, _items.end());
			for (const auto &el : els)
				el->_box = nullptr;
		}
	}
}


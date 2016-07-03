#include "html.h"
#include "html_tag.h"
#include "document.h"
#include "iterators.h"
#include "stylesheet.h"
#include "table.h"
#include <algorithm>
#include <locale>
#include "el_before_after.h"
using namespace litehtml;

html_tag::html_tag(const std::shared_ptr<document> &doc) : element(doc)
{
	_box_sizing = box_sizing_content_box;
	_z_index = 0;
	_overflow = overflow_visible;
	_box = 0;
	_text_align = text_align_left;
	_el_position = element_position_static;
	_display = display_inline;
	_vertical_align = va_baseline;
	_list_style_type = list_style_type_none;
	_list_style_position = list_style_position_outside;
	_float = float_none;
	_clear = clear_none;
	_font = 0;
	_font_size = 0;
	_white_space = white_space_normal;
	_lh_predefined = false;
	_line_height = 0;
	_visibility = visibility_visible;
	_border_spacing_x = 0;
	_border_spacing_y = 0;
	_border_collapse = border_collapse_separate;
}

html_tag::~html_tag() { }

bool html_tag::appendChild(const element::ptr &el)
{
	if (el) {
		el->parent(shared_from_this());
		_children.push_back(el);
		return true;
	}
	return false;
}

bool html_tag::removeChild(const element::ptr &el)
{
	if (el && el->parent() == shared_from_this()) {
		el->parent(nullptr);
		_children.erase(std::remove(_children.begin(), _children.end(), el), _children.end());
		return true;
	}
	return false;
}

void html_tag::clearRecursive()
{
	for (auto &el : _children) {
		el->clearRecursive();
		el->parent(nullptr);
	}
	_children.clear();
}

const tchar_t *html_tag::get_tagName() const
{
	return _tag.c_str();
}

void html_tag::set_attr(const tchar_t *name, const tchar_t *val)
{
	if (name && val) {
		tstring s_val = name;
		std::locale lc = std::locale::global(std::locale::classic());
		for (size_t i = 0; i < s_val.length(); i++)
			s_val[i] = std::tolower(s_val[i], lc);
		_attrs[s_val] = val;
		if (t_strcasecmp(name, _t("class")) == 0) {
			_class_values.resize(0);
			split_string(val, _class_values, _t(" "));
		}
	}
}

const tchar_t *html_tag::get_attr(const tchar_t *name, const tchar_t *def)
{
	string_map::const_iterator attr = _attrs.find(name);
	if (attr != _attrs.end())
		return attr->second.c_str();
	return def;
}

elements_vector html_tag::select_all( const tstring &selector )
{
	css_selector sel(media_query_list::ptr(0));
	sel.parse(selector);

	return select_all(sel);
}

elements_vector html_tag::select_all( const css_selector &selector )
{
	elements_vector res;
	select_all(selector, res);
	return res;
}

void html_tag::select_all(const css_selector &selector, elements_vector &res)
{
	if (select(selector))
		res.push_back(shared_from_this());
	for (auto &el : _children)
		el->select_all(selector, res);
}

element::ptr html_tag::select_one(const tstring &selector)
{
	css_selector sel(media_query_list::ptr(0));
	sel.parse(selector);
	return select_one(sel);
}

element::ptr html_tag::select_one(const css_selector &selector)
{
	if (select(selector))
		return shared_from_this();
	for (auto &el : _children) {
		element::ptr res = el->select_one(selector);
		if (res)
			return res;
	}
	return nullptr;
}

void html_tag::apply_stylesheet(const css &stylesheet)
{
	remove_before_after();
	for (const auto &sel : stylesheet.selectors()) {
		int apply = select(*sel, false);
		if (apply != select_no_match) {
			used_selector::ptr us = std::unique_ptr<used_selector>(new used_selector(sel, false));
			if (sel->is_media_valid()) {
				if (apply & select_match_pseudo_class) {
					if (select(*sel, true)) {
						if (apply & select_match_with_after) {
							element::ptr el = get_element_after();
							if (el)
								el->add_style(*sel->_style);
						}
						else if (apply & select_match_with_before) {
							element::ptr el = get_element_before();
							if (el)
								el->add_style(*sel->_style);
						}
						else {
							add_style(*sel->_style);
							us->_used = true;
						}
					}
				}
				else if (apply & select_match_with_after) {
					element::ptr el = get_element_after();
					if (el)
						el->add_style(*sel->_style);
				}
				else if (apply & select_match_with_before) {
					element::ptr el = get_element_before();
					if (el)
						el->add_style(*sel->_style);
				}
				else {
					add_style(*sel->_style);
					us->_used = true;
				}
			}
			_used_styles.push_back(std::move(us));
		}
	}
	for (auto &el : _children)
		if (el->get_display() != display_inline_text)
			el->apply_stylesheet(stylesheet);
}

void html_tag::get_content_size(size &sz, int max_width)
{
	sz.height = 0;
	sz.width = (_display == display_block ? max_width : 0);
}

void html_tag::draw(uint_ptr hdc, int x, int y, const position *clip)
{
	position pos = _pos;
	pos.x += x;
	pos.y += y;
	draw_background(hdc, x, y, clip);
	if (_display == display_list_item && _list_style_type != list_style_type_none) {
		if (_overflow > overflow_visible) {
			position border_box = pos;
			border_box += _padding;
			border_box += _borders;
			border_radiuses bdr_radius = _css_borders.radius.calc_percents(border_box.width, border_box.height);
			bdr_radius -= _borders;
			bdr_radius -= _padding;
			get_document()->container()->SetClip(pos, bdr_radius, true, true);
		}
		draw_list_marker(hdc, pos);
		if (_overflow > overflow_visible)
			get_document()->container()->DelClip();
	}
}

uint_ptr html_tag::get_font(font_metrics *fm)
{
	if (fm)
		*fm = _font_metrics;
	return _font;
}

const tchar_t *html_tag::get_style_property(const tchar_t *name, bool inherited, const tchar_t *def /*= nullptr*/)
{
	const tchar_t* ret = _style.get_property(name);
	element::ptr el_parent = parent();
	if (el_parent)
		if ((ret && !t_strcasecmp(ret, _t("inherit")) ) || (!ret && inherited))
			ret = el_parent->get_style_property(name, inherited, def);
	if (!ret)
		ret = def;
	return ret;
}

void html_tag::parse_styles(bool is_reparse)
{
	const tchar_t* style = get_attr(_t("style"));
	if (style)
		_style.add(style, NULL);
	init_font();
	document::ptr doc = get_document();

	_el_position = (element_position)value_index(get_style_property(_t("position"), false, _t("static")), element_position_strings, element_position_fixed);
	_text_align	= (text_align)value_index(get_style_property(_t("text-align"), true, _t("left")), text_align_strings, text_align_left);
	_overflow = (overflow)value_index(get_style_property(_t("overflow"), false,	_t("visible")), overflow_strings, overflow_visible);
	_white_space = (white_space)value_index(get_style_property(_t("white-space"), true,	_t("normal")), white_space_strings, white_space_normal);
	_display = (style_display)value_index(get_style_property(_t("display"), false, _t("inline")), style_display_strings, display_inline);
	_visibility	= (visibility)value_index(get_style_property(_t("visibility"), true, _t("visible")), visibility_strings, visibility_visible);
	_box_sizing	= (box_sizing)value_index(get_style_property(_t("box-sizing"), false, _t("content-box")), box_sizing_strings, box_sizing_content_box);

	if (_el_position != element_position_static) {
		const tchar_t *val = get_style_property(_t("z-index"), false, 0);
		if (val)
			_z_index = t_atoi(val);
	}

	const tchar_t *va = get_style_property(_t("vertical-align"), true, _t("baseline"));
	_vertical_align = (vertical_align)value_index(va, vertical_align_strings, va_baseline);

	const tchar_t* fl = get_style_property(_t("float"), false, _t("none"));
	_float = (element_float)value_index(fl, element_float_strings, float_none);
	_clear = (element_clear)value_index(get_style_property(_t("clear"), false, _t("none")), element_clear_strings, clear_none);

	if (_float != float_none) {
		// reset display in to block for floating elements
		if (_display != display_none)
			_display = display_block;
	}
	else if (_display == display_table ||
		_display == display_table_caption ||
		_display == display_table_cell ||
		_display == display_table_column ||
		_display == display_table_column_group ||
		_display == display_table_footer_group ||
		_display == display_table_header_group ||
		_display == display_table_row ||
		_display == display_table_row_group) {
			doc->add_tabular(shared_from_this());
	}
	// fix inline boxes with absolute/fixed positions
	else if (_display != display_none && is_inline_box()) {
		if (_el_position == element_position_absolute || _el_position == element_position_fixed)
			_display = display_block;
	}

	_css_text_indent.fromString(get_style_property(_t("text-indent"), true, _t("0")), _t("0"));

	_css_width.fromString(get_style_property(_t("width"), false, _t("auto")), _t("auto"));
	_css_height.fromString(get_style_property(_t("height"), false, _t("auto")), _t("auto"));

	doc->cvt_units(_css_width, _font_size);
	doc->cvt_units(_css_height, _font_size);

	_css_min_width.fromString(get_style_property(_t("min-width"), false, _t("0")));
	_css_min_height.fromString(get_style_property(_t("min-height"), false, _t("0")));

	_css_max_width.fromString(get_style_property(_t("max-width"), false, _t("none")), _t("none"));
	_css_max_height.fromString(get_style_property(_t("max-height"), false, _t("none")), _t("none"));

	doc->cvt_units(_css_min_width, _font_size);
	doc->cvt_units(_css_min_height, _font_size);

	_css_offsets.left.fromString(get_style_property(_t("left"), false, _t("auto")), _t("auto"));
	_css_offsets.right.fromString(get_style_property(_t("right"), false, _t("auto")), _t("auto"));
	_css_offsets.top.fromString(get_style_property(_t("top"), false, _t("auto")), _t("auto"));
	_css_offsets.bottom.fromString(get_style_property(_t("bottom"), false, _t("auto")), _t("auto"));

	doc->cvt_units(_css_offsets.left, _font_size);
	doc->cvt_units(_css_offsets.right, _font_size);
	doc->cvt_units(_css_offsets.top, _font_size);
	doc->cvt_units(_css_offsets.bottom,	_font_size);

	_css_margins.left.fromString(get_style_property(_t("margin-left"), false, _t("0")), _t("auto"));
	_css_margins.right.fromString(get_style_property(_t("margin-right"), false, _t("0")), _t("auto"));
	_css_margins.top.fromString(get_style_property(_t("margin-top"), false, _t("0")), _t("auto"));
	_css_margins.bottom.fromString(get_style_property(_t("margin-bottom"), false, _t("0")), _t("auto"));

	_css_padding.left.fromString(get_style_property(_t("padding-left"), false, _t("0")), _t(""));
	_css_padding.right.fromString(get_style_property(_t("padding-right"), false, _t("0")), _t(""));
	_css_padding.top.fromString(get_style_property(_t("padding-top"), false, _t("0")), _t(""));
	_css_padding.bottom.fromString(get_style_property(_t("padding-bottom"), false, _t("0")), _t(""));

	_css_borders.left.width.fromString(get_style_property(_t("border-left-width"), false, _t("medium")), border_width_strings);
	_css_borders.right.width.fromString(get_style_property(_t("border-right-width"), false, _t("medium")), border_width_strings);
	_css_borders.top.width.fromString(get_style_property(_t("border-top-width"), false, _t("medium")), border_width_strings);
	_css_borders.bottom.width.fromString(get_style_property(_t("border-bottom-width"), false, _t("medium")), border_width_strings);

	_css_borders.left.color = web_color::fro_string(get_style_property(_t("border-left-color"),	false,_t("")));
	_css_borders.left.style = (border_style) value_index(get_style_property(_t("border-left-style"), false, _t("none")), border_style_strings, border_style_none);

	_css_borders.right.color = web_color::fro_string(get_style_property(_t("border-right-color"), false, _t("")));
	_css_borders.right.style = (border_style) value_index(get_style_property(_t("border-right-style"), false, _t("none")), border_style_strings, border_style_none);

	_css_borders.top.color = web_color::fro_string(get_style_property(_t("border-top-color"), false, _t("")));
	_css_borders.top.style = (border_style) value_index(get_style_property(_t("border-top-style"), false, _t("none")), border_style_strings, border_style_none);

	_css_borders.bottom.color = web_color::fro_string(get_style_property(_t("border-bottom-color"),	false, _t("")));
	_css_borders.bottom.style = (border_style) value_index(get_style_property(_t("border-bottom-style"), false, _t("none")), border_style_strings, border_style_none);

	_css_borders.radius.top_left_x.fromString(get_style_property(_t("border-top-left-radius-x"), false, _t("0")));
	_css_borders.radius.top_left_y.fromString(get_style_property(_t("border-top-left-radius-y"), false, _t("0")));

	_css_borders.radius.top_right_x.fromString(get_style_property(_t("border-top-right-radius-x"), false, _t("0")));
	_css_borders.radius.top_right_y.fromString(get_style_property(_t("border-top-right-radius-y"), false, _t("0")));

	_css_borders.radius.bottom_right_x.fromString(get_style_property(_t("border-bottom-right-radius-x"), false, _t("0")));
	_css_borders.radius.bottom_right_y.fromString(get_style_property(_t("border-bottom-right-radius-y"), false, _t("0")));

	_css_borders.radius.bottom_left_x.fromString(get_style_property(_t("border-bottom-left-radius-x"), false, _t("0")));
	_css_borders.radius.bottom_left_y.fromString(get_style_property(_t("border-bottom-left-radius-y"), false, _t("0")));

	doc->cvt_units(_css_borders.radius.bottom_left_x, _font_size);
	doc->cvt_units(_css_borders.radius.bottom_left_y, _font_size);
	doc->cvt_units(_css_borders.radius.bottom_right_x, _font_size);
	doc->cvt_units(_css_borders.radius.bottom_right_y, _font_size);
	doc->cvt_units(_css_borders.radius.top_left_x, _font_size);
	doc->cvt_units(_css_borders.radius.top_left_y, _font_size);
	doc->cvt_units(_css_borders.radius.top_right_x, _font_size);
	doc->cvt_units(_css_borders.radius.top_right_y, _font_size);

	doc->cvt_units(_css_text_indent, _font_size);

	_margins.left = doc->cvt_units(_css_margins.left, _font_size);
	_margins.right = doc->cvt_units(_css_margins.right, _font_size);
	_margins.top = doc->cvt_units(_css_margins.top, _font_size);
	_margins.bottom	= doc->cvt_units(_css_margins.bottom, _font_size);

	_padding.left = doc->cvt_units(_css_padding.left, _font_size);
	_padding.right = doc->cvt_units(_css_padding.right, _font_size);
	_padding.top = doc->cvt_units(_css_padding.top, _font_size);
	_padding.bottom	= doc->cvt_units(_css_padding.bottom, _font_size);

	_borders.left = doc->cvt_units(_css_borders.left.width,	_font_size);
	_borders.right = doc->cvt_units(_css_borders.right.width, _font_size);
	_borders.top = doc->cvt_units(_css_borders.top.width, _font_size);
	_borders.bottom	= doc->cvt_units(_css_borders.bottom.width, _font_size);

	css_length line_height;
	line_height.fromString(get_style_property(_t("line-height"), true, _t("normal")), _t("normal"));
	if (line_height.is_predefined()) {
		_line_height = _font_metrics.height;
		_lh_predefined = true;
	}
	else if (line_height.units() == css_units_none) {
		_line_height = (int)(line_height.val() * _font_size);
		_lh_predefined = false;
	}
	else {
		_line_height = doc->cvt_units(line_height, _font_size, _font_size);
		_lh_predefined = false;
	}

	if (_display == display_list_item) {
		const tchar_t *list_type = get_style_property(_t("list-style-type"), true, _t("disc"));
		_list_style_type = (list_style_type) value_index(list_type, list_style_type_strings, list_style_type_disc);

		const tchar_t *list_pos = get_style_property(_t("list-style-position"), true, _t("outside"));
		_list_style_position = (list_style_position) value_index(list_pos, list_style_position_strings, list_style_position_outside);

		const tchar_t *list_image = get_style_property(_t("list-style-image"), true, 0);
		if (list_image && list_image[0]) {
			tstring url;
			css::parse_css_url(list_image, url);
			const tchar_t *list_image_baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
			doc->container()->LoadImage(url.c_str(), list_image_baseurl, true);
		}
	}

	parse_background();

	if (!is_reparse)
		for (auto &el : _children)
			el->parse_styles();
}

int html_tag::render(int x, int y, int max_width, bool second_pass)
{
	if (_display == display_table || _display == display_inline_table)
		return render_table(x, y, max_width, second_pass);
	return render_box(x, y, max_width, second_pass);
}

bool html_tag::is_white_space() const
{
	return false;
}

int html_tag::get_font_size() const
{
	return _font_size;
}

int html_tag::get_base_line()
{
	if (is_replaced())
		return 0;
	int bl = 0;
	if (!_boxes.empty())
		bl = _boxes.back()->baseline() + content_margins_bottom();
	return bl;
}

void html_tag::init()
{
	if (_display == display_table || _display == display_inline_table)
	{
		if (_grid)
			_grid->clear();
		else
			_grid = std::unique_ptr<table_grid>(new table_grid());

		go_inside_table table_selector;
		table_rows_selector row_selector;
		table_cells_selector cell_selector;
		elements_iterator row_iter(shared_from_this(), &table_selector, &row_selector);

		element::ptr row = row_iter.next(false);
		while (row) {
			_grid->begin_row(row);
			elements_iterator cell_iter(row, &table_selector, &cell_selector);
			element::ptr cell = cell_iter.next();
			while (cell) {
				_grid->add_cell(cell);
				cell = cell_iter.next(false);
			}
			row = row_iter.next(false);
		}
		_grid->finish();
	}
	for (auto &el : _children)
		el->init();
}

int html_tag::select(const css_selector &selector, bool apply_pseudo)
{
	int right_res = select(selector._right, apply_pseudo);
	if (right_res == select_no_match)
		return select_no_match;
	element::ptr el_parent = parent();
	if (selector._left) {
		if (!el_parent)
			return select_no_match;
		switch(selector._combinator) {
		case combinator_descendant: {
			bool is_pseudo = false;
			element::ptr res = find_ancestor(*selector._left, apply_pseudo, &is_pseudo);
			if (!res)
				return select_no_match;
			if (is_pseudo)
				right_res |= select_match_pseudo_class;
			break; }
		case combinator_child: {
			int res = el_parent->select(*selector._left, apply_pseudo);
			if (res == select_no_match)
				return select_no_match;
			if (right_res != select_match_pseudo_class)
				right_res |= res;
			break; }
		case combinator_adjacent_sibling: {
			bool is_pseudo = false;
			element::ptr res = el_parent->find_adjacent_sibling(shared_from_this(), *selector._left, apply_pseudo, &is_pseudo);
			if (!res)
				return select_no_match;
			if (is_pseudo)
				right_res |= select_match_pseudo_class;
			break; }
		case combinator_general_sibling: {
			bool is_pseudo = false;
			element::ptr res =  el_parent->find_sibling(shared_from_this(), *selector._left, apply_pseudo, &is_pseudo);
			if (!res)
				return select_no_match;
			if (is_pseudo)
				right_res |= select_match_pseudo_class;
			break; }
		default:
			right_res = select_no_match;
		}
	}
	return right_res;
}

int html_tag::select(const css_element_selector &selector, bool apply_pseudo)
{
	if (!selector._tag.empty() && selector._tag != _t("*"))
		if (selector._tag != _tag)
			return select_no_match;

	int res = select_match;
	element::ptr el_parent = parent();
	for (css_attribute_selector::vector::const_iterator i = selector._attrs.begin(); i != selector._attrs.end(); i++) {
		const tchar_t *attr_value = get_attr(i->attribute.c_str());
		switch (i->condition) {
		case select_exists:
			if (!attr_value)
				return select_no_match;
			break;
		case select_equal:
			if (!attr_value)
				return select_no_match;
			if (i->attribute == _t("class")) {
				const string_vector &tokens1 = _class_values;
				const string_vector &tokens2 = i->class_val;
				bool found = true;
				for (string_vector::const_iterator str1 = tokens2.begin(); str1 != tokens2.end() && found; str1++)
				{
					bool f = false;
					for (string_vector::const_iterator str2 = tokens1.begin(); str2 != tokens1.end() && !f; str2++)
						if (!t_strcasecmp(str1->c_str(), str2->c_str()))
							f = true;
					if (!f)
						found = false;
				}
				if (!found)
					return select_no_match;
			}
			else if (t_strcasecmp(i->val.c_str(), attr_value))
				return select_no_match;
			break;
		case select_contain_str:
			if(!attr_value)
				return select_no_match;
			if (!t_strstr(attr_value, i->val.c_str()))
				return select_no_match;
			break;
		case select_start_str:
			if (!attr_value)
				return select_no_match;
			if (t_strncmp(attr_value, i->val.c_str(), i->val.length()))
				return select_no_match;
			break;
		case select_end_str:
			if (!attr_value)
				return select_no_match;
			if (t_strncmp(attr_value, i->val.c_str(), i->val.length())) {
				const tchar_t *s = attr_value + t_strlen(attr_value) - i->val.length() - 1;
				if (s < attr_value)
					return select_no_match;
				if (i->val != s)
					return select_no_match;
			}
			break;
		case select_pseudo_element:
			if (i->val == _t("after"))
				res |= select_match_with_after;
			else if (i->val == _t("before"))
				res |= select_match_with_before;
			else
				return select_no_match;
			break;
		case select_pseudo_class:
			if (apply_pseudo)
			{
				if (!el_parent) return select_no_match;

				tstring selector_param;
				tstring	selector_name;
				tstring::size_type begin = i->val.find_first_of(_t('('));
				tstring::size_type end = (begin == tstring::npos ? tstring::npos : find_close_bracket(i->val, begin));
				if (begin != tstring::npos && end != tstring::npos)
					selector_param = i->val.substr(begin + 1, end - begin - 1);
				if (begin != tstring::npos) {
					selector_name = i->val.substr(0, begin);
					trim(selector_name);
				}
				else
					selector_name = i->val;

				int selector = value_index(selector_name.c_str(), pseudo_class_strings);
				switch (selector) {
				case pseudo_class_only_child:
					if (!el_parent->is_only_child(shared_from_this(), false))
						return select_no_match;
					break;
				case pseudo_class_only_of_type:
					if (!el_parent->is_only_child(shared_from_this(), true))
						return select_no_match;
					break;
				case pseudo_class_first_child:
					if (!el_parent->is_nth_child(shared_from_this(), 0, 1, false))
						return select_no_match;
					break;
				case pseudo_class_first_of_type:
					if (!el_parent->is_nth_child(shared_from_this(), 0, 1, true))
						return select_no_match;
					break;
				case pseudo_class_last_child:
					if (!el_parent->is_nth_last_child(shared_from_this(), 0, 1, false))
						return select_no_match;
					break;
				case pseudo_class_last_of_type:
					if (!el_parent->is_nth_last_child(shared_from_this(), 0, 1, true))
						return select_no_match;
					break;
				case pseudo_class_nth_child:
				case pseudo_class_nth_of_type:
				case pseudo_class_nth_last_child:
				case pseudo_class_nth_last_of_type: {
					if (selector_param.empty()) return select_no_match;

					int num = 0;
					int off = 0;
					parse_nth_child_params(selector_param, num, off);
					if (!num && !off) return select_no_match;
					switch (selector) {
					case pseudo_class_nth_child:
						if (!el_parent->is_nth_child(shared_from_this(), num, off, false))
							return select_no_match;
						break;
					case pseudo_class_nth_of_type:
						if (!el_parent->is_nth_child(shared_from_this(), num, off, true))
							return select_no_match;
						break;
					case pseudo_class_nth_last_child:
						if (!el_parent->is_nth_last_child(shared_from_this(), num, off, false))
							return select_no_match;
						break;
					case pseudo_class_nth_last_of_type:
						if (!el_parent->is_nth_last_child(shared_from_this(), num, off, true))
							return select_no_match;
						break;
					}
					break; }
				case pseudo_class_not: {
					css_element_selector sel;
					sel.parse(selector_param);
					if (select(sel, apply_pseudo))
						return select_no_match;
					break; }
				case pseudo_class_lang: {
					trim(selector_param);
					if (!get_document()->match_lang(selector_param))
						return select_no_match;
					break; }
				default:
					if (std::find(_pseudo_classes.begin(), _pseudo_classes.end(), i->val) == _pseudo_classes.end())
						return select_no_match;
					break;
				}
			}
			else
				res |= select_match_pseudo_class;
			break;
		}
	}
	return res;
}

element::ptr html_tag::find_ancestor(const css_selector &selector, bool apply_pseudo, bool* is_pseudo)
{
	element::ptr el_parent = parent();
	if (!el_parent)
		return nullptr;
	int res = el_parent->select(selector, apply_pseudo);
	if (res != select_no_match) {
		if (is_pseudo)
			*is_pseudo = (res & select_match_pseudo_class ? true : false);
		return el_parent;
	}
	return el_parent->find_ancestor(selector, apply_pseudo, is_pseudo);
}

int html_tag::get_floats_height(element_float el_float) const
{
	if (is_floats_holder())
	{
		int h = 0;
		bool process = false;
		for (const auto &fb : _floats_left) {
			process = false;
			switch (el_float) {
			case float_none:
				process = true;
				break;
			case float_left:
				if (fb.clear_floats == clear_left || fb.clear_floats == clear_both)
					process = true;
				break;
			case float_right:
				if (fb.clear_floats == clear_right || fb.clear_floats == clear_both)
					process = true;
				break;
			}
			if (process)
				h = (el_float == float_none ? std::max(h, fb.pos.bottom()) : std::max(h, fb.pos.top()));
		}

		for (const auto fb : _floats_right) {
			process = false;
			switch (el_float) {
			case float_none:
				process = true;
				break;
			case float_left:
				if (fb.clear_floats == clear_left || fb.clear_floats == clear_both)
					process = true;
				break;
			case float_right:
				if (fb.clear_floats == clear_right || fb.clear_floats == clear_both)
					process = true;
				break;
			}
			if (process)
				h = (el_float == float_none ? std::max(h, fb.pos.bottom()) : std::max(h, fb.pos.top()));
		}
		return h;
	}
	element::ptr el_parent = parent();
	if (el_parent) {
		int h = el_parent->get_floats_height(el_float);
		return h - _pos.y;
	}
	return 0;
}

int html_tag::get_left_floats_height() const
{
	if (is_floats_holder()) {
		int h = 0;
		if (!_floats_left.empty())
			for (const auto &fb : _floats_left)
				h = std::max(h, fb.pos.bottom());
		return h;
	}
	element::ptr el_parent = parent();
	if (el_parent) {
		int h = el_parent->get_left_floats_height();
		return h - _pos.y;
	}
	return 0;
}

int html_tag::get_right_floats_height() const
{
	if (is_floats_holder()) {
		int h = 0;
		if (!_floats_right.empty())
			for(const auto &fb : _floats_right)
				h = std::max(h, fb.pos.bottom());
		return h;
	}
	element::ptr el_parent = parent();
	if (el_parent) {
		int h = el_parent->get_right_floats_height();
		return h - _pos.y;
	}
	return 0;
}

int html_tag::get_line_left( int y )
{
	if (is_floats_holder()) {
		if (_cahe_line_left.is_valid && _cahe_line_left.hash == y)
			return _cahe_line_left.val;
		int w = 0;
		for (const auto &fb : _floats_left) {
			if (y >= fb.pos.top() && y < fb.pos.bottom()) {
				w = std::max(w, fb.pos.right());
				if (w < fb.pos.right())
					break;
			}
		}
		_cahe_line_left.set_value(y, w);
		return w;
	}
	element::ptr el_parent = parent();
	if (el_parent) {
		int w = el_parent->get_line_left(y + _pos.y);
		if (w < 0)
			w = 0;
		return w - (w ? _pos.x : 0);
	}
	return 0;
}

int html_tag::get_line_right(int y, int def_right)
{
	if (is_floats_holder()) {
		if (_cahe_line_right.is_valid && _cahe_line_right.hash == y)
			return (_cahe_line_right.is_default ? def_right : std::min(_cahe_line_right.val, def_right));
		int w = def_right;
		_cahe_line_right.is_default = true;

		for (const auto &fb : _floats_right) {
			if (y >= fb.pos.top() && y < fb.pos.bottom()) {
				w = std::min(w, fb.pos.left());
				_cahe_line_right.is_default = false;
				if (w > fb.pos.left())
					break;
			}
		}
		_cahe_line_right.set_value(y, w);
		return w;
	}
	element::ptr el_parent = parent();
	if (el_parent) {
		int w = el_parent->get_line_right(y + _pos.y, def_right + _pos.x);
		return w - _pos.x;
	}
	return 0;
}

void html_tag::get_line_left_right(int y, int def_right, int &ln_left, int &ln_right)
{
	if (is_floats_holder()) {
		ln_left = get_line_left(y);
		ln_right = get_line_right(y, def_right);
	}
	else {
		element::ptr el_parent = parent();
		if (el_parent)
			el_parent->get_line_left_right(y + _pos.y, def_right + _pos.x, ln_left, ln_right);
		ln_right -= _pos.x;
		if (ln_left < 0)
			ln_left = 0;
		else if (ln_left)
			ln_left -= _pos.x;
	}
}

int html_tag::fix_line_width( int max_width, element_float flt )
{
	int ret_width = 0;
	if (!_boxes.empty())
	{
		elements_vector els;
		_boxes.back()->get_elements(els);
		bool was_cleared = false;
		if (!els.empty() && els.front()->get_clear() != clear_none) {
			if (els.front()->get_clear() == clear_both)
				was_cleared = true;
			else if ((flt == float_left && els.front()->get_clear() == clear_left) || (flt == float_right && els.front()->get_clear() == clear_right))
				was_cleared = true;
		}

		if (!was_cleared) {
			_boxes.pop_back();
			for (elements_vector::iterator i = els.begin(); i != els.end(); i++) {
				int rw = place_element((*i), max_width);
				if (rw > ret_width)
					ret_width = rw;
			}
		}
		else {
			int line_top = (_boxes.back()->get_type() == box_line ? _boxes.back()->top() : _boxes.back()->bottom());
			int line_left = 0;
			int line_right = max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);

			if (_boxes.back()->get_type() == box_line)
			{
				if (_boxes.size() == 1 && _list_style_type != list_style_type_none && _list_style_position == list_style_position_inside) {
					int sz_font = get_font_size();
					line_left += sz_font;
				}
				if (_css_text_indent.val() != 0) {
					bool line_box_found = false;
					for (box::vector::iterator iter = _boxes.begin(); iter < _boxes.end(); iter++) {
						if ((*iter)->get_type() == box_line) {
							line_box_found = true;
							break;
						}
					}
					if (!line_box_found)
						line_left += _css_text_indent.calc_percent(max_width);
				}
			}

			elements_vector els;
			_boxes.back()->new_width(line_left, line_right, els);
			for (auto &el : els) {
				int rw = place_element(el, max_width);
				if (rw > ret_width)
					ret_width = rw;
			}
		}
	}
	return ret_width;
}

void html_tag::add_float(const element::ptr &el, int x, int y)
{
	if (is_floats_holder()) {
		floated_box fb;
		fb.pos.x = el->left() + x;
		fb.pos.y = el->top()  + y;
		fb.pos.width = el->width();
		fb.pos.height = el->height();
		fb.float_side = el->get_float();
		fb.clear_floats	= el->get_clear();
		fb.el = el;

		if (fb.float_side == float_left) {
			if (_floats_left.empty())
				_floats_left.push_back(fb);
			else {
				bool inserted = false;
				for (floated_box::vector::iterator i = _floats_left.begin(); i != _floats_left.end(); i++) {
					if (fb.pos.right() > i->pos.right()) {
						_floats_left.insert(i, std::move(fb));
						inserted = true;
						break;
					}
				}
				if (!inserted)
					_floats_left.push_back(std::move(fb));
			}
			_cahe_line_left.invalidate();
		}
		else if (fb.float_side == float_right) {
			if (_floats_right.empty())
				_floats_right.push_back(std::move(fb));
			else {
				bool inserted = false;
				for (floated_box::vector::iterator i = _floats_right.begin(); i != _floats_right.end(); i++) {
					if (fb.pos.left() < i->pos.left()) {
						_floats_right.insert(i, std::move(fb));
						inserted = true;
						break;
					}
				}
				if (!inserted)
					_floats_right.push_back(fb);
			}
			_cahe_line_right.invalidate();
		}
	}
	else {
		element::ptr el_parent = parent();
		if (el_parent)
			el_parent->add_float(el, x + _pos.x, y + _pos.y);
	}
}

int html_tag::find_next_line_top( int top, int width, int def_right )
{
	if (is_floats_holder()) {
		int new_top = top;
		int_vector points;

		for (const auto &fb : _floats_left) {
			if (fb.pos.top() >= top) {
				if (find(points.begin(), points.end(), fb.pos.top()) == points.end())
					points.push_back(fb.pos.top());
			}
			if (fb.pos.bottom() >= top)
				if (find(points.begin(), points.end(), fb.pos.bottom()) == points.end())
					points.push_back(fb.pos.bottom());
		}

		for (const auto &fb : _floats_right) {
			if (fb.pos.top() >= top)
				if (find(points.begin(), points.end(), fb.pos.top()) == points.end())
					points.push_back(fb.pos.top());
			if (fb.pos.bottom() >= top)
				if (find(points.begin(), points.end(), fb.pos.bottom()) == points.end())
					points.push_back(fb.pos.bottom());
		}

		if (!points.empty()) {
			sort(points.begin(), points.end(), std::less<int>( ));
			new_top = points.back();
			for (auto pt : points) {
				int pos_left = 0;
				int pos_right = def_right;
				get_line_left_right(pt, def_right, pos_left, pos_right);
				if (pos_right - pos_left >= width) {
					new_top = pt;
					break;
				}
			}
		}
		return new_top;
	}
	element::ptr el_parent = parent();
	if (el_parent) {
		int new_top = el_parent->find_next_line_top(top + _pos.y, width, def_right + _pos.x);
		return new_top - _pos.y;
	}
	return 0;
}

void html_tag::parse_background()
{
	// parse background-color
	_bg._color = get_color(_t("background-color"), false, web_color(0, 0, 0, 0));

	// parse background-position
	const tchar_t *str = get_style_property(_t("background-position"), false, _t("0% 0%"));
	if (str) {
		string_vector res;
		split_string(str, res, _t(" \t"));
		if (res.size() > 0) {
			if (res.size() == 1) {
				if (value_in_list(res[0].c_str(), _t("left;right;center"))) {
					_bg._position.x.fromString(res[0], _t("left;right;center"));
					_bg._position.y.set_value(50, css_units_percentage);
				}
				else if (value_in_list(res[0].c_str(), _t("top;bottom;center"))) {
					_bg._position.y.fromString(res[0], _t("top;bottom;center"));
					_bg._position.x.set_value(50, css_units_percentage);
				}
				else {
					_bg._position.x.fromString(res[0], _t("left;right;center"));
					_bg._position.y.set_value(50, css_units_percentage);
				}
			}
			else {
				if (value_in_list(res[0].c_str(), _t("left;right"))) {
					_bg._position.x.fromString(res[0], _t("left;right;center"));
					_bg._position.y.fromString(res[1], _t("top;bottom;center"));
				}
				else if (value_in_list(res[0].c_str(), _t("top;bottom"))) {
					_bg._position.x.fromString(res[1], _t("left;right;center"));
					_bg._position.y.fromString(res[0], _t("top;bottom;center"));
				}
				else if (value_in_list(res[1].c_str(), _t("left;right"))) {
					_bg._position.x.fromString(res[1], _t("left;right;center"));
					_bg._position.y.fromString(res[0], _t("top;bottom;center"));
				}
				else if (value_in_list(res[1].c_str(), _t("top;bottom"))) {
					_bg._position.x.fromString(res[0], _t("left;right;center"));
					_bg._position.y.fromString(res[1], _t("top;bottom;center"));
				}
				else {
					_bg._position.x.fromString(res[0], _t("left;right;center"));
					_bg._position.y.fromString(res[1], _t("top;bottom;center"));
				}
			}

			if (_bg._position.x.is_predefined()) {
				switch (_bg._position.x.predef()) {
				case 0: _bg._position.x.set_value(0, css_units_percentage); break;
				case 1: _bg._position.x.set_value(100, css_units_percentage); break;
				case 2: _bg._position.x.set_value(50, css_units_percentage); break;
				}
			}
			if (_bg._position.y.is_predefined()) {
				switch(_bg._position.y.predef()) {
				case 0: _bg._position.y.set_value(0, css_units_percentage); break;
				case 1: _bg._position.y.set_value(100, css_units_percentage); break;
				case 2: _bg._position.y.set_value(50, css_units_percentage); break;
				}
			}
		}
		else {
			_bg._position.x.set_value(0, css_units_percentage);
			_bg._position.y.set_value(0, css_units_percentage);
		}
	}
	else {
		_bg._position.y.set_value(0, css_units_percentage);
		_bg._position.x.set_value(0, css_units_percentage);
	}

	str = get_style_property(_t("background-size"), false, _t("auto"));
	if (str) {
		string_vector res;
		split_string(str, res, _t(" \t"));
		if (!res.empty()) {
			_bg._position.width.fromString(res[0], background_size_strings);
			if (res.size() > 1)
				_bg._position.height.fromString(res[1], background_size_strings);
			else
				_bg._position.height.predef(background_size_auto);
		}
		else {
			_bg._position.width.predef(background_size_auto);
			_bg._position.height.predef(background_size_auto);
		}
	}

	document::ptr doc = get_document();
	doc->cvt_units(_bg._position.x, _font_size);
	doc->cvt_units(_bg._position.y, _font_size);
	doc->cvt_units(_bg._position.width,	_font_size);
	doc->cvt_units(_bg._position.height, _font_size);

	// parse background_attachment
	_bg._attachment = (background_attachment) value_index(get_style_property(_t("background-attachment"), false, _t("scroll")), background_attachment_strings, background_attachment_scroll);
	// parse background_attachment
	_bg._repeat = (background_repeat) value_index(get_style_property(_t("background-repeat"), false, _t("repeat")), background_repeat_strings, background_repeat_repeat);
	// parse background_clip
	_bg._clip = (background_box) value_index(get_style_property(_t("background-clip"), false, _t("border-box")), background_box_strings, background_box_border);
	// parse background_origin
	_bg._origin = (background_box) value_index(get_style_property(_t("background-origin"), false, _t("padding-box")), background_box_strings, background_box_content);
	// parse background-image
	css::parse_css_url(get_style_property(_t("background-image"), false, _t("")), _bg._image);
	_bg._baseurl = get_style_property(_t("background-image-baseurl"), false, _t(""));
	if (!_bg._image.empty())
		doc->container()->LoadImage(_bg._image.c_str(), _bg._baseurl.empty() ? 0 : _bg._baseurl.c_str(), true);
}

void html_tag::add_positioned(const element::ptr &el)
{
	if (_el_position != element_position_static || (!have_parent()))
		_positioned.push_back(el);
	else {
		element::ptr el_parent = parent();
		if (el_parent)
			el_parent->add_positioned(el);
	}
}

void html_tag::calc_outlines( int parent_width )
{
	_padding.left = _css_padding.left.calc_percent(parent_width);
	_padding.right = _css_padding.right.calc_percent(parent_width);

	_borders.left = _css_borders.left.width.calc_percent(parent_width);
	_borders.right = _css_borders.right.width.calc_percent(parent_width);

	_margins.left = _css_margins.left.calc_percent(parent_width);
	_margins.right = _css_margins.right.calc_percent(parent_width);

	_margins.top = _css_margins.top.calc_percent(parent_width);
	_margins.bottom	= _css_margins.bottom.calc_percent(parent_width);

	_padding.top = _css_padding.top.calc_percent(parent_width);
	_padding.bottom	= _css_padding.bottom.calc_percent(parent_width);
}

void html_tag::calc_auto_margins(int parent_width)
{
	if (get_element_position() != element_position_absolute && (_display == display_block || _display == display_table)) {
		if (_css_margins.left.is_predefined() && _css_margins.right.is_predefined()) {
			int el_width = _pos.width + _borders.left + _borders.right + _padding.left + _padding.right;
			if (el_width <= parent_width) {
				_margins.left = (parent_width - el_width) / 2;
				_margins.right = (parent_width - el_width) - _margins.left;
			}
			else {
				_margins.left = 0;
				_margins.right = 0;
			}
		}
		else if (_css_margins.left.is_predefined() && !_css_margins.right.is_predefined()) {
			int el_width = _pos.width + _borders.left + _borders.right + _padding.left + _padding.right + _margins.right;
			_margins.left = parent_width - el_width;
			if (_margins.left < 0) _margins.left = 0;
		}
		else if (!_css_margins.left.is_predefined() && _css_margins.right.is_predefined()) {
			int el_width = _pos.width + _borders.left + _borders.right + _padding.left + _padding.right + _margins.left;
			_margins.right = parent_width - el_width;
			if (_margins.right < 0) _margins.right = 0;
		}
	}
}

void html_tag::parse_attributes()
{
	for (auto &el : _children)
		el->parse_attributes();
}

void html_tag::get_text(tstring &text)
{
	for (auto &el : _children)
		el->get_text(text);
}

bool html_tag::is_body()  const
{
	return false;
}

void html_tag::set_data(const tchar_t *data)
{
}

void html_tag::get_inline_boxes(position::vector &boxes)
{
	box *old_box = nullptr;
	position pos;
	for (auto &el : _children) {
		if (!el->skip()) {
			if (el->_box) {
				if (el->_box != old_box) {
					if (old_box) {
						if (boxes.empty()) {
							pos.x -= _padding.left + _borders.left;
							pos.width += _padding.left + _borders.left;
						}
						boxes.push_back(pos);
					}
					old_box = el->_box;
					pos.x = el->left() + el->margin_left();
					pos.y = el->top() - _padding.top - _borders.top;
					pos.width = 0;
					pos.height = 0;
				}
				pos.width = el->right() - pos.x - el->margin_right() - el->margin_left();
				pos.height = std::max(pos.height, el->height() + _padding.top + _padding.bottom + _borders.top + _borders.bottom);
			}
			else if (el->get_display() == display_inline) {
				position::vector sub_boxes;
				el->get_inline_boxes(sub_boxes);
				if (!sub_boxes.empty()) {
					sub_boxes.rbegin()->width += el->margin_right();
					if (boxes.empty()) {
						if (_padding.left + _borders.left > 0) {
							position padding_box = (*sub_boxes.begin());
							padding_box.x -= _padding.left + _borders.left + el->margin_left();
							padding_box.width = _padding.left + _borders.left + el->margin_left();
							boxes.push_back(padding_box);
						}
					}
					sub_boxes.rbegin()->width += el->margin_right();
					boxes.insert(boxes.end(), sub_boxes.begin(), sub_boxes.end());
				}
			}
		}
	}
	if (pos.width || pos.height) {
		if (boxes.empty()) {
			pos.x -= _padding.left + _borders.left;
			pos.width += _padding.left + _borders.left;
		}
		boxes.push_back(pos);
	}
	if (!boxes.empty()) {
		if (_padding.right + _borders.right > 0) {
			position padding_box = (*boxes.rbegin());
			padding_box.x += padding_box.width;
			padding_box.width = _padding.right + _borders.right;
			boxes.push_back(padding_box);
		}
	}
}

bool html_tag::on_mouse_over()
{
	bool ret = false;
	element::ptr el = shared_from_this();
	while (el) {
		if (el->set_pseudo_class(_t("hover"), true))
			ret = true;
		el = el->parent();
	}
	return ret;
}

bool html_tag::find_styles_changes( position::vector &redraw_boxes, int x, int y )
{
	if (_display == display_inline_text)
		return false;
	bool ret = false;
	bool apply = false;
	for (used_selector::vector::iterator iter = _used_styles.begin(); iter != _used_styles.end() && !apply; iter++) {
		if ((*iter)->_selector->is_media_valid()) {
			int res = select(*((*iter)->_selector), true);
			if ((res == select_no_match && (*iter)->_used) || (res == select_match && !(*iter)->_used))
				apply = true;
		}
	}

	if (apply) {
		if (_display == display_inline ||  _display == display_table_row) {
			position::vector boxes;
			get_inline_boxes(boxes);
			for(position::vector::iterator pos = boxes.begin(); pos != boxes.end(); pos++) {
				pos->x	+= x;
				pos->y	+= y;
				redraw_boxes.push_back(*pos);
			}
		}
		else {
			position pos = _pos;
			if (_el_position != element_position_fixed) {
				pos.x += x;
				pos.y += y;
			}
			pos += _padding;
			pos += _borders;
			redraw_boxes.push_back(pos);
		}
		ret = true;
		refresh_styles();
		parse_styles();
	}
	for (auto &el : _children) {
		if (!el->skip()) {
			if (_el_position != element_position_fixed) {
				if (el->find_styles_changes(redraw_boxes, x + _pos.x, y + _pos.y))
					ret = true;
			}
			else {
				if (el->find_styles_changes(redraw_boxes, _pos.x, _pos.y))
					ret = true;
			}
		}
	}
	return ret;
}

bool html_tag::on_mouse_leave()
{
	bool ret = false;
	element::ptr el = shared_from_this();
	while (el) {
		if (el->set_pseudo_class(_t("hover"), false))
			ret = true;
		if (el->set_pseudo_class(_t("active"), false))
			ret = true;
		el = el->parent();
	}
	return ret;
}

bool html_tag::on_lbutton_down()
{
	bool ret = false;
	element::ptr el = shared_from_this();
	while (el) {
		if (el->set_pseudo_class(_t("active"), true))
			ret = true;
		el = el->parent();
	}
	return ret;
}

bool html_tag::on_lbutton_up()
{
	bool ret = false;
	element::ptr el = shared_from_this();
	while (el) {
		if (el->set_pseudo_class(_t("active"), false))
			ret = true;
		el = el->parent();
	}
	on_click();
	return ret;
}

void html_tag::on_click()
{
	if (have_parent()) {
		element::ptr el_parent = parent();
		if (el_parent)
			el_parent->on_click();
	}
}

const tchar_t* html_tag::get_cursor()
{
	return get_style_property(_t("cursor"), true, 0);
}

static const int font_size_table[8][7] =
{
	{ 9,    9,     9,     9,    11,    14,    18},
	{ 9,    9,     9,    10,    12,    15,    20},
	{ 9,    9,     9,    11,    13,    17,    22},
	{ 9,    9,    10,    12,    14,    18,    24},
	{ 9,    9,    10,    13,    16,    20,    26},
	{ 9,    9,    11,    14,    17,    21,    28},
	{ 9,   10,    12,    15,    17,    23,    30},
	{ 9,   10,    13,    16,    18,    24,    32}
};


void html_tag::init_font()
{
	// initialize font size
	const tchar_t *str = get_style_property(_t("font-size"), false, 0);

	int doc_font_size = get_document()->container()->GetDefaultFontSize();
	element::ptr el_parent = parent();
	int parent_sz = (el_parent ? el_parent->get_font_size() : doc_font_size);

	if (!str)
		_font_size = parent_sz;
	else {
		_font_size = parent_sz;
		css_length sz;
		sz.fromString(str, font_size_strings);
		if (sz.is_predefined()) {
			int idx_in_table = doc_font_size - 9;
			if (idx_in_table >= 0 && idx_in_table <= 7)
				_font_size = (sz.predef() >= fontSize_xx_small && sz.predef() <= fontSize_xx_large ? font_size_table[idx_in_table][sz.predef()] : doc_font_size);
			else
				switch (sz.predef()) {
				case fontSize_xx_small: _font_size = doc_font_size * 3 / 5; break;
				case fontSize_x_small: _font_size = doc_font_size * 3 / 4; break;
				case fontSize_small: _font_size = doc_font_size * 8 / 9; break;
				case fontSize_large: _font_size = doc_font_size * 6 / 5; break;
				case fontSize_x_large: _font_size = doc_font_size * 3 / 2; break;
				case fontSize_xx_large: _font_size = doc_font_size * 2; break;
				default: _font_size = doc_font_size; break;
			}
		}
		else {
			if (sz.units() == css_units_percentage)
				_font_size = sz.calc_percent(parent_sz);
			else if (sz.units() == css_units_none)
				_font_size = parent_sz;
			else
				_font_size = get_document()->cvt_units(sz, parent_sz);
		}
	}

	// initialize font
	const tchar_t *name = get_style_property(_t("font-family"), true, _t("inherit"));
	const tchar_t *weight = get_style_property(_t("font-weight"), true, _t("normal"));
	const tchar_t *style = get_style_property(_t("font-style"), true, _t("normal"));
	const tchar_t *decoration = get_style_property(_t("text-decoration"), true, _t("none"));
	_font = get_document()->get_font(name, _font_size, weight, style, decoration, &_font_metrics);
}

bool html_tag::is_break() const
{
	return false;
}

void html_tag::set_tagName(const tchar_t *tag)
{
	tstring s_val = tag;
	std::locale lc = std::locale::global(std::locale::classic());
	for(size_t i = 0; i < s_val.length(); i++)
		s_val[i] = std::tolower(s_val[i], lc);
	_tag = s_val;
}

void html_tag::draw_background( uint_ptr hdc, int x, int y, const position* clip )
{
	position pos = _pos;
	pos.x += x;
	pos.y += y;
	position el_pos = pos;
	el_pos += _padding;
	el_pos += _borders;

	if (_display != display_inline && _display != display_table_row)
	{
		if (el_pos.does_intersect(clip)) {
			const background *bg = get_background();
			if (bg) {
				background_paint bg_paint;
				init_background_paint(pos, bg_paint, bg);
				get_document()->container()->DrawBackground(hdc, bg_paint);
			}
			position border_box = pos;
			border_box += _padding;
			border_box += _borders;
			borders bdr = _css_borders;
			bdr.radius = _css_borders.radius.calc_percents(border_box.width, border_box.height);
			get_document()->container()->DrawBorders(hdc, bdr, border_box, !have_parent());
		}
	}
	else {
		const background* bg = get_background();
		position::vector boxes;
		get_inline_boxes(boxes);

		background_paint bg_paint;
		position content_box;
		for (position::vector::iterator box = boxes.begin(); box != boxes.end(); box++) {
			box->x += x;
			box->y += y;
			if (box->does_intersect(clip)) {
				content_box = *box;
				content_box -= _borders;
				content_box -= _padding;
				if (bg)
					init_background_paint(content_box, bg_paint, bg);
				css_borders bdr;
				// set left borders radius for the first box
				if (box == boxes.begin()) {
					bdr.radius.bottom_left_x	= _css_borders.radius.bottom_left_x;
					bdr.radius.bottom_left_y	= _css_borders.radius.bottom_left_y;
					bdr.radius.top_left_x = _css_borders.radius.top_left_x;
					bdr.radius.top_left_y = _css_borders.radius.top_left_y;
				}
				// set right borders radius for the last box
				if (box == boxes.end() - 1) {
					bdr.radius.bottom_right_x = _css_borders.radius.bottom_right_x;
					bdr.radius.bottom_right_y = _css_borders.radius.bottom_right_y;
					bdr.radius.top_right_x = _css_borders.radius.top_right_x;
					bdr.radius.top_right_y = _css_borders.radius.top_right_y;
				}
				bdr.top = _css_borders.top;
				bdr.bottom = _css_borders.bottom;
				if (box == boxes.begin())
					bdr.left = _css_borders.left;
				if (box == boxes.end() - 1)
					bdr.right = _css_borders.right;
				if (bg) {
					bg_paint.border_radius = bdr.radius.calc_percents(bg_paint.border_box.width, bg_paint.border_box.width);
					get_document()->container()->DrawBackground(hdc, bg_paint);
				}
				borders b = bdr;
				b.radius = bdr.radius.calc_percents(box->width, box->height);
				get_document()->container()->DrawBorders(hdc, b, *box, false);
			}
		}
	}
}

int html_tag::render_inline(const element::ptr &container, int max_width)
{
	white_space ws = get_white_space();
	bool skip_spaces = (ws == white_space_normal || ws == white_space_nowrap || ws == white_space_pre_line ? true : false);
	bool was_space = false;

	int ret_width = 0;
	int rw = 0;
	for (auto &el : _children)
	{
		// skip spaces to make rendering a bit faster
		if (skip_spaces) {
			if (el->is_white_space()) {
				if (was_space) {
					el->skip(true);
					continue;
				}
				else
					was_space = true;
			}
			else
				was_space = false;
		}
		rw = container->place_element( el, max_width );
		if (rw > ret_width)
			ret_width = rw;
	}
	return ret_width;
}

int html_tag::place_element(const element::ptr &el, int max_width)
{
	if (el->get_display() == display_none) return 0;
	if (el->get_display() == display_inline)
		return el->render_inline(shared_from_this(), max_width);

	element_position el_position = el->get_element_position();
	if (el_position == element_position_absolute || el_position == element_position_fixed) {
		int line_top = 0;
		if (!_boxes.empty()) {
			if (_boxes.back()->get_type() == box_line) {
				line_top = _boxes.back()->top();
				if (!_boxes.back()->is_empty())
					line_top += line_height();
			} else
				line_top = _boxes.back()->bottom();
		}
		el->render(0, line_top, max_width);
		el->_pos.x	+= el->content_margins_left();
		el->_pos.y	+= el->content_margins_top();
		return 0;
	}

	int ret_width = 0;
	switch (el->get_float()) {
	case float_left: {
		int line_top = 0;
		if (!_boxes.empty())
			line_top = (_boxes.back()->get_type() == box_line ? _boxes.back()->top() : _boxes.back()->bottom());
		line_top = get_cleared_top(el, line_top);
		int line_left = 0;
		int line_right = max_width;
		get_line_left_right(line_top, max_width, line_left, line_right);

		el->render(line_left, line_top, line_right);
		if (el->right() > line_right) {
			int new_top = find_next_line_top(el->top(), el->width(), max_width);
			el->_pos.x = get_line_left(new_top) + el->content_margins_left();
			el->_pos.y = new_top + el->content_margins_top();
		}
		add_float(el, 0, 0);
		ret_width = fix_line_width(max_width, float_left);
		if (!ret_width)
			ret_width = el->right();
		break; }
	case float_right: {
		int line_top = 0;
		if (!_boxes.empty())
			line_top = (_boxes.back()->get_type() == box_line ? _boxes.back()->top() : _boxes.back()->bottom());
		line_top = get_cleared_top(el, line_top);
		int line_left = 0;
		int line_right = max_width;
		get_line_left_right(line_top, max_width, line_left, line_right);

		el->render(0, line_top, line_right);

		if (line_left + el->width() > line_right) {
			int new_top = find_next_line_top(el->top(), el->width(), max_width);
			el->_pos.x = get_line_right(new_top, max_width) - el->width() + el->content_margins_left();
			el->_pos.y = new_top + el->content_margins_top();
		} else
			el->_pos.x = line_right - el->width() + el->content_margins_left();
		add_float(el, 0, 0);
		ret_width = fix_line_width(max_width, float_right);

		if (!ret_width) {
			line_left = 0;
			line_right = max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);
			ret_width = ret_width + (max_width - line_right);
		}
		break; }
	default:
		{
			line_context line_ctx;
			line_ctx.top = 0;
			if (!_boxes.empty())
				line_ctx.top = _boxes.back()->top();
			line_ctx.left = 0;
			line_ctx.right = max_width;
			line_ctx.fix_top();
			get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);

			switch (el->get_display()) {
			case display_inline_block:
				ret_width = el->render(line_ctx.left, line_ctx.top, line_ctx.right);
				break;
			case display_block:		
				if (el->is_replaced() || el->is_floats_holder()) {
					element::ptr el_parent = el->parent();
					el->_pos.width = el->get_css_width().calc_percent(line_ctx.right - line_ctx.left);
					el->_pos.height = el->get_css_height().calc_percent(el_parent ? el_parent->_pos.height : 0);
				}
				el->calc_outlines(line_ctx.right - line_ctx.left);
				break;
			case display_inline_text: {
				size sz;
				el->get_content_size(sz, line_ctx.right);
				el->_pos = sz;
				break; }
			default:
				ret_width = 0;
				break;
			}

			bool add_box = true;
			if (!_boxes.empty())
				if(_boxes.back()->can_hold(el, _white_space))
					add_box = false;
			if (add_box)
				new_box(el, max_width, line_ctx);
			else if (!_boxes.empty())
				line_ctx.top = _boxes.back()->top();

			if (line_ctx.top != line_ctx.calculatedTop) {
				line_ctx.left = 0;
				line_ctx.right = max_width;
				line_ctx.fix_top();
				get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);
			}

			if (!el->is_inline_box()) {
				if (_boxes.size() == 1)
				{
					if (collapse_top_margin()) {
						int shift = el->margin_top();
						if (shift >= 0) {
							line_ctx.top -= shift;
							_boxes.back()->y_shift(-shift);
						}
					}
				}
				else {
					int prev_margin = _boxes[_boxes.size() - 2]->bottom_margin();
					int shift = (prev_margin > el->margin_top() ? el->margin_top() : prev_margin);
					if (shift >= 0) {
						line_ctx.top -= shift;
						_boxes.back()->y_shift(-shift);
					}
				}
			}

			switch(el->get_display()) {
			case display_table:
			case display_list_item:
				ret_width = el->render(line_ctx.left, line_ctx.top, line_ctx.width());
				break;
			case display_block:
			case display_table_cell:
			case display_table_caption:
			case display_table_row:
				if (el->is_replaced() || el->is_floats_holder())
					ret_width = el->render(line_ctx.left, line_ctx.top, line_ctx.width()) + line_ctx.left + (max_width - line_ctx.right);
				else
					ret_width = el->render(0, line_ctx.top, max_width);
				break;
			default:
				ret_width = 0;
				break;
			}

			_boxes.back()->add_element(el);

			if (el->is_inline_box() && !el->skip())
				ret_width = el->right() + (max_width - line_ctx.right);
		}
		break;
	}
	return ret_width;
}

bool html_tag::set_pseudo_class(const tchar_t *pclass, bool add)
{
	bool ret = false;
	if (add) {
		if (std::find(_pseudo_classes.begin(), _pseudo_classes.end(), pclass) == _pseudo_classes.end()) {
			_pseudo_classes.push_back(pclass);
			ret = true;
		}
	}
	else {
		string_vector::iterator pi = std::find(_pseudo_classes.begin(), _pseudo_classes.end(), pclass);
		if (pi != _pseudo_classes.end()) {
			_pseudo_classes.erase(pi);
			ret = true;
		}
	}
	return ret;
}

bool html_tag::set_class(const tchar_t *pclass, bool add)
{
	string_vector classes;
	bool changed = false;
	split_string(pclass, classes, _t(" "));

	if (add) {
		for (auto &_class : classes)
			if (std::find(_class_values.begin(), _class_values.end(), _class) == _class_values.end()) {
				_class_values.push_back( std::move( _class ) );
				changed = true;
			}
	}
	else {
		for (const auto &_class : classes) {
			auto end = std::remove(_class_values.begin(), _class_values.end(), _class);
			if (end != _class_values.end()) {
				_class_values.erase(end, _class_values.end());
				changed = true;
			}
		}
	}

	if (changed) {
		tstring class_string;
		join_string(class_string, _class_values, _t(" "));
		set_attr(_t("class"), class_string.c_str());
		return true;
	}
	return false;
}

int html_tag::line_height() const
{
	return _line_height;
}

bool html_tag::is_replaced() const
{
	return false;
}

int html_tag::finish_last_box(bool end_of_render)
{
	int line_top = 0;
	if (!_boxes.empty()) {
		_boxes.back()->finish(end_of_render);
		if (_boxes.back()->is_empty()) {
			line_top = _boxes.back()->top();
			_boxes.pop_back();
		}
		if (!_boxes.empty())
			line_top = _boxes.back()->bottom();
	}
	return line_top;
}

int html_tag::new_box(const element::ptr &el, int max_width, line_context &line_ctx)
{
	line_ctx.top = get_cleared_top(el, finish_last_box());
	line_ctx.left = 0;
	line_ctx.right = max_width;
	line_ctx.fix_top();
	get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);

	if (el->is_inline_box() || el->is_floats_holder()) {
		if (el->width() > line_ctx.right - line_ctx.left) {
			line_ctx.top = find_next_line_top(line_ctx.top, el->width(), max_width);
			line_ctx.left = 0;
			line_ctx.right = max_width;
			line_ctx.fix_top();
			get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);
		}
	}

	int first_line_margin = 0;
	if (_boxes.empty() && _list_style_type != list_style_type_none && _list_style_position == list_style_position_inside) {
		int sz_font = get_font_size();
		first_line_margin = sz_font;
	}

	if (el->is_inline_box()) {
		int text_indent = 0;
		if (_css_text_indent.val() != 0) {
			bool line_box_found = false;
			for (box::vector::iterator iter = _boxes.begin(); iter != _boxes.end(); iter++) {
				if ((*iter)->get_type() == box_line) {
					line_box_found = true;
					break;
				}
			}
			if (!line_box_found)
				text_indent = _css_text_indent.calc_percent(max_width);
		}

		font_metrics fm;
		get_font(&fm);
		_boxes.emplace_back(std::unique_ptr<line_box>(new line_box(line_ctx.top, line_ctx.left + first_line_margin + text_indent, line_ctx.right, line_height(), fm, _text_align)));
	}
	else
		_boxes.emplace_back(std::unique_ptr<block_box>(new block_box(line_ctx.top, line_ctx.left, line_ctx.right)));
	return line_ctx.top;
}

int html_tag::get_cleared_top(const element::ptr &el, int line_top) const
{
	switch (el->get_clear()) {
	case clear_left: {
		int fh = get_left_floats_height();
		if (fh && fh > line_top)
			line_top = fh;
		break; }
	case clear_right: {
		int fh = get_right_floats_height();
		if (fh && fh > line_top)
			line_top = fh;
		break; }
	case clear_both: {
		int fh = get_floats_height();
		if (fh && fh > line_top)
			line_top = fh;
		break; }
	default:
		if (el->get_float() != float_none) {
			int fh = get_floats_height(el->get_float());
			if (fh && fh > line_top)
				line_top = fh;
		}
		break;
	}
	return line_top;
}

style_display html_tag::get_display() const
{
	return _display;
}

element_float html_tag::get_float() const
{
	return _float;
}

bool html_tag::is_floats_holder() const
{
	return (_display == display_inline_block || 
		_display == display_table_cell || 
		!have_parent() ||
		is_body() || 
		_float != float_none ||
		_el_position == element_position_absolute ||
		_el_position == element_position_fixed ||
		_overflow > overflow_visible);
}

bool html_tag::is_first_child_inline(const element::ptr &el) const
{
	if (!_children.empty()) {
		for (const auto &this_el : _children)
			if (!this_el->is_white_space()) {
				if (el == this_el)
					return true;
				if (this_el->get_display() == display_inline) {
					if (this_el->have_inline_child())
						return false;
				}
				else
					return false;
			}
	}
	return false;
}

bool html_tag::is_last_child_inline(const element::ptr &el)
{
	if (!_children.empty()) {
		for (const auto &this_el : _children)
			if (!this_el->is_white_space()) {
				if (el == this_el)
					return true;
				if (this_el->get_display() == display_inline) {
					if (this_el->have_inline_child())
						return false;
				}
				else
					return false;
			}
	}
	return false;
}

white_space html_tag::get_white_space() const
{
	return _white_space;
}

vertical_align html_tag::get_vertical_align() const
{
	return _vertical_align;
}

css_length html_tag::get_css_left() const
{
	return _css_offsets.left;
}

css_length html_tag::get_css_right() const
{
	return _css_offsets.right;
}

css_length html_tag::get_css_top() const
{
	return _css_offsets.top;
}

css_length html_tag::get_css_bottom() const
{
	return _css_offsets.bottom;
}

css_offsets html_tag::get_css_offsets() const
{
	return _css_offsets;
}

element_clear html_tag::get_clear() const
{
	return _clear;
}

css_length html_tag::get_css_width() const
{
	return _css_width;
}

css_length html_tag::get_css_height() const
{
	return _css_height;
}

size_t html_tag::get_children_count() const
{
	return _children.size();
}

element::ptr html_tag::get_child(int idx) const
{
	return _children[idx];
}

void html_tag::set_css_width(css_length &w)
{
	_css_width = w;
}

void html_tag::apply_vertical_align()
{
	if (!_boxes.empty()) {
		int add = 0;
		int content_height	= _boxes.back()->bottom();
		if (_pos.height > content_height)
			switch(_vertical_align) {
			case va_middle: add = (_pos.height - content_height) / 2; break;
			case va_bottom: add = _pos.height - content_height; break;
			default: add = 0; break;
		}
		if (add)
			for(size_t i = 0; i < _boxes.size(); i++)
				_boxes[i]->y_shift(add);
	}
}

element_position html_tag::get_element_position(css_offsets *offsets) const
{
	if (offsets && _el_position != element_position_static)
		*offsets = _css_offsets;
	return _el_position;
}

void html_tag::init_background_paint(position pos, background_paint &bg_paint, const background *bg)
{
	if (!bg) return;

	bg_paint = *bg;
	position content_box = pos;
	position padding_box = pos;
	padding_box += _padding;
	position border_box = padding_box;
	border_box += _borders;

	switch (bg->_clip) {
	case background_box_padding: bg_paint.clip_box = padding_box; break;
	case background_box_content: bg_paint.clip_box = content_box; break;
	default: bg_paint.clip_box = border_box; break;
	}

	switch (bg->_origin) {
	case background_box_border: bg_paint.origin_box = border_box; break;
	case background_box_content: bg_paint.origin_box = content_box; break;
	default: bg_paint.origin_box = padding_box; break;
	}

	if (!bg_paint.image.empty()) {
		get_document()->container()->GetImageSize(bg_paint.image.c_str(), bg_paint.baseurl.c_str(), bg_paint.image_size);
		if (bg_paint.image_size.width && bg_paint.image_size.height) {
			size img_new_sz = bg_paint.image_size;
			double img_ar_width = (double)bg_paint.image_size.width / (double)bg_paint.image_size.height;
			double img_ar_height = (double)bg_paint.image_size.height / (double)bg_paint.image_size.width;
			if (bg->_position.width.is_predefined()) {
				switch (bg->_position.width.predef()) {
				case background_size_contain:
					if ((int)((double)bg_paint.origin_box.width * img_ar_height) <= bg_paint.origin_box.height) {
						img_new_sz.width = bg_paint.origin_box.width;
						img_new_sz.height = (int)((double)bg_paint.origin_box.width * img_ar_height);
					}
					else {
						img_new_sz.height = bg_paint.origin_box.height;
						img_new_sz.width = (int)((double)bg_paint.origin_box.height * img_ar_width);
					}
					break;
				case background_size_cover:
					if ((int)((double)bg_paint.origin_box.width * img_ar_height) >= bg_paint.origin_box.height) {
						img_new_sz.width = bg_paint.origin_box.width;
						img_new_sz.height = (int)((double)bg_paint.origin_box.width * img_ar_height);
					}
					else {
						img_new_sz.height = bg_paint.origin_box.height;
						img_new_sz.width = (int)((double)bg_paint.origin_box.height * img_ar_width);
					}
					break;
				case background_size_auto:
					if (!bg->_position.height.is_predefined()) {
						img_new_sz.height = bg->_position.height.calc_percent(bg_paint.origin_box.height);
						img_new_sz.width = (int)((double)img_new_sz.height * img_ar_width);
					}
					break;
				}
			}
			else {
				img_new_sz.width = bg->_position.width.calc_percent(bg_paint.origin_box.width);
				if (bg->_position.height.is_predefined())
					img_new_sz.height = (int)((double)img_new_sz.width * img_ar_height);
				else
					img_new_sz.height = bg->_position.height.calc_percent(bg_paint.origin_box.height);
			}
			bg_paint.image_size = img_new_sz;
			bg_paint.position_x = bg_paint.origin_box.x + (int)bg->_position.x.calc_percent(bg_paint.origin_box.width - bg_paint.image_size.width);
			bg_paint.position_y = bg_paint.origin_box.y + (int)bg->_position.y.calc_percent(bg_paint.origin_box.height - bg_paint.image_size.height);
		}
	}
	bg_paint.border_radius = _css_borders.radius.calc_percents(border_box.width, border_box.height);
	bg_paint.border_box = border_box;
	bg_paint.is_root = !have_parent();
}

visibility html_tag::get_visibility() const
{
	return _visibility;
}

void html_tag::draw_list_marker(uint_ptr hdc, const position &pos)
{
	list_marker lm;
	const tchar_t *list_image = get_style_property(_t("list-style-image"), true, 0);
	size img_size;
	if (list_image) {
		css::parse_css_url(list_image, lm.image);
		lm.baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
		get_document()->container()->GetImageSize(lm.image.c_str(), lm.baseurl, img_size);
	}
	else
		lm.baseurl = nullptr;

	int ln_height = line_height();
	int sz_font = get_font_size();
	lm.pos.x = pos.x;
	lm.pos.width = sz_font - sz_font * 2 / 3;
	lm.pos.height = sz_font - sz_font * 2 / 3;
	lm.pos.y = pos.y + ln_height / 2 - lm.pos.height / 2;

	if (img_size.width && img_size.height) {
		if (lm.pos.y + img_size.height > pos.y + pos.height)
			lm.pos.y = pos.y + pos.height - img_size.height;
		if (img_size.width > lm.pos.width)
			lm.pos.x -= img_size.width - lm.pos.width;
		lm.pos.width	= img_size.width;
		lm.pos.height	= img_size.height;
	}
	if (_list_style_position == list_style_position_outside)
		lm.pos.x -= sz_font;
	lm.color = get_color(_t("color"), true, web_color(0, 0, 0));
	lm.marker_type = _list_style_type;
	get_document()->container()->DrawListMarker(hdc, lm);
}

void html_tag::draw_children(uint_ptr hdc, int x, int y, const position *clip, draw_flag flag, int zindex)
{
	if (_display == display_table || _display == display_inline_table)
		draw_children_table(hdc, x, y, clip, flag, zindex);
	else
		draw_children_box(hdc, x, y, clip, flag, zindex);
}

bool html_tag::fetch_positioned()
{
	bool ret = false;
	_positioned.clear();
	element_position el_pos;
	for (auto &el : _children) {
		el_pos = el->get_element_position();
		if (el_pos != element_position_static)
			add_positioned(el);
		if (!ret && (el_pos == element_position_absolute || el_pos == element_position_fixed))
			ret = true;
		if (el->fetch_positioned())
			ret = true;
	}
	return ret;
}

int html_tag::get_zindex() const
{
	return _z_index;
}

void html_tag::render_positioned(render_type rt)
{
	position wnd_position;
	get_document()->container()->GetClientRect(wnd_position);

	element_position el_position;
	bool process;
	for (auto &el : _positioned) {
		el_position = el->get_element_position();

		process = false;
		if (el->get_display() != display_none) {
			if (el_position == element_position_absolute) {
				if (rt != render_fixed_only)
					process = true;
			}
			else if(el_position == element_position_fixed) {
				if (rt != render_no_fixed)
					process = true;
			}
		}

		if (process) {
			int parent_height = 0;
			int parent_width = 0;
			int client_x = 0;
			int client_y = 0;
			if (el_position == element_position_fixed) {
				parent_height = wnd_position.height;
				parent_width = wnd_position.width;
				client_x = wnd_position.left();
				client_y = wnd_position.top();
			}
			else {
				element::ptr el_parent = el->parent();
				if (el_parent) {
					parent_height = el_parent->height();
					parent_width = el_parent->width();
				}
			}

			css_length css_left = el->get_css_left();
			css_length css_right = el->get_css_right();
			css_length css_top = el->get_css_top();
			css_length css_bottom = el->get_css_bottom();
			bool need_render = false;
			css_length el_w = el->get_css_width();
			css_length el_h = el->get_css_height();

			int new_width = -1;
			int new_height = -1;
			if (el_w.units() == css_units_percentage && parent_width) {
				new_width = el_w.calc_percent(parent_width);
				if (el->_pos.width != new_width) {
					need_render = true;
					el->_pos.width = new_width;
				}
			}

			if (el_h.units() == css_units_percentage && parent_height) {
				new_height = el_h.calc_percent(parent_height);
				if (el->_pos.height != new_height) {
					need_render = true;
					el->_pos.height = new_height;
				}
			}

			bool cvt_x = false;
			bool cvt_y = false;
			if (el_position == element_position_fixed) {
				if (!css_left.is_predefined() || !css_right.is_predefined()) {
					if (!css_left.is_predefined() && css_right.is_predefined())
						el->_pos.x = css_left.calc_percent(parent_width) + el->content_margins_left();
					else if (css_left.is_predefined() && !css_right.is_predefined())
						el->_pos.x = parent_width - css_right.calc_percent(parent_width) - el->_pos.width - el->content_margins_right();
					else {
						el->_pos.x = css_left.calc_percent(parent_width) + el->content_margins_left();
						el->_pos.width = parent_width - css_left.calc_percent(parent_width) - css_right.calc_percent(parent_width) - (el->content_margins_left() + el->content_margins_right());
						need_render = true;
					}
				}
				if (!css_top.is_predefined() || !css_bottom.is_predefined()) {
					if (!css_top.is_predefined() && css_bottom.is_predefined())
						el->_pos.y = css_top.calc_percent(parent_height) + el->content_margins_top();
					else if (css_top.is_predefined() && !css_bottom.is_predefined())
						el->_pos.y = parent_height - css_bottom.calc_percent(parent_height) - el->_pos.height - el->content_margins_bottom();
					else {
						el->_pos.y = css_top.calc_percent(parent_height) + el->content_margins_top();
						el->_pos.height	= parent_height - css_top.calc_percent(parent_height) - css_bottom.calc_percent(parent_height) - (el->content_margins_top() + el->content_margins_bottom());
						need_render = true;
					}
				}
			}
			else  {
				if (!css_left.is_predefined() || !css_right.is_predefined()) {
					if (!css_left.is_predefined() && css_right.is_predefined())
						el->_pos.x = css_left.calc_percent(parent_width) + el->content_margins_left() - _padding.left;
					else if (css_left.is_predefined() && !css_right.is_predefined())
						el->_pos.x = _pos.width + _padding.right - css_right.calc_percent(parent_width) - el->_pos.width - el->content_margins_right();
					else {
						el->_pos.x = css_left.calc_percent(parent_width) + el->content_margins_left() - _padding.left;
						el->_pos.width = _pos.width + _padding.left + _padding.right - css_left.calc_percent(parent_width) - css_right.calc_percent(parent_width) - (el->content_margins_left() + el->content_margins_right());
						if (new_width != -1) {
							el->_pos.x += (el->_pos.width - new_width) / 2;
							el->_pos.width = new_width;
						}
						need_render = true;
					}
					cvt_x = true;
				}

				if (!css_top.is_predefined() || !css_bottom.is_predefined()) {
					if (!css_top.is_predefined() && css_bottom.is_predefined())
						el->_pos.y = css_top.calc_percent(parent_height) + el->content_margins_top() - _padding.top;
					else if(css_top.is_predefined() && !css_bottom.is_predefined())
						el->_pos.y = _pos.height + _padding.bottom - css_bottom.calc_percent(parent_height) - el->_pos.height - el->content_margins_bottom();
					else {
						el->_pos.y = css_top.calc_percent(parent_height) + el->content_margins_top() - _padding.top;
						el->_pos.height	= _pos.height + _padding.top + _padding.bottom - css_top.calc_percent(parent_height) - css_bottom.calc_percent(parent_height) - (el->content_margins_top() + el->content_margins_bottom());
						if (new_height != -1) {
							el->_pos.y += (el->_pos.height - new_height) / 2;
							el->_pos.height = new_height;
						}
						need_render = true;
					}
					cvt_y = true;
				}
			}

			if (cvt_x || cvt_y) {
				int offset_x = 0;
				int offset_y = 0;
				element::ptr cur_el = el->parent();
				element::ptr this_el = shared_from_this();
				while (cur_el && cur_el != this_el) {
					offset_x += cur_el->_pos.x;
					offset_y += cur_el->_pos.y;
					cur_el = cur_el->parent();
				}
				if (cvt_x) el->_pos.x -= offset_x;
				if (cvt_y) el->_pos.y -= offset_y;
			}

			if (need_render) {
				position pos = el->_pos;
				el->render(el->left(), el->top(), el->width(), true);
				el->_pos = pos;
			}
			if (el_position == element_position_fixed) {
				position fixed_pos;
				el->get_redraw_box(fixed_pos);
				get_document()->add_fixed_box(fixed_pos);
			}
		}
		el->render_positioned();
	}

	if (!_positioned.empty())
		std::stable_sort(_positioned.begin(), _positioned.end(), [](const element::ptr &_Left, const element::ptr &_Right) { return (_Left->get_zindex() < _Right->get_zindex()); });
}

void html_tag::draw_stacking_context(uint_ptr hdc, int x, int y, const position* clip, bool with_positioned)
{
	if (!is_visible()) return;

	std::map<int, bool> zindexes;
	if (with_positioned) {
		for (elements_vector::iterator i = _positioned.begin(); i != _positioned.end(); i++)
			zindexes[(*i)->get_zindex()];

		for (std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end(); idx++)
			if(idx->first < 0)
				draw_children(hdc, x, y, clip, draw_positioned, idx->first);
	}
	draw_children(hdc, x, y, clip, draw_block, 0);
	draw_children(hdc, x, y, clip, draw_floats, 0);
	draw_children(hdc, x, y, clip, draw_inlines, 0);
	if (with_positioned) {
		for (std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end(); idx++)
			if (idx->first == 0)
				draw_children(hdc, x, y, clip, draw_positioned, idx->first);
		for (std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end(); idx++)
			if (idx->first > 0)
				draw_children(hdc, x, y, clip, draw_positioned, idx->first);
	}
}

overflow html_tag::get_overflow() const
{
	return _overflow;
}

bool html_tag::is_nth_child(const element::ptr &el, int num, int off, bool of_type) const
{
	int idx = 1;
	for (const auto &child : _children) {
		if (child->get_display() != display_inline_text) {
			if (!of_type || (of_type && !t_strcmp(el->get_tagName(), child->get_tagName()))) {
				if (el == child) {
					if (num != 0) {
						if ((idx - off) >= 0 && (idx - off) % num == 0)
							return true;
					}
					else if (idx == off)
						return true;
					return false;
				}
				idx++;
			}
			if (el == child) break;
		}
	}
	return false;
}

bool html_tag::is_nth_last_child(const element::ptr &el, int num, int off, bool of_type) const
{
	int idx = 1;
	for (elements_vector::const_reverse_iterator child = _children.rbegin(); child != _children.rend(); child++) {
		if ((*child)->get_display() != display_inline_text) {
			if (!of_type || (of_type && !t_strcmp(el->get_tagName(), (*child)->get_tagName()))) {
				if (el == (*child)) {
					if (num != 0) {
						if ((idx - off) >= 0 && (idx - off) % num == 0)
							return true;
					}
					else if (idx == off)
						return true;
					return false;
				}
				idx++;
			}
			if (el == (*child)) break;
		}
	}
	return false;
}

void html_tag::parse_nth_child_params(tstring param, int &num, int &off)
{
	if (param == _t("odd")) {
		num = 2;
		off = 1;
	}
	else if (param == _t("even")) {
		num = 2;
		off = 0;
	}
	else {
		string_vector tokens;
		split_string(param, tokens, _t(" n"), _t("n"));
		tstring s_num;
		tstring s_off;
		tstring s_int;
		for (string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++) {
			if ((*tok) == _t("n")) {
				s_num = s_int;
				s_int.clear();
			}
			else
				s_int += (*tok);
		}
		s_off = s_int;
		num = t_atoi(s_num.c_str());
		off = t_atoi(s_off.c_str());
	}
}

void html_tag::calc_document_size(size &sz, int x /*= 0*/, int y /*= 0*/)
{
	if (is_visible() && _el_position != element_position_fixed) {
		element::calc_document_size(sz, x, y);
		if (_overflow == overflow_visible)
			for(auto &el : _children)
				el->calc_document_size(sz, x + _pos.x, y + _pos.y);
		// root element (<html>) must to cover entire window
		if (!have_parent()) {
			position client_pos;
			get_document()->container()->GetClientRect(client_pos);
			_pos.height = std::max(sz.height, client_pos.height) - content_margins_top() - content_margins_bottom();
			_pos.width = std::max(sz.width, client_pos.width) - content_margins_left() - content_margins_right();
		}
	}
}

void html_tag::get_redraw_box(position &pos, int x /*= 0*/, int y /*= 0*/)
{
	if (is_visible()) {
		element::get_redraw_box(pos, x, y);
		if (_overflow == overflow_visible)
			for (auto &el : _children)
				if(el->get_element_position() != element_position_fixed)
					el->get_redraw_box(pos, x + _pos.x, y + _pos.y);
	}
}

element::ptr html_tag::find_adjacent_sibling(const element::ptr &el, const css_selector &selector, bool apply_pseudo /*= true*/, bool* is_pseudo /*= 0*/)
{
	element::ptr ret = nullptr;
	for (auto &e : _children) {
		if (e->get_display() != display_inline_text) {
			if (e == el) {
				if (ret) {
					int res = ret->select(selector, apply_pseudo);
					if (res != select_no_match) {
						if (is_pseudo)
							*is_pseudo = (res & select_match_pseudo_class ? true : false);
						return ret;
					}
				}
				return nullptr;
			}
			else
				ret = e;
		}
	}
	return nullptr;
}

element::ptr html_tag::find_sibling(const element::ptr &el, const css_selector &selector, bool apply_pseudo /*= true*/, bool* is_pseudo /*= 0*/)
{
	element::ptr ret = nullptr;
	for (auto &e : _children) {
		if (e->get_display() != display_inline_text) {
			if (e == el)
				return ret;
			else if (!ret) {
				int res = e->select(selector, apply_pseudo);
				if (res != select_no_match) {
					if (is_pseudo)
						*is_pseudo = (res & select_match_pseudo_class ? true : false);
					ret = e;
				}
			}
		}
	}
	return nullptr;
}

bool html_tag::is_only_child(const element::ptr &el, bool of_type) const
{
	int child_count = 0;
	for (const auto &child : _children) {
		if (child->get_display() != display_inline_text) {
			if (!of_type || (of_type && !t_strcmp(el->get_tagName(), child->get_tagName())))
				child_count++;
			if (child_count > 1) break;
		}
	}
	return !(child_count > 1);
}

void html_tag::update_floats(int dy, const element::ptr &parent)
{
	if (is_floats_holder()) {
		bool reset_cache = false;
		for (floated_box::vector::reverse_iterator fb = _floats_left.rbegin(); fb != _floats_left.rend(); fb++) {
			if (fb->el->is_ancestor(parent)) {
				reset_cache	= true;
				fb->pos.y += dy;
			}
		}
		if (reset_cache)
			_cahe_line_left.invalidate();
		reset_cache = false;
		for (floated_box::vector::reverse_iterator fb = _floats_right.rbegin(); fb != _floats_right.rend(); fb++) {
			if (fb->el->is_ancestor(parent)) {
				reset_cache	= true;
				fb->pos.y += dy;
			}
		}
		if (reset_cache)
			_cahe_line_right.invalidate();
	}
	else {
		element::ptr el_parent = this->parent();
		if (el_parent)
			el_parent->update_floats(dy, parent);
	}
}

void html_tag::remove_before_after()
{
	if (!_children.empty())
		if (!t_strcmp(_children.front()->get_tagName(), _t("::before")))
			_children.erase(_children.begin());
	if (!_children.empty())
		if (!t_strcmp(_children.back()->get_tagName(), _t("::after")))
			_children.erase(_children.end() - 1);
}

element::ptr html_tag::get_element_before()
{
	if (!_children.empty())
		if (!t_strcmp(_children.front()->get_tagName(), _t("::before")))
			return _children.front();
	element::ptr el = std::make_shared<el_before>(get_document());
	el->parent(shared_from_this());
	_children.insert(_children.begin(), el);
	return el;
}

element::ptr html_tag::get_element_after()
{
	if (!_children.empty())
		if (!t_strcmp(_children.back()->get_tagName(), _t("::after")))
			return _children.back();
	element::ptr el = std::make_shared<el_after>(get_document());
	appendChild(el);
	return el;
}

void html_tag::add_style( const style &st )
{
	_style.combine(st);
}

bool html_tag::have_inline_child() const
{
	if (!_children.empty())
		for (const auto &el : _children)
			if(!el->is_white_space())
				return true;
	return false;
}

void html_tag::refresh_styles()
{
	remove_before_after();
	for (auto &el : _children)
		if (el->get_display() != display_inline_text)
			el->refresh_styles();
	_style.clear();
	for (auto &usel : _used_styles) {
		usel->_used = false;
		if (usel->_selector->is_media_valid()) {
			int apply = select(*usel->_selector, false);
			if (apply != select_no_match) {
				if (apply & select_match_pseudo_class) {
					if (select(*usel->_selector, true)) {
						if (apply & select_match_with_after) {
							element::ptr el = get_element_after();
							if (el)
								el->add_style(*usel->_selector->_style);
						}
						else if (apply & select_match_with_before) {
							element::ptr el = get_element_before();
							if (el)
								el->add_style(*usel->_selector->_style);
						}
						else {
							add_style(*usel->_selector->_style);
							usel->_used = true;
						}
					}
				}
				else if (apply & select_match_with_after) {
					element::ptr el = get_element_after();
					if (el)
						el->add_style(*usel->_selector->_style);
				}
				else if (apply & select_match_with_before) {
					element::ptr el = get_element_before();
					if (el)
						el->add_style(*usel->_selector->_style);
				}
				else {
					add_style(*usel->_selector->_style);
					usel->_used = true;
				}
			}
		}
	}
}

element::ptr html_tag::get_child_by_point(int x, int y, int client_x, int client_y, draw_flag flag, int zindex)
{
	element::ptr ret = nullptr;
	if (_overflow > overflow_visible)
		if(!_pos.is_point_inside(x, y))
			return ret;
	position pos = _pos;
	pos.x = x - pos.x;
	pos.y = y - pos.y;
	for (elements_vector::reverse_iterator i = _children.rbegin(); i != _children.rend() && !ret; i++) {
		element::ptr el = (*i);
		if (el->is_visible() && el->get_display() != display_inline_text) {
			switch (flag) {
			case draw_positioned:
				if (el->is_positioned() && el->get_zindex() == zindex) {
					if (el->get_element_position() == element_position_fixed) {
						ret = el->get_element_by_point(client_x, client_y, client_x, client_y);
						if (!ret && (*i)->is_point_inside(client_x, client_y))
							ret = (*i);
					}
					else {
						ret = el->get_element_by_point(pos.x, pos.y, client_x, client_y);
						if (!ret && (*i)->is_point_inside(pos.x, pos.y))
							ret = (*i);
					}
					el = nullptr;
				}
				break;
			case draw_block:
				if (!el->is_inline_box() && el->get_float() == float_none && !el->is_positioned())
					if (el->is_point_inside(pos.x, pos.y))
						ret = el;
				break;
			case draw_floats:
				if (el->get_float() != float_none && !el->is_positioned()) {
					ret = el->get_element_by_point(pos.x, pos.y, client_x, client_y);
					if (!ret && (*i)->is_point_inside(pos.x, pos.y))
						ret = (*i);
					el = nullptr;
				}
				break;
			case draw_inlines:
				if (el->is_inline_box() && el->get_float() == float_none && !el->is_positioned()) {
					if (el->get_display() == display_inline_block) {
						ret = el->get_element_by_point(pos.x, pos.y, client_x, client_y);
						el = nullptr;
					}
					if (!ret && (*i)->is_point_inside(pos.x, pos.y))
						ret = (*i);
				}
				break;
			default:
				break;
			}

			if (el && !el->is_positioned()) {
				if (flag == draw_positioned) {
					element::ptr child = el->get_child_by_point(pos.x, pos.y, client_x, client_y, flag, zindex);
					if (child)
						ret = child;
				}
				else {
					if (el->get_float() == float_none && el->get_display() != display_inline_block) {
						element::ptr child = el->get_child_by_point(pos.x, pos.y, client_x, client_y, flag, zindex);
						if (child)
							ret = child;
					}
				}
			}
		}
	}
	return ret;
}

element::ptr html_tag::get_element_by_point(int x, int y, int client_x, int client_y)
{
	if (!is_visible()) return nullptr;

	element::ptr ret;
	std::map<int, bool> zindexes;
	for (elements_vector::iterator i = _positioned.begin(); i != _positioned.end(); i++)
		zindexes[(*i)->get_zindex()];

	for (std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end() && !ret; idx++)
		if (idx->first > 0)
			ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, idx->first);
	if (ret) return ret;

	for (std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end() && !ret; idx++)
		if (idx->first == 0)
			ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, idx->first);
	if (ret) return ret;

	ret = get_child_by_point(x, y, client_x, client_y, draw_inlines, 0);
	if (ret) return ret;

	ret = get_child_by_point(x, y, client_x, client_y, draw_floats, 0);
	if (ret) return ret;

	ret = get_child_by_point(x, y, client_x, client_y, draw_block, 0);
	if (ret) return ret;

	for (std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end() && !ret; idx++)
		if (idx->first < 0)
			ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, idx->first);
	if (ret) return ret;

	if (_el_position == element_position_fixed) {
		if (is_point_inside(client_x, client_y))
			ret = shared_from_this();
	} else {
		if (is_point_inside(x, y))
			ret = shared_from_this();
	}
	return ret;
}

const background *html_tag::get_background(bool own_only)
{
	if (own_only)
		// return own background with check for empty one
			return (_bg._image.empty() && !_bg._color.alpha ? nullptr : &_bg);
	if (_bg._image.empty() && !_bg._color.alpha) {
		// if this is root element (<html>) try to get background from body
		if (!have_parent())
			for (const auto &el : _children)
				if (el->is_body())
					// return own body background
						return el->get_background(true);
		return nullptr;
	}
	if (is_body()) {
		element::ptr el_parent = parent();
		if (el_parent)
			if (!el_parent->get_background(true))
				// parent of body will draw background for body
					return nullptr;
	}
	return &_bg;
}

int html_tag::render_box(int x, int y, int max_width, bool second_pass /*= false*/)
{
	int parent_width = max_width;
	calc_outlines(parent_width);
	_pos.clear();
	_pos.move_to(x, y);
	_pos.x += content_margins_left();
	_pos.y += content_margins_top();

	int ret_width = 0;
	def_value<int> block_width(0);

	if (_display != display_table_cell && !_css_width.is_predefined()) {
		int w = calc_width(parent_width);
		if (_box_sizing == box_sizing_border_box)
			w -= _padding.width() + _borders.width();
		ret_width = max_width = block_width = w;
	}
	else {
		if (max_width)
			max_width -= content_margins_left() + content_margins_right();
	}

	// check for max-width
	if (!_css_max_width.is_predefined()) {
		int mw = get_document()->cvt_units(_css_max_width, _font_size, parent_width);
		if (_box_sizing == box_sizing_border_box)
			mw -= _padding.left + _borders.left + _padding.right + _borders.right;
		if (max_width > mw)
			max_width = mw;
	}

	_floats_left.clear();
	_floats_right.clear();
	_boxes.clear();
	_cahe_line_left.invalidate();
	_cahe_line_right.invalidate();

	element_position el_position;

	int block_height = 0;
	_pos.height = 0;

	if (get_predefined_height(block_height))
		_pos.height = block_height;

	white_space ws = get_white_space();
	bool skip_spaces = (ws == white_space_normal ||
		ws == white_space_nowrap ||
		ws == white_space_pre_line);
	bool was_space = false;

	for (auto el : _children) {
		// we don't need process absolute and fixed positioned element on the second pass
		if (second_pass) {
			el_position = el->get_element_position();
			if ((el_position == element_position_absolute || el_position == element_position_fixed)) continue;
		}

		// skip spaces to make rendering a bit faster
		if (skip_spaces) {
			if (el->is_white_space()) {
				if (was_space) {
					el->skip(true);
					continue;
				}
				else
					was_space = true;
			}
			else
				was_space = false;
		}

		// place element into rendering flow
		int rw = place_element(el, max_width);
		if (rw > ret_width)
			ret_width = rw;
	}

	finish_last_box(true);

	_pos.width = (block_width.is_default() && is_inline_box() ? ret_width : max_width);
	calc_auto_margins(parent_width);

	if (!_boxes.empty()) {
		if (collapse_top_margin()) {
			int old_top = _margins.top;
			_margins.top = std::max(_boxes.front()->top_margin(), _margins.top);
			if (_margins.top != old_top)
				update_floats(_margins.top - old_top, shared_from_this());
		}
		if (collapse_bottom_margin()) {
			_margins.bottom = std::max(_boxes.back()->bottom_margin(), _margins.bottom);
			_pos.height = _boxes.back()->bottom() - _boxes.back()->bottom_margin();
		}
		else
			_pos.height = _boxes.back()->bottom();
	}

	// add the floats height to the block height
	if (is_floats_holder()) {
		int floats_height = get_floats_height();
		if (floats_height > _pos.height)
			_pos.height = floats_height;
	}

	// calculate the final position
	_pos.move_to(x, y);
	_pos.x += content_margins_left();
	_pos.y += content_margins_top();
	if (get_predefined_height(block_height))
		_pos.height = block_height;

	int min_height = 0;
	if (!_css_min_height.is_predefined() && _css_min_height.units() == css_units_percentage) {
		element::ptr el_parent = parent();
		if (el_parent)
			if (el_parent->get_predefined_height(block_height))
				min_height = _css_min_height.calc_percent(block_height);
	}
	else
		min_height = (int)_css_min_height.val();
	if (min_height != 0 && _box_sizing == box_sizing_border_box) {
		min_height -= _padding.top + _borders.top + _padding.bottom + _borders.bottom;
		if (min_height < 0) min_height = 0;
	}
	if (_display == display_list_item) {
		const tchar_t *list_image = get_style_property(_t("list-style-image"), true, 0);
		if (list_image) {
			tstring url;
			css::parse_css_url(list_image, url);
			size sz;
			const tchar_t *list_image_baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
			get_document()->container()->GetImageSize(url.c_str(), list_image_baseurl, sz);
			if (min_height < sz.height)
				min_height = sz.height;
		}
	}

	if (min_height > _pos.height)
		_pos.height = min_height;
	int min_width = _css_min_width.calc_percent(parent_width);
	if (min_width != 0 && _box_sizing == box_sizing_border_box) {
		min_width -= _padding.left + _borders.left + _padding.right + _borders.right;
		if (min_width < 0) min_width = 0;
	}

	if (min_width != 0) {
		if (min_width > _pos.width)
			_pos.width = min_width;
		if (min_width > ret_width)
			ret_width = min_width;
	}

	ret_width += content_margins_left() + content_margins_right();

	// re-render with new width
	if (ret_width < max_width && !second_pass && have_parent()) {
		if (_display == display_inline_block || _css_width.is_predefined() && (_float != float_none || _display == display_table || _el_position == element_position_absolute || _el_position == element_position_fixed)) {
			render(x, y, ret_width, true);
			_pos.width = ret_width - (content_margins_left() + content_margins_right());
		}
	}
	if (is_floats_holder() && !second_pass)
		for (const auto &fb : _floats_left)
			fb.el->apply_relative_shift(fb.el->parent()->calc_width(_pos.width));
	return ret_width;
}

int html_tag::render_table(int x, int y, int max_width, bool second_pass /*= false*/)
{
	if (!_grid) return 0;

	int parent_width = max_width;
	calc_outlines(parent_width);

	_pos.clear();
	_pos.move_to(x, y);
	_pos.x += content_margins_left();
	_pos.y += content_margins_top();

	def_value<int> block_width(0);
	if (!_css_width.is_predefined())
		max_width = block_width = calc_width(parent_width) - _padding.width() - _borders.width();
	else {
		if (max_width)
			max_width -= content_margins_left() + content_margins_right();
	}

	// Calculate table spacing
	int table_width_spacing = 0;
	if (_border_collapse == border_collapse_separate)
		table_width_spacing = _border_spacing_x * (_grid->cols_count() + 1);
	else {
		table_width_spacing = 0;
		if (_grid->cols_count()) {
			table_width_spacing -= std::min(border_left(), _grid->column(0).border_left);
			table_width_spacing -= std::min(border_right(), _grid->column(_grid->cols_count() - 1).border_right);
		}
		for (int col = 1; col < _grid->cols_count(); col++)
			table_width_spacing -= std::min(_grid->column(col).border_left, _grid->column(col - 1).border_right);
	}

	// Calculate the minimum content width (MCW) of each cell: the formatted content may span any number of lines but may not overflow the cell box. 
	// If the specified 'width' (W) of the cell is greater than MCW, W is the minimum cell width. A value of 'auto' means that MCW is the minimum cell width.
	// 
	// Also, calculate the "maximum" cell width of each cell: formatting the content without breaking lines other than where explicit line breaks occur.

	if (_grid->cols_count() == 1 && !block_width.is_default()) {
		for (int row = 0; row < _grid->rows_count(); row++) {
			table_cell *cell = _grid->cell(0, row);
			if (cell && cell->el) {
				cell->min_width = cell->max_width = cell->el->render(0, 0, max_width - table_width_spacing);
				cell->el->_pos.width = cell->min_width - cell->el->content_margins_left() - cell->el->content_margins_right();
			}
		}
	}
	else {
		for (int row = 0; row < _grid->rows_count(); row++) {
			for (int col = 0; col < _grid->cols_count(); col++) {
				table_cell *cell = _grid->cell(col, row);
				if (cell && cell->el) {
					if (!_grid->column(col).css_width.is_predefined() && _grid->column(col).css_width.units() != css_units_percentage) {
						int css_w = _grid->column(col).css_width.calc_percent(block_width);
						int el_w = cell->el->render(0, 0, css_w);
						cell->min_width = cell->max_width = std::max(css_w, el_w);
						cell->el->_pos.width = cell->min_width - cell->el->content_margins_left() - cell->el->content_margins_right();
					}
					else {
						// calculate minimum content width
						cell->min_width = cell->el->render(0, 0, 1);
						// calculate maximum content width
						cell->max_width = cell->el->render(0, 0, max_width - table_width_spacing);
					}
				}
			}
		}
	}

	// For each column, determine a maximum and minimum column width from the cells that span only that column. 
	// The minimum is that required by the cell with the largest minimum cell width (or the column 'width', whichever is larger). 
	// The maximum is that required by the cell with the largest maximum cell width (or the column 'width', whichever is larger).
	for (int col = 0; col < _grid->cols_count(); col++) {
		_grid->column(col).max_width = 0;
		_grid->column(col).min_width = 0;
		for (int row = 0; row < _grid->rows_count(); row++)
			if (_grid->cell(col, row)->colspan <= 1) {
				_grid->column(col).max_width = std::max(_grid->column(col).max_width, _grid->cell(col, row)->max_width);
				_grid->column(col).min_width = std::max(_grid->column(col).min_width, _grid->cell(col, row)->min_width);
			}
	}

	// For each cell that spans more than one column, increase the minimum widths of the columns it spans so that together, 
	// they are at least as wide as the cell. Do the same for the maximum widths. 
	// If possible, widen all spanned columns by approximately the same amount.
	for (int col = 0; col < _grid->cols_count(); col++) {
		for (int row = 0; row < _grid->rows_count(); row++)
			if (_grid->cell(col, row)->colspan > 1) {
				int max_total_width = _grid->column(col).max_width;
				int min_total_width = _grid->column(col).min_width;
				for (int col2 = col + 1; col2 < col + _grid->cell(col, row)->colspan; col2++) {
					max_total_width += _grid->column(col2).max_width;
					min_total_width += _grid->column(col2).min_width;
				}
				if (min_total_width < _grid->cell(col, row)->min_width)
					_grid->distribute_min_width(_grid->cell(col, row)->min_width - min_total_width, col, col + _grid->cell(col, row)->colspan - 1);
				if (max_total_width < _grid->cell(col, row)->max_width)
					_grid->distribute_max_width(_grid->cell(col, row)->max_width - max_total_width, col, col + _grid->cell(col, row)->colspan - 1);
			}
	}

	// If the 'table' or 'inline-table' element's 'width' property has a computed value (W) other than 'auto', the used width is the 
	// greater of W, CAPMIN, and the minimum width required by all the columns plus cell spacing or borders (MIN). 
	// If the used width is greater than MIN, the extra width should be distributed over the columns.
	//
	// If the 'table' or 'inline-table' element has 'width: auto', the used width is the greater of the table's containing block width, 
	// CAPMIN, and MIN. However, if either CAPMIN or the maximum width required by the columns plus cell spacing or borders (MAX) is 
	// less than that of the containing block, use max(MAX, CAPMIN).
	int table_width = 0;
	int min_table_width = 0;
	int max_table_width = 0;

	if (!block_width.is_default())
		table_width = _grid->calc_table_width(block_width - table_width_spacing, false, min_table_width, max_table_width);
	else
		table_width = _grid->calc_table_width(max_width - table_width_spacing, true, min_table_width, max_table_width);
	min_table_width += table_width_spacing;
	max_table_width += table_width_spacing;
	table_width += table_width_spacing;
	_grid->calc_horizontal_positions(_borders, _border_collapse, _border_spacing_x);
	bool row_span_found = false;

	// render cells with computed width
	for (int row = 0; row < _grid->rows_count(); row++) {
		_grid->row(row).height = 0;
		for (int col = 0; col < _grid->cols_count(); col++) {
			table_cell *cell = _grid->cell(col, row);
			if (cell->el) {
				int span_col = col + cell->colspan - 1;
				if (span_col >= _grid->cols_count())
					span_col = _grid->cols_count() - 1;
				int cell_width = _grid->column(span_col).right - _grid->column(col).left;
				if (cell->el->_pos.width != cell_width - cell->el->content_margins_left() - cell->el->content_margins_right()) {
					cell->el->render(_grid->column(col).left, 0, cell_width);
					cell->el->_pos.width = cell_width - cell->el->content_margins_left() - cell->el->content_margins_right();
				}
				else
					cell->el->_pos.x = _grid->column(col).left + cell->el->content_margins_left();
				if (cell->rowspan <= 1)
					_grid->row(row).height = std::max(_grid->row(row).height, cell->el->height());
				else
					row_span_found = true;
			}
		}
	}

	if (row_span_found) {
		for (int col = 0; col < _grid->cols_count(); col++)
			for (int row = 0; row < _grid->rows_count(); row++) {
				table_cell *cell = _grid->cell(col, row);
				if (cell->el) {
					int span_row = row + cell->rowspan - 1;
					if (span_row >= _grid->rows_count())
						span_row = _grid->rows_count() - 1;
					if (span_row != row) {
						int h = 0;
						for (int i = row; i <= span_row; i++)
							h += _grid->row(i).height;
						if (h < cell->el->height())
							_grid->row(span_row).height += cell->el->height() - h;
					}
				}
			}
	}

	// Calculate vertical table spacing
	int table_height_spacing = 0;
	if (_border_collapse == border_collapse_separate)
		table_height_spacing = _border_spacing_y * (_grid->rows_count() + 1);
	else {
		table_height_spacing = 0;
		if (_grid->rows_count()) {
			table_height_spacing -= std::min(border_top(), _grid->row(0).border_top);
			table_height_spacing -= std::min(border_bottom(), _grid->row(_grid->rows_count() - 1).border_bottom);
		}
		for (int row = 1; row < _grid->rows_count(); row++)
			table_height_spacing -= std::min(_grid->row(row).border_top, _grid->row(row - 1).border_bottom);
	}

	// calculate block height
	int block_height = 0;
	if (get_predefined_height(block_height))
		block_height -= _padding.height() + _borders.height();

	// calculate minimum height from _css_min_height
	int min_height = 0;
	if (!_css_min_height.is_predefined() && _css_min_height.units() == css_units_percentage) {
		element::ptr el_parent = parent();
		if (el_parent) {
			int parent_height = 0;
			if (el_parent->get_predefined_height(parent_height))
				min_height = _css_min_height.calc_percent(parent_height);
		}
	}
	else
		min_height = (int)_css_min_height.val();

	int extra_row_height = 0;
	int minimu_table_height = std::max(block_height, min_height);

	_grid->calc_rows_height(minimu_table_height - table_height_spacing, _border_spacing_y);
	_grid->calc_vertical_positions(_borders, _border_collapse, _border_spacing_y);

	int table_height = 0;

	// place cells vertically
	for (int col = 0; col < _grid->cols_count(); col++) {
		for (int row = 0; row < _grid->rows_count(); row++) {
			table_cell *cell = _grid->cell(col, row);
			if (cell->el) {
				int span_row = row + cell->rowspan - 1;
				if (span_row >= _grid->rows_count())
					span_row = _grid->rows_count() - 1;
				cell->el->_pos.y = _grid->row(row).top + cell->el->content_margins_top();
				cell->el->_pos.height = _grid->row(span_row).bottom - _grid->row(row).top - cell->el->content_margins_top() - cell->el->content_margins_bottom();
				table_height = std::max(table_height, _grid->row(span_row).bottom);
				cell->el->apply_vertical_align();
			}
		}
	}
	if (_border_collapse == border_collapse_collapse) {
		if (_grid->rows_count())
			table_height -= std::min(border_bottom(), _grid->row(_grid->rows_count() - 1).border_bottom);
	}
	else
		table_height += _border_spacing_y;
	_pos.width = table_width;

	calc_auto_margins(parent_width);
	_pos.move_to(x, y);
	_pos.x += content_margins_left();
	_pos.y += content_margins_top();
	_pos.width = table_width;
	_pos.height = table_height;
	return max_table_width;
}

void html_tag::draw_children_box(uint_ptr hdc, int x, int y, const position *clip, draw_flag flag, int zindex)
{
	position pos = _pos;
	pos.x += x;
	pos.y += y;
	document::ptr doc = get_document();
	if (_overflow > overflow_visible) {
		position border_box = pos;
		border_box += _padding;
		border_box += _borders;
		border_radiuses bdr_radius = _css_borders.radius.calc_percents(border_box.width, border_box.height);
		bdr_radius -= _borders;
		bdr_radius -= _padding;
		doc->container()->SetClip(pos, bdr_radius, true, true);
	}

	position browser_wnd;
	doc->container()->GetClientRect(browser_wnd);

	element::ptr el;
	for (auto &item : _children) {
		el = item;
		if (el->is_visible()) {
			switch (flag) {
			case draw_positioned:
				if (el->is_positioned() && el->get_zindex() == zindex) {
					if (el->get_element_position() == element_position_fixed) {
						el->draw(hdc, browser_wnd.x, browser_wnd.y, clip);
						el->draw_stacking_context(hdc, browser_wnd.x, browser_wnd.y, clip, true);
					}
					else {
						el->draw(hdc, pos.x, pos.y, clip);
						el->draw_stacking_context(hdc, pos.x, pos.y, clip, true);
					}
					el = nullptr;
				}
				break;
			case draw_block:
				if (!el->is_inline_box() && el->get_float() == float_none && !el->is_positioned())
					el->draw(hdc, pos.x, pos.y, clip);
				break;
			case draw_floats:
				if (el->get_float() != float_none && !el->is_positioned()) {
					el->draw(hdc, pos.x, pos.y, clip);
					el->draw_stacking_context(hdc, pos.x, pos.y, clip, false);
					el = nullptr;
				}
				break;
			case draw_inlines:
				if (el->is_inline_box() && el->get_float() == float_none && !el->is_positioned()) {
					el->draw(hdc, pos.x, pos.y, clip);
					if (el->get_display() == display_inline_block) {
						el->draw_stacking_context(hdc, pos.x, pos.y, clip, false);
						el = nullptr;
					}
				}
				break;
			default:
				break;
			}

			if (el) {
				if (flag == draw_positioned) {
					if (!el->is_positioned())
						el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
				}
				else {
					if (el->get_float() == float_none && el->get_display() != display_inline_block && !el->is_positioned())
						el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
				}
			}
		}
	}

	if (_overflow > overflow_visible)
		doc->container()->DelClip();
}

void html_tag::draw_children_table(uint_ptr hdc, int x, int y, const position *clip, draw_flag flag, int zindex)
{
	if (!_grid) return;
	position pos = _pos;
	pos.x += x;
	pos.y += y;
	for (int row = 0; row < _grid->rows_count(); row++) {
		if (flag == draw_block)
			_grid->row(row).el_row->draw_background(hdc, pos.x, pos.y, clip);
		for (int col = 0; col < _grid->cols_count(); col++) {
			table_cell *cell = _grid->cell(col, row);
			if (cell->el) {
				if (flag == draw_block)
					cell->el->draw(hdc, pos.x, pos.y, clip);
				cell->el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
			}
		}
	}
}

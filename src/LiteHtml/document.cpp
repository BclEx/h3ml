#include "html.h"
#include "document.h"
#include "stylesheet.h"
#include "html_tag.h"
#include "el_text.h"
#include "el_para.h"
#include "el_space.h"
#include "el_body.h"
#include "el_image.h"
#include "el_table.h"
#include "el_td.h"
#include "el_link.h"
#include "el_title.h"
#include "el_style.h"
#include "el_script.h"
#include "el_comment.h"
#include "el_cdata.h"
#include "el_base.h"
#include "el_anchor.h"
#include "el_break.h"
#include "el_div.h"
#include "el_font.h"
#include "el_tr.h"
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include "gumbo/gumbo.h"
#include "utf8_strings.h"
using namespace litehtml;

document::document(document_container *objContainer, context *ctx)
{
	_container = objContainer;
	_context = ctx;
}

document::~document()
{
	_over_element = nullptr;
	if (_container)
		for (fonts_map::iterator f = _fonts.begin(); f != _fonts.end(); f++)
			_container->DeleteFont(f->second.font);
}

document::ptr document::createFromString(const tchar_t *str, document_container *objPainter, context *ctx, css *user_styles)
{
	return createFromUTF8(litehtml_to_utf8(str), objPainter, ctx, user_styles);
}

document::ptr document::createFromUTF8(const char *str, document_container *objPainter, context *ctx, css *user_styles)
{
	// parse document into GumboOutput
	GumboOutput *output = gumbo_parse((const char *)str);

	// Create document
	document::ptr doc = std::make_shared<document>(objPainter, ctx);

	// Create elements.
	elements_vector root_elements;
	doc->create_node(output->root, root_elements);
	if (!root_elements.empty())
		doc->_root = root_elements.back();
	// Destroy GumboOutput
	gumbo_destroy_output(&kGumboDefaultOptions, output);

	// Let's process created elements tree
	if (doc->_root) {
		doc->container()->GetMediaFeatures(doc->_media);

		// apply master CSS
		doc->_root->apply_stylesheet(ctx->master_css());

		// parse elements attributes
		doc->_root->parse_attributes();

		// parse style sheets linked in document
		media_query_list::ptr media;
		for (css_text::vector::iterator css = doc->_css.begin(); css != doc->_css.end(); css++) {
			media = (!css->media.empty() ? media_query_list::create_fro_string(css->media, doc) : nullptr);
			doc->_styles.parse_stylesheet(css->text.c_str(), css->baseurl.c_str(), doc, media);
		}
		// Sort css selectors using CSS rules.
		doc->_styles.sort_selectors();

		// get current media features
		if (!doc->_media_lists.empty())
			doc->update_media_lists(doc->_media);

		// Apply parsed styles.
		doc->_root->apply_stylesheet(doc->_styles);

		// Apply user styles if any
		if (user_styles)
			doc->_root->apply_stylesheet(*user_styles);

		// Parse applied styles in the elements
		doc->_root->parse_styles();

		// Now the _tabular_elements is filled with tabular elements.
		// We have to check the tabular elements for missing table elements 
		// and create the anonymous boxes in visual table layout
		doc->fix_tables_layout();

		// Fanaly initialize elements
		doc->_root->init();
	}
	return doc;
}

uint_ptr document::add_font( const tchar_t* name, int size, const tchar_t* weight, const tchar_t* style, const tchar_t* decoration, font_metrics* fm )
{
	uint_ptr ret = 0;
	if (!name || (name && !t_strcasecmp(name, _t("inherit"))))
		name = _container->GetDefaultFontName();
	if (!size)
		size = container()->GetDefaultFontSize();
	tchar_t strSize[20];
	t_itoa(size, strSize, 20, 10);
	tstring key = name;
	key += _t(":");
	key += strSize;
	key += _t(":");
	key += weight;
	key += _t(":");
	key += style;
	key += _t(":");
	key += decoration;

	if (_fonts.find(key) == _fonts.end())
	{
		font_style fs = (font_style) value_index(style, font_style_strings, fontStyleNormal);
		int	fw = value_index(weight, font_weight_strings, -1);
		if (fw >= 0)
			switch(fw) {
			case fontWeightBold: fw = 700; break;
			case fontWeightBolder: fw = 600; break;
			case fontWeightLighter: fw = 300; break;
			default: fw = 400; break;
		}
		else {
			fw = t_atoi(weight);
			if (fw < 100)
				fw = 400;
		}

		unsigned int decor = 0;
		if (decoration) {
			std::vector<tstring> tokens;
			split_string(decoration, tokens, _t(" "));
			for (std::vector<tstring>::iterator i = tokens.begin(); i != tokens.end(); i++) {
				if (!t_strcasecmp(i->c_str(), _t("underline")))
					decor |= font_decoration_underline;
				else if (!t_strcasecmp(i->c_str(), _t("line-through")))
					decor |= font_decoration_linethrough;
				else if (!t_strcasecmp(i->c_str(), _t("overline")))
					decor |= font_decoration_overline;
			}
		}

		font_item fi= {0};
		fi.font = _container->CreateFont(name, size, fw, fs, decor, &fi.metrics);
		_fonts[key] = fi;
		ret = fi.font;
		if (fm)
			*fm = fi.metrics;
	}
	return ret;
}

uint_ptr document::get_font(const tchar_t *name, int size, const tchar_t *weight, const tchar_t *style, const tchar_t *decoration, font_metrics *fm)
{
	if (!name || (name && !t_strcasecmp(name, _t("inherit"))))
		name = _container->GetDefaultFontName();
	if (!size)
		size = container()->GetDefaultFontSize();
	tchar_t strSize[20];
	t_itoa(size, strSize, 20, 10);
	tstring key = name;
	key += _t(":");
	key += strSize;
	key += _t(":");
	key += weight;
	key += _t(":");
	key += style;
	key += _t(":");
	key += decoration;

	fonts_map::iterator el = _fonts.find(key);
	if (el != _fonts.end()) {
		if (fm)
			*fm = el->second.metrics;
		return el->second.font;
	}
	return add_font(name, size, weight, style, decoration, fm);
}

int document::render(int max_width, render_type rt)
{
	int ret = 0;
	if (_root) {
		if (rt == render_fixed_only) {
			_fixed_boxes.clear();
			_root->render_positioned(rt);
		}
		else {
			ret = _root->render(0, 0, max_width);
			if (_root->fetch_positioned()) {
				_fixed_boxes.clear();
				_root->render_positioned(rt);
			}
			_size.width	= 0;
			_size.height = 0;
			_root->calc_document_size(_size);
		}
	}
	return ret;
}

void document::draw(uint_ptr hdc, int x, int y, const position *clip)
{
	if (_root) {
		_root->draw(hdc, x, y, clip);
		_root->draw_stacking_context(hdc, x, y, clip, true);
	}
}

int document::cvt_units(const tchar_t *str, int fontSize, bool *is_percent /*= nullptr*/) const
{
	if (!str) return 0;
	css_length val;
	val.fromString(str);
	if (is_percent && val.units() == css_units_percentage && !val.is_predefined())
		*is_percent = true;
	return cvt_units(val, fontSize);
}

int document::cvt_units(css_length &val, int fontSize, int size) const
{
	if (val.is_predefined())
		return 0;
	int ret = 0;
	switch (val.units()) {
	case css_units_percentage: ret = val.calc_percent(size); break;
	case css_units_em: ret = round_f(val.val() * fontSize); val.set_value((float)ret, css_units_px); break;
	case css_units_pt: ret = _container->PtToPx((int)val.val()); val.set_value((float)ret, css_units_px); break;
	case css_units_in: ret = _container->PtToPx((int)(val.val() * 72)); val.set_value((float)ret, css_units_px); break;
	case css_units_cm: ret = _container->PtToPx((int)(val.val() * 0.3937 * 72)); val.set_value((float)ret, css_units_px); break;
	case css_units_mm: ret = _container->PtToPx((int)(val.val() * 0.3937 * 72) / 10); val.set_value((float)ret, css_units_px); break;
	case css_units_vw: ret = (int)((double)_media.width * (double)val.val() / 100.0); break;
	case css_units_vh: ret = (int)((double)_media.height * (double)val.val() / 100.0); break;
	case css_units_vmin: ret = (int)((double)std::min(_media.height, _media.width) * (double)val.val() / 100.0); break;
	case css_units_vmax: ret = (int)((double)std::max(_media.height, _media.width) * (double)val.val() / 100.0); break;
	default: ret = (int)val.val(); break;
	}
	return ret;
}

int document::width() const
{
	return _size.width;
}

int document::height() const
{
	return _size.height;
}

void document::add_stylesheet(const tchar_t *str, const tchar_t *baseurl, const tchar_t *media)
{
	if (str && str[0])
		_css.push_back(css_text(str, baseurl, media));
}

bool document::on_mouse_over(int x, int y, int client_x, int client_y, position::vector &redraw_boxes)
{
	if (!_root)
		return false;
	element::ptr over_el = _root->get_element_by_point(x, y, client_x, client_y);
	bool state_was_changed = false;
	if (over_el != _over_element) {
		if (_over_element)
			if (_over_element->on_mouse_leave())
				state_was_changed = true;
		_over_element = over_el;
	}
	const tchar_t *cursor = nullptr;
	if (_over_element) {
		if (_over_element->on_mouse_over())
			state_was_changed = true;
		cursor = _over_element->get_cursor();
	}
	_container->SetCursor(cursor ? cursor : _t("auto"));
	if (state_was_changed)
		return _root->find_styles_changes(redraw_boxes, 0, 0);
	return false;
}

bool document::on_mouse_leave(position::vector &redraw_boxes)
{
	if (!_root)
		return false;
	if (_over_element)
		if (_over_element->on_mouse_leave())
			return _root->find_styles_changes(redraw_boxes, 0, 0);
	return false;
}

bool document::on_lbutton_down(int x, int y, int client_x, int client_y, position::vector &redraw_boxes)
{
	if (!_root)
		return false;
	element::ptr over_el = _root->get_element_by_point(x, y, client_x, client_y);
	bool state_was_changed = false;
	if (over_el != _over_element) {
		if (_over_element)
			if (_over_element->on_mouse_leave())
				state_was_changed = true;
		_over_element = over_el;
		if (_over_element)
			if (_over_element->on_mouse_over())
				state_was_changed = true;
	}
	const tchar_t *cursor = nullptr;
	if (_over_element) {
		if (_over_element->on_lbutton_down())
			state_was_changed = true;
		cursor = _over_element->get_cursor();
	}
	_container->SetCursor(cursor ? cursor : _t("auto"));
	if (state_was_changed)
		return _root->find_styles_changes(redraw_boxes, 0, 0);
	return false;
}

bool document::on_lbutton_up(int x, int y, int client_x, int client_y, position::vector &redraw_boxes)
{
	if (!_root)
		return false;
	if (_over_element)
		if (_over_element->on_lbutton_up())
			return _root->find_styles_changes(redraw_boxes, 0, 0);
	return false;
}

element::ptr document::create_element(const tchar_t *tag_name, const string_map &attributes)
{
	element::ptr newTag;
	document::ptr this_doc = shared_from_this();
	if (_container)
		newTag = _container->CreateElement(tag_name, attributes, this_doc);
	if (!newTag) {
		if (!t_strcmp(tag_name, _t("br")))
			newTag = std::make_shared<el_break>(this_doc);
		else if (!t_strcmp(tag_name, _t("p")))
			newTag = std::make_shared<el_para>(this_doc);
		else if (!t_strcmp(tag_name, _t("img")))
			newTag = std::make_shared<el_image>(this_doc);
		else if (!t_strcmp(tag_name, _t("table")))
			newTag = std::make_shared<el_table>(this_doc);
		else if (!t_strcmp(tag_name, _t("td")) || !t_strcmp(tag_name, _t("th")))
			newTag = std::make_shared<el_td>(this_doc);
		else if (!t_strcmp(tag_name, _t("link")))
			newTag = std::make_shared<el_link>(this_doc);
		else if (!t_strcmp(tag_name, _t("title")))
			newTag = std::make_shared<el_title>(this_doc);
		else if (!t_strcmp(tag_name, _t("a")))
			newTag = std::make_shared<el_anchor>(this_doc);
		else if (!t_strcmp(tag_name, _t("tr")))
			newTag = std::make_shared<el_tr>(this_doc);
		else if (!t_strcmp(tag_name, _t("style")))
			newTag = std::make_shared<el_style>(this_doc);
		else if (!t_strcmp(tag_name, _t("base")))
			newTag = std::make_shared<el_base>(this_doc);
		else if (!t_strcmp(tag_name, _t("body")))
			newTag = std::make_shared<el_body>(this_doc);
		else if (!t_strcmp(tag_name, _t("div")))
			newTag = std::make_shared<el_div>(this_doc);
		else if (!t_strcmp(tag_name, _t("script")))
			newTag = std::make_shared<el_script>(this_doc);
		else if (!t_strcmp(tag_name, _t("font")))
			newTag = std::make_shared<el_font>(this_doc);
		else
			newTag = std::make_shared<html_tag>(this_doc);
	}
	if (newTag) {
		newTag->set_tagName(tag_name);
		for (string_map::const_iterator iter = attributes.begin(); iter != attributes.end(); iter++)
			newTag->set_attr(iter->first.c_str(), iter->second.c_str());
	}
	return newTag;
}

void document::get_fixed_boxes( position::vector &fixed_boxes )
{
	fixed_boxes = _fixed_boxes;
}

void document::add_fixed_box( const position &pos )
{
	_fixed_boxes.push_back(pos);
}

bool document::media_changed()
{
	if (!_media_lists.empty()) {
		container()->GetMediaFeatures(_media);
		if (update_media_lists(_media)) {
			_root->refresh_styles();
			_root->parse_styles();
			return true;
		}
	}
	return false;
}

bool document::lang_changed()
{
	if (!_media_lists.empty()) {
		tstring culture;
		container()->GetLanguage(_lang, culture);
		if (!culture.empty())
			_culture = _lang + _t('-') + culture;
		else
			_culture.clear();
		_root->refresh_styles();
		_root->parse_styles();
		return true;
	}
	return false;
}

bool document::update_media_lists(const media_features &features)
{
	bool update_styles = false;
	for (media_query_list::vector::iterator iter = _media_lists.begin(); iter != _media_lists.end(); iter++)
		if ((*iter)->apply_media_features(features))
			update_styles = true;
	return update_styles;
}

void document::add_media_list(media_query_list::ptr list)
{
	if (list)
		if (std::find(_media_lists.begin(), _media_lists.end(), list) == _media_lists.end())
			_media_lists.push_back(list);
}

void document::create_node(GumboNode *node, elements_vector &elements)
{
	switch (node->type) {
	case GUMBO_NODE_ELEMENT: {
		string_map attrs;
		GumboAttribute *attr;
		for (unsigned int i = 0; i < node->v.element.attributes.length; i++) {
			attr = (GumboAttribute *)node->v.element.attributes.data[i];
			attrs[tstring(litehtml_from_utf8(attr->name))] = litehtml_from_utf8(attr->value);
		}
		element::ptr ret;
		const char *tag = gumbo_normalized_tagname(node->v.element.tag);
		if (tag[0])
			ret = create_element(litehtml_from_utf8(tag), attrs);
		else {
			if (node->v.element.original_tag.data && node->v.element.original_tag.length) {
				std::string strA;
				gumbo_tag_from_original_text(&node->v.element.original_tag);
				strA.append(node->v.element.original_tag.data, node->v.element.original_tag.length);
				ret = create_element(litehtml_from_utf8(strA.c_str()), attrs);
			}
		}
		if (ret) {
			elements_vector child;
			for (unsigned int i = 0; i < node->v.element.children.length; i++) {
				child.clear();
				create_node(static_cast<GumboNode *>(node->v.element.children.data[i]), child);
				std::for_each(child.begin(), child.end(), [&ret](element::ptr &el) { ret->appendChild(el); } );
			}
			elements.push_back(ret);
		}
		break; }
	case GUMBO_NODE_TEXT:
		{
			std::wstring str;
			std::wstring str_in = (const wchar_t *)(utf8_to_wchar(node->v.text.text));
			ucode_t c;
			for (size_t i = 0; i < str_in.length(); i++) {
				c = (ucode_t) str_in[i];
				if (c <= ' ' && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f')) {
					if (!str.empty()) {
						elements.push_back(std::make_shared<el_text>(litehtml_from_wchar(str.c_str()), shared_from_this()));
						str.clear();
					}
					str += c;
					elements.push_back(std::make_shared<el_space>(litehtml_from_wchar(str.c_str()), shared_from_this()));
					str.clear();
				}
				// CJK character range
				else if (c >= 0x4E00 && c <= 0x9FCC)
				{
					if (!str.empty()) {
						elements.push_back(std::make_shared<el_text>(litehtml_from_wchar(str.c_str()), shared_from_this()));
						str.clear();
					}
					str += c;
					elements.push_back(std::make_shared<el_text>(litehtml_from_wchar(str.c_str()), shared_from_this()));
					str.clear();
				}
				else
					str += c;
			}
			if (!str.empty())
				elements.push_back(std::make_shared<el_text>(litehtml_from_wchar(str.c_str()), shared_from_this()));
		}
		break;
	case GUMBO_NODE_CDATA: {
		element::ptr ret = std::make_shared<el_cdata>(shared_from_this());
		ret->set_data(litehtml_from_utf8(node->v.text.text));
		elements.push_back(ret);
		break; }
	case GUMBO_NODE_COMMENT: {
		element::ptr ret = std::make_shared<el_comment>(shared_from_this());
		ret->set_data(litehtml_from_utf8(node->v.text.text));
		elements.push_back(ret);
		break; }
	case GUMBO_NODE_WHITESPACE: {
		tstring str = litehtml_from_utf8(node->v.text.text);
		for (size_t i = 0; i < str.length(); i++)
			elements.push_back(std::make_shared<el_space>(str.substr(i, 1).c_str(), shared_from_this()));
		break; }
	default:
		break;
	}
}

void document::fix_tables_layout()
{
	size_t i = 0;
	while (i < _tabular_elements.size())
	{
		element::ptr el_ptr = _tabular_elements[i];
		switch (el_ptr->get_display()) {
		case display_inline_table:
		case display_table:
			fix_table_children(el_ptr, display_table_row_group, _t("table-row-group"));
			break;
		case display_table_footer_group:
		case display_table_row_group:
		case display_table_header_group:
			fix_table_parent(el_ptr, display_table, _t("table"));
			fix_table_children(el_ptr, display_table_row, _t("table-row"));
			break;
		case display_table_row:
			fix_table_parent(el_ptr, display_table_row_group, _t("table-row-group"));
			fix_table_children(el_ptr, display_table_cell, _t("table-cell"));
			break;
		case display_table_cell:
			fix_table_parent(el_ptr, display_table_row, _t("table-row"));
			break;
			// TODO: make table layout fix for table-caption, table-column etc. elements
		case display_table_caption:
		case display_table_column:
		case display_table_column_group:
		default:
			break;
		}
		i++;
	}
}

void document::fix_table_children(element::ptr &el_ptr, style_display disp, const tchar_t *disp_str)
{
	elements_vector tmp;
	elements_vector::iterator first_iter = el_ptr->_children.begin();
	elements_vector::iterator cur_iter = el_ptr->_children.begin();
	
	auto flush_elements = [&]() {
		element::ptr annon_tag = std::make_shared<html_tag>(shared_from_this());
		style st;
		st.add_property(_t("display"), disp_str, 0, false);
		annon_tag->add_style(st);
		annon_tag->parent(el_ptr);
		annon_tag->parse_styles();
		std::for_each(tmp.begin(), tmp.end(), [&annon_tag](element::ptr &el) { annon_tag->appendChild(el); });
		first_iter = el_ptr->_children.insert(first_iter, annon_tag);
		cur_iter = first_iter + 1;
		while (cur_iter != el_ptr->_children.end() && (*cur_iter)->parent() != el_ptr)
			cur_iter = el_ptr->_children.erase(cur_iter);
		first_iter = cur_iter;
		tmp.clear();
	};

	while (cur_iter != el_ptr->_children.end()) {
		if ((*cur_iter)->get_display() != disp) {
			if (!(*cur_iter)->is_white_space() || ((*cur_iter)->is_white_space() && !tmp.empty())) {
				if (tmp.empty())
					first_iter = cur_iter;
				tmp.push_back((*cur_iter));
			}
			cur_iter++;
		}
		else if (!tmp.empty())
			flush_elements();
		else
			cur_iter++;
	}
	if (!tmp.empty())
		flush_elements();
}

void document::fix_table_parent(element::ptr &el_ptr, style_display disp, const tchar_t *disp_str)
{
	element::ptr parent = el_ptr->parent();
	if (parent->get_display() != disp) {
		elements_vector::iterator this_element = std::find_if(parent->_children.begin(), parent->_children.end(), [&](element::ptr &el) { return (el == el_ptr); });
		if (this_element != parent->_children.end())
		{
			style_display el_disp = el_ptr->get_display();
			elements_vector::iterator first = this_element;
			elements_vector::iterator last = this_element;
			elements_vector::iterator cur = this_element;

			// find first element with same display
			while (true) {
				if (cur == parent->_children.begin()) break;
				cur--;
				if ((*cur)->is_white_space() || (*cur)->get_display() == el_disp)
					first = cur;
				else
					break;
			}

			// find last element with same display
			cur = this_element;
			while (true) {
				cur++;
				if (cur == parent->_children.end()) break;
				if ((*cur)->is_white_space() || (*cur)->get_display() == el_disp)
					last = cur;
				else
					break;
			}

			// extract elements with the same display and wrap them with anonymous object
			element::ptr annon_tag = std::make_shared<html_tag>(shared_from_this());
			style st;
			st.add_property(_t("display"), disp_str, 0, false);
			annon_tag->add_style(st);
			annon_tag->parent(parent);
			annon_tag->parse_styles();
			std::for_each(first, last + 1, [&annon_tag](element::ptr &el) { annon_tag->appendChild(el); });
			first = parent->_children.erase(first, last + 1);
			parent->_children.insert(first, annon_tag);
		}
	}
}

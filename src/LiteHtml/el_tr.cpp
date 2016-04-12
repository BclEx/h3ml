#include "html.h"
#include "el_tr.h"
using namespace litehtml;

el_tr::el_tr(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_tr::~el_tr() { }

void el_tr::parse_attributes()
{
	const tchar_t *str = get_attr(_t("align"));
	if (str)
		_style.add_property(_t("text-align"), str, 0, false);
	str = get_attr(_t("valign"));
	if (str)
		_style.add_property(_t("vertical-align"), str, 0, false);
	str = get_attr(_t("bgcolor"));
	if (str)
		_style.add_property(_t("background-color"), str, 0, false);
	html_tag::parse_attributes();
}

void el_tr::get_inline_boxes(position::vector &boxes)
{
	position pos;
	for (auto &el : _children)
		if (el->get_display() == display_table_cell) {
			pos.x = el->left() + el->margin_left();
			pos.y = el->top() - _padding.top - _borders.top;
			pos.width = el->right() - pos.x - el->margin_right() - el->margin_left();
			pos.height = el->height() + _padding.top + _padding.bottom + _borders.top + _borders.bottom;
			boxes.push_back(pos);
		}
}

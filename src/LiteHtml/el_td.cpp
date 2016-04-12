#include "html.h"
#include "el_td.h"
using namespace litehtml;

el_td::el_td(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_td::~el_td() { }

void el_td::parse_attributes()
{
	const tchar_t *str = get_attr(_t("width"));
	if (str)
		_style.add_property(_t("width"), str, 0, false);
	str = get_attr(_t("background"));
	if (str) {
		tstring url = _t("url('");
		url += str;
		url += _t("')");
		_style.add_property(_t("background-image"), url.c_str(), 0, false);
	}
	str = get_attr(_t("align"));
	if (str)
		_style.add_property(_t("text-align"), str, 0, false);
	str = get_attr(_t("bgcolor"));
	if (str)
		_style.add_property(_t("background-color"), str, 0, false);
	str = get_attr(_t("valign"));
	if (str)
		_style.add_property(_t("vertical-align"), str, 0, false);
	html_tag::parse_attributes();
}


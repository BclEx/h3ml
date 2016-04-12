#include "html.h"
#include "el_div.h"
using namespace litehtml;

el_div::el_div(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_div::~el_div() { }

void el_div::parse_attributes()
{
	const tchar_t *str = get_attr(_t("align"));
	if (str)
		_style.add_property(_t("text-align"), str, 0, false);
	html_tag::parse_attributes();
}

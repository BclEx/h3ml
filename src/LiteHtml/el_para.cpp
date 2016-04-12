#include "html.h"
#include "el_para.h"
#include "document.h"
using namespace litehtml;

el_para::el_para(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_para::~el_para() { }

void el_para::parse_attributes()
{
	const tchar_t *str = get_attr(_t("align"));
	if (str)
		_style.add_property(_t("text-align"), str, 0, false);
	html_tag::parse_attributes();
}

#include "html.h"
#include "el_style.h"
#include "document.h"
using namespace litehtml;

el_style::el_style(const std::shared_ptr<document> &doc) : element(doc) { }
el_style::~el_style() { }

void el_style::parse_attributes()
{
	tstring text;
	for(auto &el : _children)
		el->get_text(text);
	get_document()->add_stylesheet( text.c_str(), 0, get_attr(_t("media")) );
}

bool el_style::appendChild(const ptr &el)
{
	_children.push_back(el);
	return true;
}

const tchar_t *el_style::get_tagName() const
{
	return _t("style");
}

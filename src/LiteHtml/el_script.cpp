#include "html.h"
#include "el_script.h"
#include "document.h"
using namespace litehtml;

el_script::el_script(const std::shared_ptr<document> &doc) : element(doc) { }
el_script::~el_script() { }

void el_script::parse_attributes()
{
	// TODO: pass script text to document container
}

bool el_script::appendChild(const ptr &el)
{
	el->get_text(_text);
	return true;
}

const tchar_t *el_script::get_tagName() const
{
	return _t("script");
}

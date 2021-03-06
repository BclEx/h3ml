#include "html.h"
#include "el_base.h"
#include "document.h"
using namespace litehtml;

el_base::el_base(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_base::~el_base() { }

void el_base::parse_attributes()
{
	get_document()->container()->SetBaseUrl(get_attr(_t("href")));
}

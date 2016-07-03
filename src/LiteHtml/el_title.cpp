#include "html.h"
#include "el_title.h"
#include "document.h"
using namespace litehtml;

el_title::el_title(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_title::~el_title() { }

void el_title::parse_attributes()
{
	tstring text;
	get_text(text);
	get_document()->container()->SetCaption(text.c_str());
}

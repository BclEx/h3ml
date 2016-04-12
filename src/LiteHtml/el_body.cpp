#include "html.h"
#include "el_body.h"
#include "document.h"
using namespace litehtml;

el_body::el_body(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_body::~el_body() { }

bool el_body::is_body() const
{
	return true;
}

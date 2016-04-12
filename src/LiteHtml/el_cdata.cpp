#include "html.h"
#include "el_cdata.h"
using namespace litehtml;

el_cdata::el_cdata(const std::shared_ptr<document> &doc) : element(doc)
{
	_skip = true;
}
el_cdata::~el_cdata() { }

void el_cdata::get_text(tstring &text)
{
	text += _text;
}

void el_cdata::set_data(const tchar_t *data)
{
	if (data)
		_text += data;
}

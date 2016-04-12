#include "html.h"
#include "el_comment.h"
using namespace litehtml;

el_comment::el_comment(const std::shared_ptr<document> &doc) : element(doc)
{
	_skip = true;
}
el_comment::~el_comment() { }

void el_comment::get_text(tstring &text)
{
	text += _text;
}

void el_comment::set_data(const tchar_t *data)
{
	if (data)
		_text += data;
}

#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_comment : public element
	{
		tstring	_text;
	public:
		el_comment(const std::shared_ptr<document> &doc);
		virtual ~el_comment();

		virtual void get_text(tstring &text) override;
		virtual void set_data(const tchar_t *data) override;
	};
}

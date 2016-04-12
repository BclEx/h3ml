#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_script : public element
	{
		tstring _text;
	public:
		el_script(const std::shared_ptr<document> &doc);
		virtual ~el_script();

		virtual void parse_attributes() override;
		virtual bool appendChild(const ptr &el) override;
		virtual const tchar_t *get_tagName() const override;
	};
}

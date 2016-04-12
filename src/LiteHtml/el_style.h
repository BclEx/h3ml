#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_style : public element
	{
		elements_vector _children;
	public:
		el_style(const std::shared_ptr<document> &doc);
		virtual ~el_style();

		virtual void parse_attributes() override;
		virtual bool appendChild(const ptr &el) override;
		virtual const tchar_t *get_tagName() const override;
	};
}

#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_base : public html_tag
	{
	public:
		el_base(const std::shared_ptr<document> &doc);
		virtual ~el_base();

		virtual void parse_attributes() override;
	};
}

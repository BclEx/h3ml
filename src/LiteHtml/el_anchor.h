#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_anchor : public html_tag
	{
	public:
		el_anchor(const std::shared_ptr<document> &doc);
		virtual ~el_anchor();

		virtual void on_click() override;
		virtual void apply_stylesheet(const css &stylesheet) override;
	};
}
#pragma once
#include "style.h"
#include "css_selector.h"

namespace litehtml
{
	class document_container;
	class css
	{
		css_selector::vector _selectors;
	public:
		css() { }
		~css() { }

		const css_selector::vector &selectors() const
		{
			return _selectors;
		}

		void clear()
		{
			_selectors.clear();
		}

		void parse_stylesheet(const tchar_t *str, const tchar_t *baseurl, const std::shared_ptr <document> &doc, const media_query_list::ptr &media);
		void sort_selectors();
		static void	parse_css_url(const tstring &str, tstring &url);

	private:
		void parse_atrule(const tstring &text, const tchar_t *baseurl, const std::shared_ptr<document> &doc, const media_query_list::ptr &media);
		void add_selector(css_selector::ptr selector);
		bool parse_selectors(const tstring &txt, const style::ptr &styles, const media_query_list::ptr &media);
	};

	inline void css::add_selector(css_selector::ptr selector)
	{
		selector->_order = (int)_selectors.size();
		_selectors.push_back(selector);
	}
}

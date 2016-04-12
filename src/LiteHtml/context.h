#pragma once
#include "stylesheet.h"

namespace litehtml
{
	class context
	{
		css _master_css;
	public:
		void load_master_stylesheet(const tchar_t *str);
		css &master_css()
		{
			return _master_css;
		}
	};
}
#include "html.h"
#include "el_font.h"
using namespace litehtml;

el_font::el_font(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_font::~el_font() { }

void el_font::parse_attributes()
{
	const tchar_t *str = get_attr(_t("color"));
	if (str)
		_style.add_property(_t("color"), str, 0, false);
	str = get_attr(_t("face"));
	if (str)
		_style.add_property(_t("font-face"), str, 0, false);
	str = get_attr(_t("size"));
	if (str) {
		int sz = t_atoi(str);
		if (sz <= 1)
			_style.add_property(_t("font-size"), _t("x-small"), 0, false);
		else if (sz >= 6)
			_style.add_property(_t("font-size"), _t("xx-large"), 0, false);
		else
			switch (sz) {
			case 2: _style.add_property(_t("font-size"), _t("small"), 0, false); break;
			case 3: _style.add_property(_t("font-size"), _t("medium"), 0, false); break;
			case 4: _style.add_property(_t("font-size"), _t("large"), 0, false); break;
			case 5: _style.add_property(_t("font-size"), _t("x-large"), 0, false); break;
		}
	}
	html_tag::parse_attributes();
}

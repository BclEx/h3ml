#include "html.h"
#include "el_anchor.h"
#include "document.h"
using namespace litehtml;

el_anchor::el_anchor(const std::shared_ptr<document> &doc) : html_tag(doc) { }
el_anchor::~el_anchor() { }

void el_anchor::on_click()
{
	const tchar_t *href = get_attr(_t("href"));
	if (href)
		get_document()->container()->on_anchor_click(href, shared_from_this());
}

void el_anchor::apply_stylesheet(const css &stylesheet)
{
	if (get_attr(_t("href")) )
		_pseudo_classes.push_back(_t("link"));
	html_tag::apply_stylesheet(stylesheet);
}

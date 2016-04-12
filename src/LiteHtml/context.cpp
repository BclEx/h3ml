#include "html.h"
#include "context.h"
#include "stylesheet.h"
using namespace litehtml;

void context::load_master_stylesheet(const tchar_t *str)
{
	media_query_list::ptr media;
	_master_css.parse_stylesheet(str, 0, std::shared_ptr<document>(), media_query_list::ptr());
	_master_css.sort_selectors();
}

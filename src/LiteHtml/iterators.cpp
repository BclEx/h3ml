#include "html.h"
#include "iterators.h"
#include "html_tag.h"
using namespace litehtml;

element::ptr elements_iterator::next(bool ret_parent)
{
	next_idx();
	while (_idx < (int) _el->get_children_count()) {
		element::ptr el = _el->get_child(_idx);
		if (el->get_children_count() && _go_inside && _go_inside->select(el)) {
			stack_item si;
			si.idx = _idx;
			si.el = _el;
			_stack.push_back(si);
			_el = el;
			_idx = -1;
			if (ret_parent)
				return el;
			next_idx();
		}
		else {
			if (!_select || (_select && _select->select(_el->get_child(_idx))))
				return _el->get_child(_idx);
			else
				next_idx();
		}
	}
	return 0;
}

void elements_iterator::next_idx()
{
	_idx++;
	while (_idx >= (int)_el->get_children_count() && _stack.size()) {
		stack_item si = _stack.back();
		_stack.pop_back();
		_idx = si.idx;
		_el	= si.el;
		_idx++;
		continue;
	}
}

bool go_inside_inline::select(const element::ptr &el)
{
	return (el->get_display() == display_inline || el->get_display() == display_inline_text);
}

bool go_inside_table::select(const element::ptr &el)
{
	return (el->get_display() == display_table_row_group || el->get_display() == display_table_header_group || el->get_display() == display_table_footer_group);
}

bool table_rows_selector::select(const element::ptr &el)
{
	return (el->get_display() == display_table_row);
}

bool table_cells_selector::select(const element::ptr &el)
{
	return (el->get_display() == display_table_cell);
}

#pragma once
#include "style.h"
#include "media_query.h"

namespace litehtml
{
	struct selector_specificity
	{
		int a;
		int b;
		int c;
		int d;

		selector_specificity(int va = 0, int vb = 0, int vc = 0, int vd = 0)
		{
			a = va;
			b = vb;
			c = vc;
			d = vd;
		}
		void operator += (const selector_specificity &val)
		{
			a += val.a;
			b += val.b;
			c += val.c;
			d += val.d;
		}
		bool operator==(const selector_specificity &val) const
		{
			return (a == val.a && b == val.b && c == val.c && d == val.d);
		}
		bool operator!=(const selector_specificity &val) const
		{
			return (a != val.a || b != val.b || c != val.c || d != val.d);
		}
		bool operator > (const selector_specificity &val) const
		{
			if (a > val.a)
				return true;
			else if (a < val.a)
				return false;
			else {
				if (b > val.b)
					return true;
				else if (b < val.b)
					return false;
				else {
					if (c > val.c)
						return true;
					else if (c < val.c)
						return false;
					else {
						if (d > val.d)
							return true;
						else if (d < val.d)
							return false;
					}
				}
			}
			return false;
		}
		bool operator >= (const selector_specificity &val) const
		{
			if ((*this) == val) return true;
			if ((*this) > val) return true;
			return false;
		}
		bool operator <= (const selector_specificity &val) const
		{
			return !((*this) > val);
		}
		bool operator < (const selector_specificity &val) const
		{
			return ((*this) <= val && (*this) != val);
		}
	};

	enum attr_select_condition
	{
		select_exists,
		select_equal,
		select_contain_str,
		select_start_str,
		select_end_str,
		select_pseudo_class,
		select_pseudo_element,
	};

	struct css_attribute_selector
	{
		typedef std::vector<css_attribute_selector>	vector;
		tstring attribute;
		tstring val;
		string_vector class_val;
		attr_select_condition condition;

		css_attribute_selector()
		{
			condition = select_exists;
		}
	};

	class css_element_selector
	{
	public:
		tstring _tag;
		css_attribute_selector::vector	_attrs;
	public:
		void parse(const tstring &txt);
	};

	enum css_combinator
	{
		combinator_descendant,
		combinator_child,
		combinator_adjacent_sibling,
		combinator_general_sibling
	};

	class css_selector
	{
	public:
		typedef std::shared_ptr<css_selector> ptr;
		typedef std::vector<css_selector::ptr> vector;
	public:
		selector_specificity _specificity;
		css_element_selector _right;
		css_selector::ptr _left;
		css_combinator _combinator;
		style::ptr _style;
		int _order;
		media_query_list::ptr _media_query;
	public:
		css_selector(media_query_list::ptr media)
		{
			_media_query = media;
			_combinator	= combinator_descendant;
			_order = 0;
		}
		~css_selector() { }
		css_selector(const css_selector &val)
		{
			_right = val._right;
			_left = (val._left ? std::make_shared<css_selector>(*val._left) : 0);
			_combinator	= val._combinator;
			_specificity = val._specificity;
			_order = val._order;
			_media_query = val._media_query;
		}

		bool parse(const tstring &text);
		void calc_specificity();
		bool is_media_valid() const;
		void add_media_to_doc(document* doc) const;
	};

	inline bool css_selector::is_media_valid() const
	{
		return (!_media_query ? true : _media_query->is_used());
	}

	inline bool operator > (const css_selector &v1, const css_selector &v2)
	{
		return (v1._specificity == v2._specificity ? v1._order > v2._order : v1._specificity > v2._specificity);
	}
	inline bool operator < (const css_selector &v1, const css_selector &v2)
	{
		return (v1._specificity == v2._specificity ? v1._order < v2._order : v1._specificity < v2._specificity);
	}
	inline bool operator > (css_selector::ptr v1, css_selector::ptr v2)
	{
		return (*v1 > *v2);
	}
	inline bool operator < (css_selector::ptr v1, css_selector::ptr v2)
	{
		return (*v1 < *v2);
	}

	class used_selector
	{
	public:
		typedef std::unique_ptr<used_selector> ptr;
		typedef std::vector<used_selector::ptr>	vector;
		css_selector::ptr _selector;
		bool _used;

		used_selector(const css_selector::ptr &selector, bool used)
		{
			_used = used;
			_selector = selector;
		}
	};
}
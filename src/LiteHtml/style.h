#pragma once
#include "attributes.h"
#include <string>

namespace litehtml
{
	class property_value
	{
	public:
		tstring	_value;
		bool _important;

		property_value()
		{
			_important = false;
		}
		property_value(const tchar_t *val, bool imp)
		{
			_important = imp;
			_value = val;
		}
		property_value(const property_value &val)
		{
			_value = val._value;
			_important = val._important;
		}
		property_value &operator=(const property_value &val)
		{
			_value = val._value;
			_important = val._important;
			return *this;
		}
	};

	typedef std::map<tstring, property_value> props_map;

	class style
	{
	public:
		typedef std::shared_ptr<style>		ptr;
		typedef std::vector<style::ptr> vector;
	private:
		props_map _properties;
		static string_map _valid_values;
	public:
		style();
		style(const style &val);
		virtual ~style();

		void operator=(const style &val)
		{
			_properties = val._properties;
		}

		void add(const tchar_t *txt, const tchar_t *baseurl)
		{
			parse(txt, baseurl);
		}

		void add_property(const tchar_t *name, const tchar_t *val, const tchar_t *baseurl, bool important);

		const tchar_t *get_property(const tchar_t *name) const
		{
			if (name)
			{
				props_map::const_iterator f = _properties.find(name);
				if (f != _properties.end())
					return f->second._value.c_str();
			}
			return nullptr;
		}

		void combine(const style &src);
		void clear()
		{
			_properties.clear();
		}

	private:
		void parse_property(const tstring &txt, const tchar_t *baseurl);
		void parse(const tchar_t *txt, const tchar_t *baseurl);
		void parse_short_border(const tstring &prefix, const tstring &val, bool important);
		void parse_short_background(const tstring &val, const tchar_t *baseurl, bool important);
		void parse_short_font(const tstring &val, bool important);
		void add_parsed_property(const tstring &name, const tstring &val, bool important);
		void remove_property(const tstring &name, bool important);
	};
}
#pragma once
#include "types.h"

namespace litehtml
{
	class css_length
	{
		union
		{
			float _value;
			int _predef;
		};
		css_units _units;
		bool _is_predefined;
	public:
		css_length();
		css_length(const css_length &val);

		css_length &operator=(const css_length &val);
		css_length &operator=(float val);
		bool is_predefined() const;
		void predef(int val);
		int predef() const;
		void set_value(float val, css_units units);
		float val() const;
		css_units units() const;
		int calc_percent(int width) const;
		void fromString(const tstring &str, const tstring &predefs = _t(""), int defValue = 0);
	};

	// css_length inlines
	inline css_length::css_length()
	{
		_value = 0;
		_predef = 0;
		_units = css_units_none;
		_is_predefined = false;
	}
	inline css_length::css_length(const css_length &val)
	{
		if (val.is_predefined())
			_predef	= val._predef;
		else
			_value = val._value;
		_units = val._units;
		_is_predefined = val._is_predefined;
	}
	inline css_length &css_length::operator=(const css_length &val)
	{
		if (val.is_predefined())
			_predef	= val._predef;
		else
			_value = val._value;
		_units = val._units;
		_is_predefined = val._is_predefined;
		return *this;
	}
	inline css_length &css_length::operator=(float val)
	{
		_value = val;
		_units = css_units_px;
		_is_predefined = false;
		return *this;
	}

	inline bool css_length::is_predefined() const
	{ 
		return _is_predefined;					
	}

	inline void css_length::predef(int val)		
	{ 
		_predef = val; 
		_is_predefined = true;	
	}

	inline int css_length::predef() const
	{ 
		return (_is_predefined ? _predef : 0);
	}

	inline void css_length::set_value(float val, css_units units)		
	{ 
		_value = val; 
		_is_predefined = false;	
		_units = units;
	}

	inline float css_length::val() const
	{
		return (!_is_predefined ? _value : 0);
	}

	inline css_units css_length::units() const
	{
		return _units;
	}

	inline int css_length::calc_percent(int width) const
	{
		if (!is_predefined()) {
			if (units() == css_units_percentage)
				return (int)((double)width * (double)_value / 100.0);
			else
				return (int)val();
		}
		return 0;
	}
}
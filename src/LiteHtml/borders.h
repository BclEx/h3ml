#pragma once
#include "css_length.h"
#include "types.h"

namespace litehtml
{
	struct css_border
	{
		css_length width;
		border_style style;
		web_color color;

		css_border()
		{
			style = border_style_none;
		}
		css_border(const css_border &val)
		{
			width = val.width;
			style = val.style;
			color = val.color;
		}
		css_border &operator=(const css_border &val)
		{
			width = val.width;
			style = val.style;
			color = val.color;
			return *this;
		}
	};

	struct border
	{
		int width;
		border_style style;
		web_color color;

		border()
		{
			width = 0;
		}
		border(const border &val)
		{
			width = val.width;
			style = val.style;
			color = val.color;
		}
		border(const css_border &val)
		{
			width = (int)val.width.val();
			style = val.style;
			color = val.color;
		}
		border &operator=(const border &val)
		{
			width = val.width;
			style = val.style;
			color = val.color;
			return *this;
		}
		border &operator=(const css_border &val)
		{
			width = (int)val.width.val();
			style = val.style;
			color = val.color;
			return *this;
		}
	};

	struct border_radiuses
	{
		int	top_left_x;
		int	top_left_y;
		int	top_right_x;
		int	top_right_y;
		int	botto_right_x;
		int	botto_right_y;
		int	botto_left_x;
		int	botto_left_y;

		border_radiuses()
		{
			top_left_x = 0;
			top_left_y = 0;
			top_right_x = 0;
			top_right_y = 0;
			botto_right_x = 0;
			botto_right_y = 0;
			botto_left_x = 0;
			botto_left_y = 0;
		}
		border_radiuses(const border_radiuses &val)
		{
			top_left_x = val.top_left_x;
			top_left_y = val.top_left_y;
			top_right_x = val.top_right_x;
			top_right_y = val.top_right_y;
			botto_right_x = val.botto_right_x;
			botto_right_y = val.botto_right_y;
			botto_left_x = val.botto_left_x;
			botto_left_y = val.botto_left_y;
		}
		border_radiuses &operator = (const border_radiuses &val)
		{
			top_left_x = val.top_left_x;
			top_left_y = val.top_left_y;
			top_right_x = val.top_right_x;
			top_right_y = val.top_right_y;
			botto_right_x = val.botto_right_x;
			botto_right_y = val.botto_right_y;
			botto_left_x = val.botto_left_x;
			botto_left_y = val.botto_left_y;
			return *this;
		}
		void operator += (const margins &mg)
		{
			top_left_x += mg.left;
			top_left_y += mg.top;
			top_right_x += mg.right;
			top_right_y += mg.top;
			botto_right_x += mg.right;
			botto_right_y += mg.bottom;
			botto_left_x += mg.left;
			botto_left_y += mg.bottom;
			fix_values();
		}
		void operator -= (const margins &mg)
		{
			top_left_x -= mg.left;
			top_left_y -= mg.top;
			top_right_x -= mg.right;
			top_right_y -= mg.top;
			botto_right_x -= mg.right;
			botto_right_y -= mg.bottom;
			botto_left_x -= mg.left;
			botto_left_y -= mg.bottom;
			fix_values();
		}

		void fix_values()
		{
			if (top_left_x < 0)	top_left_x = 0;
			if (top_left_y < 0)	top_left_y = 0;
			if (top_right_x < 0) top_right_x = 0;
			if (botto_right_x < 0) botto_right_x = 0;
			if (botto_right_y < 0) botto_right_y = 0;
			if (botto_left_x < 0) botto_left_x = 0;
			if (botto_left_y < 0) botto_left_y = 0;
		}
	};

	struct css_border_radius
	{
		css_length top_left_x;
		css_length top_left_y;
		css_length top_right_x;
		css_length top_right_y;
		css_length botto_right_x;
		css_length botto_right_y;
		css_length botto_left_x;
		css_length botto_left_y;

		css_border_radius() { }
		css_border_radius(const css_border_radius &val)
		{
			top_left_x = val.top_left_x;
			top_left_y = val.top_left_y;
			top_right_x = val.top_right_x;
			top_right_y = val.top_right_y;
			botto_left_x = val.botto_left_x;
			botto_left_y = val.botto_left_y;
			botto_right_x = val.botto_right_x;
			botto_right_y = val.botto_right_y;
		}

		css_border_radius &operator=(const css_border_radius &val)
		{
			top_left_x = val.top_left_x;
			top_left_y = val.top_left_y;
			top_right_x = val.top_right_x;
			top_right_y = val.top_right_y;
			botto_left_x = val.botto_left_x;
			botto_left_y = val.botto_left_y;
			botto_right_x = val.botto_right_x;
			botto_right_y = val.botto_right_y;
			return *this;
		}

		border_radiuses calc_percents(int width, int height)
		{
			border_radiuses ret;
			ret.botto_left_x = botto_left_x.calc_percent(width);
			ret.botto_left_y = botto_left_y.calc_percent(height);
			ret.top_left_x = top_left_x.calc_percent(width);
			ret.top_left_y = top_left_y.calc_percent(height);
			ret.top_right_x = top_right_x.calc_percent(width);
			ret.top_right_y = top_right_y.calc_percent(height);
			ret.botto_right_x = botto_right_x.calc_percent(width);
			ret.botto_right_y = botto_right_y.calc_percent(height);
			return ret;
		}
	};

	struct css_borders
	{
		css_border left;
		css_border top;
		css_border right;
		css_border bottom;
		css_border_radius radius;

		css_borders() { }
		css_borders(const css_borders &val)
		{
			left = val.left;
			right = val.right;
			top = val.top;
			bottom = val.bottom;
			radius = val.radius;
		}
		css_borders &operator=(const css_borders &val)
		{
			left = val.left;
			right = val.right;
			top = val.top;
			bottom = val.bottom;
			radius = val.radius;
			return *this;
		}
	};

	struct borders
	{
		border left;
		border top;
		border right;
		border bottom;
		border_radiuses	radius;

		borders() { }
		borders(const borders &val)
		{
			left = val.left;
			right = val.right;
			top = val.top;
			bottom = val.bottom;
			radius = val.radius;
		}
		borders(const css_borders &val)
		{
			left = val.left;
			right = val.right;
			top = val.top;
			bottom = val.bottom;
		}
		borders &operator=(const borders &val)
		{
			left = val.left;
			right = val.right;
			top = val.top;
			bottom = val.bottom;
			radius = val.radius;
			return *this;
		}
		borders &operator=(const css_borders &val)
		{
			left = val.left;
			right = val.right;
			top = val.top;
			bottom = val.bottom;
			return *this;
		}
	};
}

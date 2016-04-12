#pragma once
#include "types.h"
#include "attributes.h"
#include "css_length.h"
#include "css_position.h"
#include "web_color.h"
#include "borders.h"

namespace litehtml
{
	class background
	{
	public:
		tstring _image;
		tstring _baseurl;
		web_color _color;
		background_attachment _attachment;
		css_position _position;
		background_repeat _repeat;
		background_box _clip;
		background_box _origin;
		css_border_radius _radius;
	public:
		background();
		background(const background &val);
		~background();
		background &operator=(const background &val);
	};

	class background_paint
	{
	public:
		tstring image;
		tstring baseurl;
		background_attachment attachment;
		background_repeat repeat;
		web_color color;
		position clip_box;
		position origin_box;
		position border_box;
		border_radiuses border_radius;
		size image_size;
		int position_x;
		int position_y;
		bool is_root;
	public:
		background_paint();
		background_paint(const background_paint &val);
		void operator=(const background &val);
	};
}
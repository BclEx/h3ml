#include "html.h"
#include "background.h"
using namespace litehtml;

background::background()
{
	_attachment	= background_attachment_scroll;
	_repeat		= background_repeat_repeat;
	_clip		= background_box_border;
	_origin		= background_box_padding;
	_color.alpha	= 0;
	_color.red		= 0;
	_color.green	= 0;
	_color.blue		= 0;
}
background::background(const background &val)
{
	_image			= val._image;
	_baseurl		= val._baseurl;
	_color			= val._color;
	_attachment		= val._attachment;
	_position		= val._position;
	_repeat			= val._repeat;
	_clip			= val._clip;
	_origin			= val._origin;
}
background::~background() { }
background &background::operator=(const background &val)
{
	_image			= val._image;
	_baseurl		= val._baseurl;
	_color			= val._color;
	_attachment	= val._attachment;
	_position		= val._position;
	_repeat		= val._repeat;
	_clip			= val._clip;
	_origin		= val._origin;
	return *this;
}
background_paint::background_paint() : color(0, 0, 0, 0)
{
	position_x		= 0;
	position_y		= 0;
	attachment		= background_attachment_scroll;
	repeat			= background_repeat_repeat;
	is_root			= false;
}
background_paint::background_paint(const background_paint &val)
{
	image			= val.image;
	baseurl			= val.baseurl;
	attachment		= val.attachment;
	repeat			= val.repeat;
	color			= val.color;
	clip_box		= val.clip_box;
	origin_box		= val.origin_box;
	border_box		= val.border_box;
	border_radius	= val.border_radius;
	image_size		= val.image_size;
	position_x		= val.position_x;
	position_y		= val.position_y;
	is_root			= val.is_root;
}
void background_paint::operator=(const background &val)
{
	attachment	= val._attachment;
	baseurl		= val._baseurl;
	image		= val._image;
	repeat		= val._repeat;
	color		= val._color;
}

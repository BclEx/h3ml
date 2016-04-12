#pragma once

namespace litehtml
{
	class html_tag;

	enum box_type
	{
		box_block,
		box_line
	};

	class box
	{
	public:
		typedef std::unique_ptr<box> ptr;
		typedef std::vector<box::ptr> vector;
	protected:
		int _box_top;
		int _box_left;
		int _box_right;
	public:
		box(int top, int left, int right)
		{
			_box_top = top;
			_box_left = left;
			_box_right = right;
		}

		int bottom() { return _box_top + height(); }
		int top() { return _box_top; }
		int right() { return _box_left + width(); }
		int left() { return _box_left; }

		virtual box_type get_type() = 0;
		virtual int height() = 0;
		virtual int width() = 0;
		virtual void add_element(const element::ptr &el) = 0;
		virtual bool can_hold(const element::ptr &el, white_space ws) = 0;
		virtual void finish(bool last_box = false) = 0;
		virtual bool is_empty() = 0;
		virtual int baseline() = 0;
		virtual void get_elements(elements_vector &els) = 0;
		virtual int top_margin() = 0;
		virtual int botto_margin() = 0;
		virtual void y_shift(int shift) = 0;
		virtual void new_width(int left, int right, elements_vector &els) = 0;
	};

	//////////////////////////////////////////////////////////////////////////

	class block_box : public box
	{
		element::ptr _element;
	public:
		block_box(int top, int left, int right)
			: box(top, left, right)
		{
			_element = 0;
		}

		virtual box_type get_type();
		virtual int height();
		virtual int width();
		virtual void add_element(const element::ptr &el);
		virtual bool can_hold(const element::ptr &el, white_space ws);
		virtual void finish(bool last_box = false);
		virtual bool is_empty();
		virtual int baseline();
		virtual void get_elements(elements_vector &els);
		virtual int top_margin();
		virtual int botto_margin();
		virtual void y_shift(int shift);
		virtual void new_width(int left, int right, elements_vector &els);
	};

	//////////////////////////////////////////////////////////////////////////

	class line_box : public box
	{
		elements_vector _items;
		int _height;
		int _width;
		int _line_height;
		font_metrics _font_metrics;
		int _baseline;
		text_align _text_align;
	public:
		line_box(int top, int left, int right, int line_height, font_metrics &fm, text_align align)
			: box(top, left, right)
		{
			_height = 0;
			_width = 0;
			_font_metrics = fm;
			_line_height = line_height;
			_baseline = 0;
			_text_align	= align;
		}

		virtual box_type get_type();
		virtual int height();
		virtual int width();
		virtual void add_element(const element::ptr &el);
		virtual bool can_hold(const element::ptr &el, white_space ws);
		virtual void finish(bool last_box = false);
		virtual bool is_empty();
		virtual int baseline();
		virtual void get_elements(elements_vector &els);
		virtual int top_margin();
		virtual int botto_margin();
		virtual void y_shift(int shift);
		virtual void new_width(int left, int right, elements_vector &els);
	private:
		bool have_last_space();
		bool is_break_only();
	};
}
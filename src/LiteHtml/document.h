#pragma once
#include "style.h"
#include "types.h"
#include "context.h"
#include "gumbo/gumbo.h"

namespace litehtml
{
	struct css_text
	{
		typedef std::vector<css_text> vector;
		tstring	text;
		tstring	baseurl;
		tstring	media;
		
		css_text() { }
		css_text(const tchar_t *txt, const tchar_t *url, const tchar_t *media_str)
		{
			text = (txt ? txt : _t(""));
			baseurl	= (url ? url : _t(""));
			media = (media_str ? media_str : _t(""));
		}
		css_text(const css_text &val)
		{
			text = val.text;
			baseurl = val.baseurl;
			media = val.media;
		}
	};

	struct stop_tags_t
	{
		const tchar_t *tags;
		const tchar_t *stop_parent;
	};

	struct ommited_end_tags_t
	{
		const tchar_t *tag;
		const tchar_t *followed_tags;
	};

	class html_tag;

	class document : public std::enable_shared_from_this<document>
	{
	public:
		typedef std::shared_ptr<document> ptr;
		typedef std::weak_ptr<document> weak_ptr;
	private:
		std::shared_ptr<element> _root;
		document_container *_container;
		fonts_map _fonts;
		css_text::vector _css;
		css _styles;
		web_color _def_color;
		context *_context;
		size _size;
		position::vector _fixed_boxes;
		media_query_list::vector _media_lists;
		element::ptr _over_element;
		elements_vector _tabular_elements;
		media_features _media;
		tstring _lang;
		tstring _culture;
	public:
		document(document_container *objContainer, context *ctx);
		virtual ~document();

		document_container *container()	{ return _container; }
		uint_ptr get_font(const tchar_t *name, int size, const tchar_t *weight, const tchar_t *style, const tchar_t *decoration, font_metrics *fm);
		int render(int max_width, render_type rt = render_all);
		void draw(uint_ptr hdc, int x, int y, const position *clip);
		web_color get_def_color() { return _def_color; }
		int cvt_units(const tchar_t *str, int fontSize, bool *is_percent = nullptr) const;
		int cvt_units(css_length &val, int fontSize, int size = 0) const;
		int width() const;
		int height() const;
		void add_stylesheet(const tchar_t *str, const tchar_t *baseurl, const tchar_t *media);
		bool on_mouse_over(int x, int y, int client_x, int client_y, position::vector &redraw_boxes);
		bool on_lbutton_down(int x, int y, int client_x, int client_y, position::vector &redraw_boxes);
		bool on_lbutton_up(int x, int y, int client_x, int client_y, position::vector &redraw_boxes);
		bool on_mouse_leave(position::vector &redraw_boxes);
		element::ptr create_element(const tchar_t *tag_name, const string_map &attributes);
		element::ptr root();
		void get_fixed_boxes(position::vector &fixed_boxes);
		void add_fixed_box(const position &pos);
		void add_media_list(media_query_list::ptr list);
		bool media_changed();
		bool lang_changed();
		bool match_lang(const tstring &lang);
		void add_tabular(const element::ptr &el);

		static document::ptr createFromString(const tchar_t *str, document_container *objPainter, context *ctx, css *user_styles = nullptr);
		static document::ptr createFromUTF8(const char *str, document_container *objPainter, context *ctx, css *user_styles = nullptr);
	
	private:
		uint_ptr add_font(const tchar_t *name, int size, const tchar_t *weight, const tchar_t *style, const tchar_t *decoration, font_metrics *fm);

		void create_node(GumboNode *node, elements_vector &elements);
		bool update_media_lists(const media_features &features);
		void fix_tables_layout();
		void fix_table_children(element::ptr &el_ptr, style_display disp, const tchar_t *disp_str);
		void fix_table_parent(element::ptr &el_ptr, style_display disp, const tchar_t *disp_str);
	};

	inline element::ptr document::root()
	{
		return _root;
	}
	inline void document::add_tabular(const element::ptr &el)
	{
		_tabular_elements.push_back(el);
	}
	inline bool document::match_lang(const tstring &lang)
	{
		return lang == _lang || lang == _culture;
	}
}

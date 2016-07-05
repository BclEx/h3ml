#ifndef __SIMPLEWIN_H__
#define __SIMPLEWIN_H__

class UserInterfaceLocal;
class DeviceContext;
class SimpleWindow;

typedef struct {
	Window *win;
	SimpleWindow *simp;
} drawWin_t;

class SimpleWindow {
	friend class Window;
public:
	SimpleWindow(Window* win);
	virtual ~SimpleWindow();
	void Redraw(float x, float y);
	void StateChanged(bool redraw);

	string _name;

	WinVar *GetWinVarByName(const char *name);
	int GetWinVarOffset(WinVar *wv, drawWin_t *owner);
	size_t Size();

	Window *GetParent() { return _parent; }

protected:
	void CalcClientRect(float xofs, float yofs);
	void SetupTransforms(float x, float y);
	void DrawBackground(const Rectangle &drawRect);
	void DrawBorderAndCaption(const Rectangle &drawRect);

	UserInterfaceLocal *_gui;
	int 			_flags;
	Rectangle 		_drawRect;			// overall rect
	Rectangle 		_clientRect;			// client area
	Rectangle 		_textRect;
	vec2			_origin;
	class Font		*_font;
	float 			_matScalex;
	float 			_matScaley;
	float 			_borderSize;
	int 			_textAlign;
	float 			_textAlignx;
	float 			_textAligny;
	int				_textShadow;

	WinStr			_text;
	WinBool			_visible;
	WinRectangle	_rect;				// overall rect
	WinVec4			_backColor;
	WinVec4			_matColor;
	WinVec4			_foreColor;
	WinVec4			_borderColor;
	WinFloat		_textScale;
	WinFloat		_rotate;
	WinVec2			_shear;
	WinBackground	_backGroundName;

	const Material *_background;

	Window			*_parent;
	WinBool			_hideCursor;
};

#endif /* !__SIMPLEWIN_H__ */

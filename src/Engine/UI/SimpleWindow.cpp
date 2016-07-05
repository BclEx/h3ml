#include "..\Global.h"
#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "SimpleWindow.h"

SimpleWindow::SimpleWindow(Window *win) {
	_gui = win->GetGui();
	_drawRect = win->_drawRect;
	_clientRect = win->_clientRect;
	_textRect = win->_textRect;
	_origin = win->_origin;
	_font = win->_font;
	_name = win->_name;
	_matScalex = win->_matScalex;
	_matScaley = win->_matScaley;
	_borderSize = win->_borderSize;
	_textAlign = win->_textAlign;
	_textAlignx = win->_textAlignx;
	_textAligny = win->_textAligny;
	_background = win->_background;
	_flags = win->_flags;
	_textShadow = win->_textShadow;

	_visible = win->_visible;
	_text = win->_text;
	_rect = win->_rect;
	_backColor = win->_backColor;
	_matColor = win->_matColor;
	_foreColor = win->_foreColor;
	_borderColor = win->_borderColor;
	_textScale = win->_textScale;
	_rotate = win->_rotate;
	_shear = win->_shear;
	_backGroundName = win->_backGroundName;
	if (_backGroundName.Length()) {
		_background = g_declManager->FindMaterial(_backGroundName);
		_background->SetSort(SS_GUI);
	}
	_backGroundName.SetMaterialPtr(&_background);

	//  added parent
	_parent = win->GetParent();
	_hideCursor = win->_hideCursor;

	Window *parent = win->GetParent();
	if (parent) {
		if (_text.NeedsUpdate())
			_parent->AddUpdateVar(&_text);
		if (_visible.NeedsUpdate())
			_parent->AddUpdateVar(&_visible);
		if (_rect.NeedsUpdate())
			_parent->AddUpdateVar(&_rect);
		if (_backColor.NeedsUpdate())
			_parent->AddUpdateVar(&_backColor);
		if (_matColor.NeedsUpdate())
			_parent->AddUpdateVar(&_matColor);
		if (_foreColor.NeedsUpdate())
			_parent->AddUpdateVar(&_foreColor);
		if (_borderColor.NeedsUpdate())
			_parent->AddUpdateVar(&_borderColor);
		if (_textScale.NeedsUpdate())
			_parent->AddUpdateVar(&_textScale);
		if (_rotate.NeedsUpdate())
			_parent->AddUpdateVar(&_rotate);
		if (_shear.NeedsUpdate())
			_parent->AddUpdateVar(&_shear);
		if (_backGroundName.NeedsUpdate())
			_parent->AddUpdateVar(&_backGroundName);
	}
}

SimpleWindow::~SimpleWindow() {
}

void SimpleWindow::StateChanged(bool redraw) {
}

void SimpleWindow::SetupTransforms(float x, float y) {
	static mat3 trans;
	static vec3 org;
	trans.Identity();
	org.Set(_origin.x + x, _origin.y + y, 0);
	if (_rotate) {
		static Rotation rot;
		static vec3 vec(0, 0, 1);
		rot.Set(org, vec, _rotate);
		trans = rot.ToMat3();
	}

	static mat3 smat;
	smat.Identity();
	if (_shear.x() || _shear.y()) {
		smat[0][1] = _shear.x();
		smat[1][0] = _shear.y();
		trans *= smat;
	}

	if (!trans.IsIdentity())
		_dc->SetTransformInfo(org, trans);
}

void SimpleWindow::DrawBackground(const Rectangle &drawRect) {
	if (_backColor.w() > 0)
		_dc->DrawFilledRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, _backColor);

	if (_background)
		if (_matColor.w() > 0) {
			float scalex, scaley;
			if (_flags & WIN_NATURALMAT) {
				scalex = drawRect.w / _background->GetImageWidth();
				scaley = drawRect.h / _background->GetImageHeight();
			}
			else {
				scalex = _matScalex;
				scaley = _matScaley;
			}
			_dc->DrawMaterial(drawRect.x, drawRect.y, drawRect.w, drawRect.h, _background, _matColor, scalex, scaley);
		}
}

void SimpleWindow::DrawBorderAndCaption(const Rectangle &drawRect) {
	if (_flags & WIN_BORDER)
		if (_borderSize)
			_dc->DrawRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, _borderSize, _borderColor);
}

void SimpleWindow::CalcClientRect(float xofs, float yofs) {
	_drawRect = _rect;
	if (_flags & WIN_INVERTRECT) {
		_drawRect.x = _rect.x() - _rect.w();
		_drawRect.y = _rect.y() - _rect.h();
	}

	_drawRect.x += xofs;
	_drawRect.y += yofs;

	_clientRect = _drawRect;
	if (_rect.h() > 0.0 && _rect.w() > 0.0) {

		if (_flags & WIN_BORDER && _borderSize != 0.0) {
			_clientRect.x += _borderSize;
			_clientRect.y += _borderSize;
			_clientRect.w -= _borderSize;
			_clientRect.h -= _borderSize;
		}

		_textRect = _clientRect;
		_textRect.x += 2.0;
		_textRect.w -= 2.0;
		_textRect.y += 2.0;
		_textRect.h -= 2.0;
		_textRect.x += _textAlignx;
		_textRect.y += _textAligny;

	}
	_origin.Set(_rect.x() + (_rect.w() / 2), _rect.y() + (_rect.h() / 2));
}

void SimpleWindow::Redraw(float x, float y) {
	if (!_visible)
		return;
	CalcClientRect(0, 0);
	_dc->SetFont(_font);
	_drawRect.Offset(x, y);
	_clientRect.Offset(x, y);
	_textRect.Offset(x, y);
	SetupTransforms(x, y);
	if (_flags & WIN_NOCLIP)
		_dc->EnableClipping(false);
	DrawBackground(_drawRect);
	DrawBorderAndCaption(_drawRect);
	if (_textShadow) {
		string shadowText = _text;
		Rectangle shadowRect = _textRect;

		shadowText.RemoveColors();
		shadowRect.x += _textShadow;
		shadowRect.y += _textShadow;

		_dc->DrawText(shadowText, _textScale, _textAlign, colorBlack, _shadowRect, !(_flags & WIN_NOWRAP), -1);
	}
	_dc->DrawText(_text, _textScale, _textAlign, _foreColor, _textRect, !(_flags & WIN_NOWRAP), -1);
	_dc->SetTransformInfo(vec3_origin, mat3_identity);
	if (_flags & WIN_NOCLIP)
		_dc->EnableClipping(true);
	_drawRect.Offset(-x, -y);
	_clientRect.Offset(-x, -y);
	_textRect.Offset(-x, -y);
}

int SimpleWindow::GetWinVarOffset(WinVar *wv, drawWin_t *owner) {
	int ret = -1;
	if (wv == &_rect) ret = (int)&((SimpleWindow *)0)->_rect;
	else if (wv == &_backColor) ret = (int)&((SimpleWindow *)0)->_backColor;
	else if (wv == &_matColor) ret = (int)&((SimpleWindow *)0)->_matColor;
	else if (wv == &_foreColor) ret = (int)&((SimpleWindow *)0)->_foreColor;
	else if (wv == &_borderColor) ret = (int)&((SimpleWindow *)0)->_borderColor;
	else if (wv == &_textScale) ret = (int)&((SimpleWindow *)0)->_textScale;
	else if (wv == &_rotate) ret = (int)&((SimpleWindow *)0)->_rotate;
	if (ret != -1) owner->simp = this;
	return ret;
}

WinVar *SimpleWindow::GetWinVarByName(const char *name) {
	WinVar *retVar = NULL;
	if (!strcmp(name, "background")) retVar = &_backGroundName;
	else if (!strcmp(name, "visible"))  retVar = &_visible;
	else if (!strcmp(name, "rect")) retVar = &_rect;
	else if (!strcmp(name, "backColor")) retVar = &_backColor;
	else if (!strcmp(name, "matColor")) retVar = &_matColor;
	else if (!strcmp(name, "foreColor")) retVar = &_foreColor;
	else if (!strcmp(name, "borderColor")) retVar = &_borderColor;
	else if (!strcmp(name, "textScale")) retVar = &_textScale;
	else if (!strcmp(name, "rotate")) retVar = &_rotate;
	else if (!strcmp(name, "shear")) retVar = &_shear;
	else if (!strcmp(name, "text"))  retVar = &_text;
	return retVar;
}

size_t SimpleWindow::Size() {
	size_t sz = sizeof(*this);
	sz += _name.length();
	sz += _text.Size();
	sz += _backGroundName.Size();
	return sz;
}

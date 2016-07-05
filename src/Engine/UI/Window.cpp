#include "..\Global.h"

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
//#include "EditWindow.h"
//#include "ChoiceWindow.h"
//#include "SliderWindow.h"
//#include "BindWindow.h"
//#include "ListWindow.h"
//#include "RenderWindow.h"
//#include "FieldWindow.h"
//
//#include "GameSSDWindow.h"
//#include "GameBearShootWindow.h"
//#include "GameBustOutWindow.h"

bool Window::_registerIsTemporary[MAX_EXPRESSION_REGISTERS]; // statics to assist during parsing
//float Window::_shaderRegisters[MAX_EXPRESSION_REGISTERS];
//wexpOp_t Window::_shaderOps[MAX_EXPRESSION_OPS];

int Window::c_gui_debug = 0; // ("gui_debug", "0", CVAR_GUI | CVAR_BOOL, "");
int Window::c_gui_edit = 0; // ("gui_edit", "0", CVAR_GUI | CVAR_BOOL, "");
//idCVar hud_titlesafe("hud_titlesafe", "0.0", CVAR_GUI | CVAR_FLOAT, "fraction of the screen to leave around hud for titlesafe area");
//extern idCVar r_skipGuiShaders;		// 1 = don't render any gui elements on surfaces

// made RegisterVars a member of idWindow
const RegEntry Window::RegisterVars[] = {
	{ "forecolor", Register::VEC4 },
	{ "hovercolor", Register::VEC4 },
	{ "backcolor", Register::VEC4 },
	{ "bordercolor", Register::VEC4 },
	{ "rect", Register::RECTANGLE },
	{ "matcolor", Register::VEC4 },
	{ "scale", Register::VEC2 },
	{ "translate", Register::VEC2 },
	{ "rotate", Register::FLOAT },
	{ "textscale", Register::FLOAT },
	{ "visible", Register::BOOL },
	{ "noevents", Register::BOOL },
	{ "text", Register::STRING },
	{ "background", Register::STRING },
	{ "runscript", Register::STRING },
	{ "varbackground", Register::STRING },
	{ "cvar", Register::STRING },
	{ "choices", Register::STRING },
	{ "choiceVar", Register::STRING },
	{ "bind", Register::STRING },
	{ "modelRotate", Register::VEC4 },
	{ "modelOrigin", Register::VEC4 },
	{ "lightOrigin", Register::VEC4 },
	{ "lightColor", Register::VEC4 },
	{ "viewOffset", Register::VEC4 },
	{ "hideCursor", Register::BOOL}
};

const int Window::NumRegisterVars = sizeof(RegisterVars) / sizeof(RegEntry);

const char *Window::ScriptNames[] = {
	"onMouseEnter",
	"onMouseExit",
	"onAction",
	"onActivate",
	"onDeactivate",
	"onESC",
	"onEvent",
	"onTrigger",
	"onActionRelease",
	"onEnter",
	"onEnterRelease"
};

void Window::CommonInit()
{
	_childID = 0;
	_flags = 0;
	_lastTimeRun = 0;
	_origin.x = _origin.y = 0;
	_font = renderSystem->RegisterFont("");
	_timeLine = -1;
	_xOffset = _yOffset = 0.0;
	_cursor = 0;
	_forceAspectWidth = 640;
	_forceAspectHeight = 480;
	_matScalex = 1;
	_matScaley = 1;
	_borderSize = 0;
	_noTime = false;
	_visible = true;
	_textAlign = 0;
	_textAlignx = 0;
	_textAligny = 0;
	_noEvents = false;
	_rotate = 0;
	_shear.x = _shear.y = 0;
	_textScale = 0.35f;
	_backColor.Zero();
	_foreColor = vec4(1, 1, 1, 1);
	_hoverColor = vec4(1, 1, 1, 1);
	_matColor = vec4(1, 1, 1, 1);
	_borderColor.Zero();
	_background = nullptr;
	_backGroundName = "";
	_focusedChild = nullptr;
	_captureChild = nullptr;
	_overChild = nullptr;
	_parent = nullptr;
	_saveOps = nullptr;
	_saveRegs = nullptr;
	_timeLine = -1;
	_textShadow = 0;
	_hover = false;
	for (int i = 0; i < SCRIPT_COUNT; i++)
		_scripts[i] = nullptr;
	_hideCursor = false;
}

size_t Window::Size() {
	int c = _children.size();
	int sz = 0;
	for (int i = 0; i < c; i++)
		sz += _children[i]->Size();
	sz += sizeof(*this) + Allocated();
	return sz;
}

size_t Window::Allocated() {
	int i, c;
	int sz = _name.capacity();
	sz += _text.Size();
	sz += _backGroundName.Size();
	c = _definedVars.size();
	for (i = 0; i < c; i++)
		sz += _definedVars[i]->Size();
	for (i = 0; i < SCRIPT_COUNT; i++)
		if (_scripts[i])
			sz += _scripts[i]->Size();
	c = _timeLineEvents.size();
	for (i = 0; i < c; i++)
		sz += _timeLineEvents[i]->Size();
	c = _namedEvents.size();
	for (i = 0; i < c; i++)
		sz += _namedEvents[i]->Size();
	c = _drawWindows.size();
	for (i = 0; i < c; i++)
		if (_drawWindows[i].simp)
			sz += _drawWindows[i].simp->Size();
	return sz;
}

Window::Window(UserInterfaceLocal *ui) {
	_gui = ui;
	CommonInit();
}

void Window::CleanUp() {
	int i, c = _drawWindows.size();
	for (i = 0; i < c; i++)
		delete _drawWindows[i].simp;

	_regList.Reset(); // ensure the register list gets cleaned up
	_namedEvents.DeleteContents(true); // Cleanup the named events

	_drawWindows.clear();
	_children.DeleteContents(true);
	_definedVars.DeleteContents(true);
	_timeLineEvents.DeleteContents(true);
	for (i = 0; i < SCRIPT_COUNT; i++)
		delete _scripts[i];
	CommonInit();
}

Window::~Window() {
	CleanUp();
}

void Window::Move(float x, float y) {
	Rectangle rct = _rect;
	rct.x = x;
	rct.y = y;
	Register *reg = RegList()->FindReg("rect");
	if (reg)
		reg->Enable(false);
	_rect = rct;
}

void Window::SetFont() {
	_dc->SetFont(font);
}

float Window::GetMaxCharHeight() {
	SetFont();
	return _dc->MaxCharHeight(textScale);
}

float Window::GetMaxCharWidth() {
	SetFont();
	return _dc->MaxCharWidth(textScale);
}

void Window::Draw(int time, float x, float y) {
	if (_text.Length() == 0)
		return;
	if (_textShadow) {
		string shadowText = _text;
		Rectangle shadowRect = _textRect;

		shadowText.RemoveColors();
		shadowRect.x += textShadow;
		shadowRect.y += textShadow;

		_dc->DrawText(shadowText, _textScale, _textAlign, colorBlack, shadowRect, !(_flags & WIN_NOWRAP), -1);
	}
	_dc->DrawText(text, textScale, _textAlign, foreColor, _textRect, !(_flags & WIN_NOWRAP), -1);

	if (c_gui_edit) {
		_dc->EnableClipping(false);
		_dc->DrawText(va("x: %i  y: %i", (int)_rect.x(), (int)_rect.y()), 0.25, 0, _dc->colorWhite, Rectangle(_rect.x(), _rect.y() - 15, 100, 20), false);
		_dc->DrawText(va("w: %i  h: %i", (int)_rect.w(), (int)_rect.h()), 0.25, 0, _dc->colorWhite, Rectangle(_rect.x() + _rect.w(), _rect.w() + _rect.h() + 5, 100, 20), false);
		_dc->EnableClipping(true);
	}
}

void Window::BringToTop(Window *w) {
	if (w && !(w->_flags & WIN_MODAL))
		return;
	int c = _children.size();
	for (int i = 0; i < c; i++) {
		if (_children[i] == w) {
			// this is it move from i - 1 to 0 to i to 1 then shove this one into 0
			for (int j = i + 1; j < c; j++)
				_children[j - 1] = _children[j];
			_children[c - 1] = w;
			break;
		}
	}
}

void Window::Size(float x, float y, float w, float h) {
	Rectangle rct = _rect;
	rct.x = x;
	rct.y = y;
	rct.w = w;
	rct.h = h;
	rect = rct;
	CalcClientRect(0, 0);
}

void Window::MouseEnter() {
	if (noEvents)
		return;
	RunScript(ON_MOUSEENTER);
}

void Window::MouseExit() {
	if (noEvents)
		return;
	RunScript(ON_MOUSEEXIT);
}

Window *Window::GetChildWithOnAction(float xd, float yd) {
	int c = _children.size();
	while (c > 0) {
		Window *child = _children[--c];
		if (child->_visible && child->Contains(child->_drawRect, _gui->CursorX(), _gui->CursorY()) && !child->_noEvents) {
			child->_hover = true;
			if (child->_cursor > 0)
				return child;
		}
		Window *check = child->GetChildWithOnAction(xd, yd);
		if (check && check != child)
			return check;
	}
	return this;

}

const char *Window::RouteMouseCoords(float xd, float yd) {
	string str;
	if (GetCaptureChild())
		return GetCaptureChild()->RouteMouseCoords(xd, yd); //FIXME: unkludge this whole mechanism
	if (xd == -2000 || yd == -2000)
		return "";
	Window *child = GetChildWithOnAction(xd, yd);
	if (_overChild != child) {
		if (_overChild) {
			_overChild->MouseExit();
			str = _overChild->_cmd;
			if (str.size()) {
				_gui->GetDesktop()->AddCommand(str.c_str());
				_overChild->_cmd = "";
			}
		}
		_overChild = child;
		if (_overChild) {
			_overChild->MouseEnter();
			str = _overChild->_cmd;
			if (str.size()) {
				gui->GetDesktop()->AddCommand(str.c_str());
				_overChild->_cmd = "";
			}
			_dc->SetCursor(_overChild->_cursor);
		}
	}
	return "";
}

void Window::Activate(bool activate, string &act) {
	int n = (activate ? ON_ACTIVATE : ON_DEACTIVATE);
	//  make sure win vars are updated before activation
	UpdateWinVars();
	RunScript(n);
	int c = _children.size();
	for (int i = 0; i < c; i++)
		_children[i]->Activate(activate, act);
	if (act.size())
		act += " ; ";
}

void Window::Trigger() {
	RunScript(ON_TRIGGER);
	int c = _children.size();
	for (int i = 0; i < c; i++)
		_children[i]->Trigger();
	StateChanged(true);
}

void Window::StateChanged(bool redraw) {
	UpdateWinVars();
	if (_expressionRegisters.size() && _ops.size())
		EvalRegs();

	int c = _drawWindows.size();
	for (int i = 0; i < c; i++) {
		if (_drawWindows[i].win)
			_drawWindows[i].win->StateChanged(redraw);
		else
			_drawWindows[i].simp->StateChanged(redraw);
	}
}

Window *Window::SetCapture(Window *w) {
	// only one child can have the focus
	Window *last = nullptr;
	int c = _children.size();
	for (int i = 0; i < c; i++)
		if (_children[i]->_flags & WIN_CAPTURE) {
			last = _children[i];
			//last->_flags &= ~WIN_CAPTURE;
			last->LoseCapture();
			break;
		}
	w->_flags |= WIN_CAPTURE;
	w->GainCapture();
	_gui->GetDesktop()->_captureChild = w;
	return last;
}

void Window::AddUpdateVar(WinVar *var) {
	_updateVars.AddUnique(var);
}

void Window::UpdateWinVars() {
	int c = _updateVars.size();
	for (int i = 0; i < c; i++)
		_updateVars[i]->Update();
}

bool Window::RunTimeEvents(int time) {
	if (time == _lastTimeRun)
		return false;
	_lastTimeRun = time;
	UpdateWinVars();

	if (_expressionRegisters.size() && _ops.size())
		EvalRegs();
	if (_flags & WIN_INTRANSITION)
		Transition();
	Time();

	// renamed ON_EVENT to ON_FRAME
	RunScript(ON_FRAME);
	int c = _children.size();
	for (int i = 0; i < c; i++)
		_children[i]->RunTimeEvents(time);
	return true;
}

void Window::RunNamedEvent(const char *eventName)
{
	int i;
	int c;
	// Find and run the event	
	c = _namedEvents.size();
	for (i = 0; i < c; i++) {
		if (_namedEvents[i]->name->compare(eventName))
			continue;
		UpdateWinVars();
		// Make sure we got all the current values for stuff
		if (_expressionRegisters.size() && _ops.size())
			EvalRegs(-1, true);
		RunScriptList(_namedEvents[i]->event);
		break;
	}

	// Run the event in all the children as well
	c = _children.size();
	for (i = 0; i < c; i++)
		_children[i]->RunNamedEvent(eventName);
}

bool Window::Contains(const Rectangle &sr, float x, float y) {
	Rectangle r = sr;
	r.x += _actualX - _drawRect.x;
	r.y += _actualY - _drawRect.y;
	return r.Contains(x, y);
}

bool Window::Contains(float x, float y) {
	Rectangle r = _drawRect;
	r.x = _actualX;
	r.y = _actualY;
	return r.Contains(x, y);
}

void Window::AddCommand(const char *cmd) {
	string str = string(cmd);
	if (str.size()) {
		str += " ; ";
		str += cmd;
	}
	else
		str = cmd;
	_cmd = str;
}

const char *Window::HandleEvent(const Event_t *event, bool *updateVisuals) {
	static bool actionDownRun;
	static bool actionUpRun;
	_cmd = "";
	if (_flags & WIN_DESKTOP) {
		actionDownRun = false;
		actionUpRun = false;
		if (_expressionRegisters.size() && _ops.size())
			EvalRegs();
		RunTimeEvents(_gui->GetTime());
		CalcRects(0, 0);
		if (_overChild)
			_dc->SetCursor(_overChild->_cursor);
		else
			_dc->SetCursor(DeviceContext::CURSOR_ARROW);
	}

	if (visible && !noEvents) {
		if (event->evType == SE_KEY) {
			EvalRegs(-1, true);
			if (updateVisuals)
				*updateVisuals = true;
			if (event->evValue == K_MOUSE1) {
				if (!event->evValue2 && GetCaptureChild()) {
					GetCaptureChild()->LoseCapture();
					_gui->GetDesktop()->_captureChild = nullptr;
					return "";
				}

				int c = _children.size();
				while (--c >= 0)
					if (_children[c]->_visible && _children[c]->Contains(_children[c]->_drawRect, _gui->CursorX(), _gui->CursorY()) && !(_children[c]->_noEvents)) {
						Window *child = _children[c];
						if (event->evValue2) {
							BringToTop(child);
							SetFocus(child);
							if (child->_flags & WIN_HOLDCAPTURE)
								SetCapture(child);
						}
						if (child->Contains(child->_clientRect, _gui->CursorX(), _gui->CursorY())) {
							//if ((c_gui_edit.GetBool() && (child->_flags & WIN_SELECTED)) || (!c_gui_edit.GetBool() && (child->_flags & WIN_MOVABLE)))
							//	SetCapture(child);
							SetFocus(child);
							const char *childRet = child->HandleEvent(event, updateVisuals);
							if (childRet && *childRet)
								return childRet;
							if (child->_flags & WIN_MODAL)
								return "";
						}
						else {
							if (event->evValue2) {
								SetFocus(child);
								bool capture = true;
								if (capture && ((child->_flags & WIN_MOVABLE) || c_gui_edit.GetBool()))
									SetCapture(child);
								return "";
							}
							else {}
						}
					}
				if (event->evValue2 && !actionDownRun)
					actionDownRun = RunScript(ON_ACTION);
				else if (!actionUpRun)
					actionUpRun = RunScript(ON_ACTIONRELEASE);
			}
			else if (event->evValue == K_MOUSE2) {
				if (!event->evValue2 && GetCaptureChild()) {
					GetCaptureChild()->LoseCapture();
					gui->GetDesktop()->captureChild = nullptr;
					return "";
				}

				int c = _children.size();
				while (--c >= 0)
					if (_children[c]->_visible && _children[c]->Contains(_children[c]->_drawRect, _gui->CursorX(), _gui->CursorY()) && !(_children[c]->_noEvents)) {
						Window *child = _children[c];
						if (event->evValue2) {
							BringToTop(child);
							SetFocus(child);
						}
						if (child->Contains(child->_clientRect, gui->CursorX(), gui->CursorY()) || GetCaptureChild() == child) {
							if ((c_gui_edit && (child->_flags & WIN_SELECTED)) || (!c_gui_edit && (child->_flags & WIN_MOVABLE)))
								SetCapture(child);
							const char *childRet = child->HandleEvent(event, updateVisuals);
							if (childRet && *childRet)
								return childRet;
							if (child->_flags & WIN_MODAL)
								return "";
						}
					}
			}
			else if (event->evValue == K_MOUSE3) {
				if (c_gui_edit) {
					int c = _children.size();
					for (int i = 0; i < c; i++)
						if (_children[i]->drawRect.Contains(gui->CursorX(), gui->CursorY()))
							if (event->evValue2) {
								children[i]->flags ^= WIN_SELECTED;
								if (children[i]->flags & WIN_SELECTED) {
									flags &= ~WIN_SELECTED;
									return "childsel";
								}
							}
				}
			}
			else if (event->evValue == K_TAB && event->evValue2) {
				if (GetFocusedChild()) {
					const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
					if (childRet && *childRet)
						return childRet;

					// If the window didn't handle the tab, then move the focus to the next window
					// or the previous window if shift is held down
					int direction = 1;
					if (KeyInput::IsDown(K_LSHIFT) || KeyInput::IsDown(K_RSHIFT))
						direction = -1;

					Window *currentFocus = GetFocusedChild();
					Window *child = GetFocusedChild();
					Window *parent = child->GetParent();
					while (parent) {
						bool foundFocus = false;
						bool recurse = false;
						int index = 0;
						if (child)
							index = parent->GetChildIndex(child) + direction;
						else if (direction < 0)
							index = parent->GetChildCount() - 1;
						while (index < parent->GetChildCount() && index >= 0) {
							Window *testWindow = parent->GetChild(index);
							if (testWindow == currentFocus) {
								// we managed to wrap around and get back to our starting window
								foundFocus = true;
								break;
							}
							if (testWindow && !testWindow->_noEvents && testWindow->_visible) {
								if (testWindow->_flags & WIN_CANFOCUS) {
									SetFocus(testWindow);
									foundFocus = true;
									break;
								}
								else if (testWindow->GetChildCount() > 0) {
									parent = testWindow;
									child = nullptr;
									recurse = true;
									break;
								}
							}
							index += direction;
						}
						if (foundFocus)
							break; // We found a child to focus on
						else if (recurse)
							continue; // We found a child with children
						else {
							// We didn't find anything, so go back up to our parent
							child = parent;
							parent = child->GetParent();
							if (parent == _gui->GetDesktop()) {
								// We got back to the desktop, so wrap around but don't actually go to the desktop
								parent = nullptr;
								child = nullptr;
							}
						}
					}
				}
			}
			else if ((event->evValue == K_ESCAPE || event->evValue == K_JOY9) && event->evValue2) {
				if (GetFocusedChild()) {
					const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
					if (childRet && *childRet)
						return childRet;
				}
				RunScript(ON_ESC);
			}
			else if (event->evValue == K_ENTER) {
				if (GetFocusedChild()) {
					const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
					if (childRet && *childRet)
						return childRet;
				}
				if (_flags & WIN_WANTENTER) {
					if (event->evValue2)
						RunScript(ON_ACTION);
					else
						RunScript(ON_ACTIONRELEASE);
				}
			}
			else {
				if (GetFocusedChild()) {
					const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
					if (childRet && *childRet)
						return childRet;
				}
			}

		}
		else if (event->evType == SE_MOUSE) {
			if (updateVisuals)
				*updateVisuals = true;
			const char *mouseRet = RouteMouseCoords(event->evValue, event->evValue2);
			if (mouseRet && *mouseRet)
				return mouseRet;
		}
		else if (event->evType == SE_NONE) {
		}
		else if (event->evType == SE_CHAR) {
			if (GetFocusedChild()) {
				const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
				if (childRet && *childRet)
					return childRet;
			}
		}
	}

	_gui->GetReturnCmd() = _cmd;
	if (_gui->GetPendingCmd().size()) {
		_gui->GetReturnCmd() += " ; ";
		_gui->GetReturnCmd() += _gui->GetPendingCmd();
		_gui->GetPendingCmd().clear();
	}
	_cmd = "";
	return _gui->GetReturnCmd();
}

void Window::DebugDraw(int time, float x, float y) {
	static char buff[16384] = { 0 };
	if (_dc) {
		_dc->EnableClipping(false);
		if (c_gui_debug == 1)
			_dc->DrawRect(_drawRect.x, _drawRect.y, _drawRect.w, _drawRect.h, 1, DeviceContext::colorRed);
		else if (c_gui_debug == 2) {
			char out[1024];
			string str = text.c_str();
			if (str.size())
				sprintf(buff, "%s\n", str.c_str());
			sprintf(out, "Rect: %0.1f, %0.1f, %0.1f, %0.1f\n", _rect.x(), _rect.y(), _rect.w(), _rect.h());
			strcat(buff, out);
			sprintf(out, "Draw Rect: %0.1f, %0.1f, %0.1f, %0.1f\n", _drawRect.x, _drawRect.y, _drawRect.w, _drawRect.h);
			strcat(buff, out);
			sprintf(out, "Client Rect: %0.1f, %0.1f, %0.1f, %0.1f\n", _clientRect.x, _clientRect.y, _clientRect.w, _clientRect.h);
			strcat(buff, out);
			sprintf(out, "Cursor: %0.1f : %0.1f\n", _gui->CursorX(), _gui->CursorY());
			strcat(buff, out);

			//Rectangle tempRect = textRect;
			//tempRect.x += offsetX;
			//drawRect.y += offsetY;
			_dc->DrawText(buff, textScale, _textAlign, foreColor, _textRect, true);
		}
		_dc->EnableClipping(true);
	}
}

void Window::Transition() {
	int i, c = _transitions.size();
	bool clear = true;

	for (i = 0; i < c; i++) {
		TransitionData *data = &_transitions[i];
		WinRectangle *r = nullptr;
		WinVec4 *v4 = dynamic_cast<WinVec4 *>(data->data);
		WinFloat* val = nullptr;
		if (!v4) {
			r = dynamic_cast<WinRectangle *>(data->data);
			if (!r)
				val = dynamic_cast<WinFloat *>(data->data);
		}
		if (data->interp.IsDone(_gui->GetTime()) && data->data) {
			if (v4)
				*v4 = data->interp.GetEndValue();
			else if (val)
				*val = data->interp.GetEndValue()[0];
			else if (r != nullptr)
				*r = data->interp.GetEndValue();
		}
		else {
			clear = false;
			if (data->data) {
				if (v4)
					*v4 = data->interp.GetCurrentValue(_gui->GetTime());
				else if (val)
					*val = data->interp.GetCurrentValue(_gui->GetTime())[0];
				else if (r != nullptr)
					*r = data->interp.GetCurrentValue(_gui->GetTime());
			}
			else
				common->Warning("Invalid transitional data for window %s in gui %s", GetName(), _gui->GetSourceFile());
		}
	}

	if (clear) {
		_transitions.resize(0);
		_flags &= ~WIN_INTRANSITION;
	}
}

void Window::Time() {
	if (noTime)
		return;
	if (_timeLine == -1)
		_timeLine = _gui->GetTime();
	_cmd = "";

	int c = _timeLineEvents.size();
	if (c > 0)
		for (int i = 0; i < c; i++)
			if (_timeLineEvents[i]->pending && _gui->GetTime() - _timeLine >= _timeLineEvents[i]->time) {
				_timeLineEvents[i]->pending = false;
				RunScriptList(_timeLineEvents[i]->event);
			}
	if (_gui->Active()) {
		if (_gui->GetPendingCmd().size() > 0)
			_gui->GetPendingCmd() += ";";
		_gui->GetPendingCmd() += _cmd;
	}
}

float Window::EvalRegs(int test, bool force) {
	static float regs[MAX_EXPRESSION_REGISTERS];
	static Window *lastEval = nullptr;

	if (!force && test >= 0 && test < MAX_EXPRESSION_REGISTERS && lastEval == this)
		return regs[test];
	lastEval = this;

	if (_expressionRegisters.size()) {
		_regList.SetToRegs(regs);
		EvaluateRegisters(regs);
		_regList.GetFromRegs(regs);
	}

	if (test >= 0 && test < MAX_EXPRESSION_REGISTERS)
		return regs[test];
	return 0.0;
}

void Window::DrawBackground(const Rectangle &drawRect) {
	if (_backColor.w())
		_dc->DrawFilledRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, _backColor);
	if (_background && _matColor.w()) {
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

void Window::DrawBorderAndCaption(const Rectangle &drawRect) {
	if (_flags & WIN_BORDER && _borderSize && _borderColor.w())
		_dc->DrawRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, _borderSize, borderColor);
}

void Window::SetupTransforms(float x, float y) {
	static mat3 trans;
	static vec3 org;

	trans.Identity();
	org.Set(origin.x + x, origin.y + y, 0);

	if (rotate) {
		static Rotation rot;
		static vec3 vec(0, 0, 1);
		rot.Set(org, vec, rotate);
		trans = rot.ToMat3();
	}

	if (_shear.x || _shear.y) {
		static mat3 smat;
		smat.Identity();
		smat[0][1] = _shear.x;
		smat[1][0] = _shear.y;
		trans *= smat;
	}

	if (!trans.IsIdentity())
		_dc->SetTransformInfo(org, trans);
}

void Window::CalcRects(float x, float y) {
	CalcClientRect(0, 0);
	_drawRect.Offset(x, y);
	_clientRect.Offset(x, y);
	_actualX = _drawRect.x;
	_actualY = _drawRect.y;
	int c = _drawWindows.size();
	for (int i = 0; i < c; i++)
		if (_drawWindows[i].win)
			_drawWindows[i].win->CalcRects(_clientRect.x + _xOffset, _clientRect.y + _yOffset);
	_drawRect.Offset(-x, -y);
	_clientRect.Offset(-x, -y);
}

void Window::Redraw(float x, float y, bool hud) {
	string str;
	if (c_r_skipGuiShaders == 1 || !_dc)
		return;
	int time = _gui->GetTime();
	if (_flags & WIN_DESKTOP && c_r_skipGuiShaders != 3)
		RunTimeEvents(time);
	if (c_r_skipGuiShaders == 2)
		return;
	if (_flags & WIN_SHOWTIME)
		_dc->DrawText(va(" %0.1f seconds\n%s", (float)(time - _timeLine) / 1000, _gui->State().GetString("name")), 0.35f, 0, _dc->colorWhite, Rectangle(100, 0, 80, 80), false);
	if (_flags & WIN_SHOWCOORDS) {
		_dc->EnableClipping(false);
		sprintf(str, "x: %i y: %i  cursorx: %i cursory: %i", (int)_rect.x(), (int)_rect.y(), (int)_gui->CursorX(), (int)_gui->CursorY());
		_dc->DrawText(str, 0.25f, 0, _dc->colorWhite, Rectangle(0, 0, 100, 20), false);
		_dc->EnableClipping(true);
	}

	if (!visible)
		return;
	CalcClientRect(0, 0);

	SetFont();

	if (hud) {
		float tileSafeOffset = c_hud_titlesafe.GetFloat();
		float tileSafeScale = 1.0f / (1.0f - c_hud_titlesafe.GetFloat() * 2.0f);
		_dc->SetSize(_forceAspectWidth * tileSafeScale, _forceAspectHeight * tileSafeScale);
		_dc->SetOffset(_forceAspectWidth * tileSafeOffset, _forceAspectHeight * tileSafeOffset);
	}
	else {
		_dc->SetSize(_forceAspectWidth, _forceAspectHeight);
		_dc->SetOffset(0.0f, 0.0f);
	}

	//FIXME: go to screen coord tracking
	_drawRect.Offset(x, y);
	_clientRect.Offset(x, y);
	_textRect.Offset(x, y);
	_actualX = _drawRect.x;
	_actualY = _drawRect.y;

	vec3 oldOrg;
	mat3 oldTrans;
	_dc->GetTransformInfo(oldOrg, oldTrans);

	SetupTransforms(x, y);
	DrawBackground(_drawRect);
	DrawBorderAndCaption(_drawRect);

	if (!(_flags & WIN_NOCLIP))
		_dc->PushClipRect(_clientRect);

	if (c_r_skipGuiShaders < 5)
		Draw(time, x, y);
	if (c_gui_debug)
		DebugDraw(time, x, y);

	int c = _drawWindows.size();
	for (int i = 0; i < c; i++) {
		if (_drawWindows[i].win)
			_drawWindows[i].win->Redraw(_clientRect.x + _xOffset, _clientRect.y + _yOffset, hud);
		else
			_drawWindows[i].simp->Redraw(_clientRect.x + _xOffset, _clientRect.y + _yOffset);
	}

	// Put transforms back to what they were before the children were processed
	_dc->SetTransformInfo(oldOrg, oldTrans);

	if (!(_flags & WIN_NOCLIP))
		_dc->PopClipRect();

	if (c_gui_edit || (_flags & WIN_DESKTOP && !(_flags & WIN_NOCURSOR) && !hideCursor && (_gui->Active() || (_flags & WIN_MENUGUI)))) {
		_dc->SetTransformInfo(vec3_origin, mat3_identity);
		_gui->DrawCursor();
	}

	if (c_gui_debug && _flags & WIN_DESKTOP) {
		_dc->EnableClipping(false);
		sprintf(str, "x: %1.f y: %1.f", _gui->CursorX(), _gui->CursorY());
		_dc->DrawText(str, 0.25, 0, _dc->colorWhite, Rectangle(0, 0, 100, 20), false);
		_dc->DrawText(_gui->GetSourceFile(), 0.25, 0, _dc->colorWhite, Rectangle(0, 20, 300, 20), false);
		_dc->EnableClipping(true);
	}

	_drawRect.Offset(-x, -y);
	_clientRect.Offset(-x, -y);
	_textRect.Offset(-x, -y);
}

void Window::ArchiveToDictionary(Dict *dict, bool useNames) {
	// FIXME: rewrite without state
	int c = _children.size();
	for (int i = 0; i < c; i++)
		_children[i]->ArchiveToDictionary(dict);
}

void Window::InitFromDictionary(Dict *dict, bool byName) {
	// FIXME: rewrite without state
	int c = _children.size();
	for (int i = 0; i < c; i++)
		_children[i]->InitFromDictionary(dict);
}

void Window::CalcClientRect(float xofs, float yofs) {
	_drawRect = _rect;

	if (_flags & WIN_INVERTRECT) {
		_drawRect.x = _rect.x() - _rect.w();
		_drawRect.y = _rect.y() - _rect.h();
	}

	if (_flags & (WIN_HCENTER | WIN_VCENTER) && _parent) {
		// in this case treat xofs and yofs as absolute top left coords and ignore the original positioning
		if (_flags & WIN_HCENTER)
			_drawRect.x = (_parent->_rect.w() - _rect.w()) / 2;
		else
			_drawRect.y = (_parent->_rect.h() - _rect.h()) / 2;
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

void Window::SetupBackground() {
	if (_backGroundName.Length()) {
		_background = g_declManager->FindMaterial(backGroundName);
		if (_background != nullptr && !background->TestMaterialFlag(MF_DEFAULTED))
			_background->SetSort(SS_GUI);
	}
	backGroundName.SetMaterialPtr(&background);
}

void Window::SetupFromState() {
	_background = nullptr;
	SetupBackground();
	if (_borderSize)
		_flags |= WIN_BORDER;
	if (_regList.FindReg("rotate") || _regList.FindReg("shear"))
		_flags |= WIN_TRANSFORM;
	CalcClientRect(0, 0);
	if (_scripts[ON_ACTION]) {
		_cursor = DeviceContext::CURSOR_HAND;
		_flags |= WIN_CANFOCUS;
	}
}

void Window::Moved() {
}

void Window::Sized() {
}

void Window::GainFocus() {
}

void Window::LoseFocus() {
}

void Window::GainCapture() {
}

void Window::LoseCapture() {
	_flags &= ~WIN_CAPTURE;
}

void Window::SetFlag(unsigned int f) {
	_flags |= f;
}

void Window::ClearFlag(unsigned int f) {
	_flags &= ~f;
}

void Window::SetParent(Window *w) {
	_parent = w;
}

Window *Window::GetCaptureChild() {
	return (_flags & WIN_DESKTOP ? _gui->GetDesktop()->_captureChild : nullptr);
}

Window *Window::GetFocusedChild() {
	return (_flags & WIN_DESKTOP ? _gui->GetDesktop()->_focusedChild : nullptr);
}

Window *Window::SetFocus(Window *w, bool scripts) {
	// only one child can have the focus
	Window *lastFocus = nullptr;
	if (w->_flags & WIN_CANFOCUS) {
		lastFocus = _gui->GetDesktop()->_focusedChild;
		if (lastFocus) {
			lastFocus->_flags &= ~WIN_FOCUS;
			lastFocus->LoseFocus();
		}
		//  call on lose focus
		//if (scripts && lastFocus) lastFocus->RunScript(ON_MOUSEEXIT); // calling this broke all sorts of guis
		//  call on gain focus
		//if (scripts && w) w->RunScript(ON_MOUSEENTER); // calling this broke all sorts of guis
		w->_flags |= WIN_FOCUS;
		w->GainFocus();
		_gui->GetDesktop()->_focusedChild = w;
	}
	return lastFocus;
}

bool Window::ParseScript(TokenParser *src, GuiScriptList &list, int *timeParm, bool elseBlock) {
	bool ifElseBlock = false;
	Token token;

	// scripts start with { ( unless parm is true ) and have ; separated command lists.. commands are command,
	// arg.. basically we want everything between the { } as it will be interpreted at run time

	if (elseBlock) {
		src->ReadToken(&token);
		if (!token.compare("if"))
			ifElseBlock = true;
		src->UnreadToken(&token);
		if (!ifElseBlock && !src->ExpectTokenString("{"))
			return false;
	}
	else if (!src->ExpectTokenString("{"))
		return false;
	int nest = 0;
	while (1) {
		if (!src->ReadToken(&token)) {
			src->Error("Unexpected end of file");
			return false;
		}
		if (token == "{")
			nest++;
		if (token == "}")
			if (nest-- <= 0)
				return true;
		GuiScript *gs = new GuiScript();
		if (!token.compare("if")) {
			gs->_conditionReg = ParseExpression(src);
			gs->_ifList = new GuiScriptList();
			ParseScript(src, *gs->_ifList, nullptr);
			if (src->ReadToken(&token)) {
				if (token == "else") {
					gs->_elseList = new GuiScriptList();
					// pass true to indicate we are parsing an else condition
					ParseScript(src, *gs->_elseList, nullptr, true);
				}
				else
					src->UnreadToken(&token);
			}
			list.Append(gs);
			// if we are parsing an else if then return out so the initial "if" parser can handle the rest of the tokens
			if (ifElseBlock)
				return true;
			continue;
		}
		else
			src->UnreadToken(&token);
		// empty { } is not allowed
		if (token == "{") {
			src->Error("Unexpected {");
			delete gs;
			return false;
		}
		gs->Parse(src);
		list.Append(gs);
	}
}

void Window::SaveExpressionParseState() {
	_saveTemps = (bool *)Mem_Alloc(MAX_EXPRESSION_REGISTERS * sizeof(bool), TAG_CRAP);
	memcpy(_saveTemps, registerIsTemporary, MAX_EXPRESSION_REGISTERS * sizeof(bool));
}

void Window::RestoreExpressionParseState() {
	memcpy(registerIsTemporary, _saveTemps, MAX_EXPRESSION_REGISTERS * sizeof(bool));
	Mem_Free(_saveTemps);
}

bool Window::ParseScriptEntry(const char *name, TokenParser *src) {
	for (int i = 0; i < SCRIPT_COUNT; i++)
		if (!strcmp(name, ScriptNames[i])) {
			delete _scripts[i];
			_scripts[i] = new GuiScriptList();
			return ParseScript(src, *_scripts[i]);
		}
	return false;
}

void Window::DisableRegister(const char *name) {
	Register *reg = RegList()->FindReg(name);
	if (reg)
		reg->Enable(false);
}

class Sort_TimeLine : public Sort_Quick<TimeLineEvent *, Sort_TimeLine> {
public:
	int Compare(TimeLineEvent *const &a, TimeLineEvent *const &b) const { return a->time - b->time; }
};

void Window::PostParse() {
	// Sort timeline events
	Sort_TimeLine sorter;
	_timeLineEvents.SortWithTemplate(sorter);
}

int Window::GetWinVarOffset(WinVar *wv, drawWin_t *owner) {
	int ret = -1;
	if (wv == &_rect)
		ret = (int)&((Window *)0)->_rect;
	if (wv == &_backColor)
		ret = (int)&((Window *)0)->_backColor;
	if (wv == &_matColor)
		ret = (int)&((Window *)0)->_matColor;
	if (wv == &_foreColor)
		ret = (int)&((Window *)0)->_foreColor;
	if (wv == &_hoverColor)
		ret = (int)&((Window *)0)->_hoverColor;
	if (wv == &_borderColor)
		ret = (int)&((Window *)0)->_borderColor;
	if (wv == &_textScale)
		ret = (int)&((Window *)0)->_textScale;
	if (wv == &_rotate)
		ret = (int)&((Window *)0)->_rotate;
	if (ret != -1) {
		owner->win = this;
		return ret;
	}
	for (int i = 0; i < _drawWindows.size(); i++) {
		if (_drawWindows[i].win)
			ret = _drawWindows[i].win->GetWinVarOffset(wv, owner);
		else
			ret = _drawWindows[i].simp->GetWinVarOffset(wv, owner);
		if (ret != -1)
			break;
	}
	return ret;
}

WinVar *Window::GetWinVarByName(const char *name, bool fixup, drawWin_t **owner) {
	WinVar *retVar = nullptr;
	if (owner)
		*owner = nullptr;
	if (!strcmp(name, "notime")) retVar = &_noTime;
	if (!strcmp(name, "background")) retVar = &_backGroundName;
	if (!strcmp(name, "visible")) retVar = &_visible;
	if (!strcmp(name, "rect")) retVar = &_rect;
	if (!strcmp(name, "backColor")) retVar = &_backColor;
	if (!strcmp(name, "matColor")) retVar = &_matColor;
	if (!strcmp(name, "foreColor")) retVar = &_foreColor;
	if (!strcmp(name, "hoverColor")) retVar = &_hoverColor;
	if (!strcmp(name, "borderColor")) retVar = &_borderColor;
	if (!strcmp(name, "textScale")) retVar = &_textScale;
	if (!strcmp(name, "rotate")) retVar = &_rotate;
	if (!strcmp(name, "noEvents")) retVar = &_noEvents;
	if (!strcmp(name, "text"))  retVar = &_text;
	if (!strcmp(name, "backGroundName")) retVar = &_backGroundName;
	if (!strcmp(name, "hidecursor")) retVar = &_hideCursor;
	string key = name;
	bool guiVar = (key.Find(VAR_GUIPREFIX) >= 0);
	int c = _definedVars.size();
	for (int i = 0; i < c; i++)
		if (!strcmp(name, guiVar ? va("%s", _definedVars[i]->GetName()) : _definedVars[i]->GetName())) {
			retVar = definedVars[i];
			break;
		}
	if (retVar) {
		if (fixup && *name != '$')
			DisableRegister(name);
		if (owner && _parent)
			*owner = _parent->FindChildByName(_name);
		return retVar;
	}

	int len = key.size();
	if (len > 5 && guiVar) {
		WinVar *var = new WinStr();
		var->Init(name, this);
		_definedVars.push_back(var);
		return var;
	}
	else if (fixup) {
		int n = key.Find("::");
		if (n > 0) {
			string winName = key.Left(n);
			string var = key.Right(key.size() - n - 2);
			drawWin_t *win = GetGui()->GetDesktop()->FindChildByName(winName);
			if (win) {
				if (win->win)
					return win->win->GetWinVarByName(var, false, owner);
				else {
					if (owner)
						*owner = win;
					return win->simp->GetWinVarByName(var);
				}
			}
		}
	}
	return nullptr;
}

void Window::ParseString(TokenParser *src, string &out) {
	Token tok;
	if (src->ReadToken(&tok))
		out = tok;
}

void Window::ParseVec4(TokenParser *src, vec4 &out) {
	Token tok;
	src->ReadToken(&tok);
	out.x = atof(tok);
	src->ExpectTokenString(",");
	src->ReadToken(&tok);
	out.y = atof(tok);
	src->ExpectTokenString(",");
	src->ReadToken(&tok);
	out.z = atof(tok);
	src->ExpectTokenString(",");
	src->ReadToken(&tok);
	out.w = atof(tok);
}

bool Window::ParseInternalVar(const char *name, TokenParser *src) {
	if (!strcmp(name, "showtime")) {
		if (src->ParseBool())
			_flags |= WIN_SHOWTIME;
		return true;
	}
	if (!strcmp(name, "showcoords")) {
		if (src->ParseBool())
			flags |= WIN_SHOWCOORDS;
		return true;
	}
	if (!strcmp(name, "forceaspectwidth")) {
		_forceAspectWidth = src->ParseFloat();
		return true;
	}
	if (!strcmp(name, "forceaspectheight")) {
		_forceAspectHeight = src->ParseFloat();
		return true;
	}
	if (!strcmp(name, "matscalex")) {
		_matScalex = src->ParseFloat();
		return true;
	}
	if (!strcmp(name, "matscaley")) {
		_matScaley = src->ParseFloat();
		return true;
	}
	if (!strcmp(name, "bordersize")) {
		_borderSize = src->ParseFloat();
		return true;
	}
	if (!strcmp(name, "nowrap")) {
		if (src->ParseBool())
			_flags |= WIN_NOWRAP;
		return true;
	}
	if (idStr::Icmp(_name, "shadow") == 0) {
		textShadow = src->ParseInt();
		return true;
	}
	if (idStr::Icmp(_name, "textalign") == 0) {
		textAlign = src->ParseInt();
		return true;
	}
	if (idStr::Icmp(_name, "textalignx") == 0) {
		textAlignx = src->ParseFloat();
		return true;
	}
	if (idStr::Icmp(_name, "textaligny") == 0) {
		textAligny = src->ParseFloat();
		return true;
	}
	if (idStr::Icmp(_name, "shear") == 0) {
		shear.x = src->ParseFloat();
		idToken tok;
		src->ReadToken(&tok);
		if (tok.Icmp(",")) {
			src->Error("Expected comma in shear definiation");
			return false;
		}
		shear.y = src->ParseFloat();
		return true;
	}
	if (idStr::Icmp(_name, "wantenter") == 0) {
		if (src->ParseBool()) {
			flags |= WIN_WANTENTER;
		}
		return true;
	}
	if (idStr::Icmp(_name, "naturalmatscale") == 0) {
		if (src->ParseBool()) {
			flags |= WIN_NATURALMAT;
		}
		return true;
	}
	if (idStr::Icmp(_name, "noclip") == 0) {
		if (src->ParseBool()) {
			flags |= WIN_NOCLIP;
		}
		return true;
	}
	if (idStr::Icmp(_name, "nocursor") == 0) {
		if (src->ParseBool()) {
			flags |= WIN_NOCURSOR;
		}
		return true;
	}
	if (idStr::Icmp(_name, "menugui") == 0) {
		if (src->ParseBool()) {
			flags |= WIN_MENUGUI;
		}
		return true;
	}
	if (idStr::Icmp(_name, "modal") == 0) {
		if (src->ParseBool()) {
			flags |= WIN_MODAL;
		}
		return true;
	}
	if (idStr::Icmp(_name, "invertrect") == 0) {
		if (src->ParseBool()) {
			flags |= WIN_INVERTRECT;
		}
		return true;
	}
	if (idStr::Icmp(_name, "name") == 0) {
		ParseString(src, name);
		return true;
	}
	if (idStr::Icmp(_name, "play") == 0) {
		common->Warning("play encountered during gui parse.. see Robert\n");
		idStr playStr;
		ParseString(src, playStr);
		return true;
	}
	if (idStr::Icmp(_name, "comment") == 0) {
		ParseString(src, comment);
		return true;
	}
	if (idStr::Icmp(_name, "font") == 0) {
		idStr fontName;
		ParseString(src, fontName);
		font = renderSystem->RegisterFont(fontName);
		return true;
	}
	return false;
}

/*
================
idWindow::ParseRegEntry
================
*/
bool idWindow::ParseRegEntry(const char *name, idTokenParser *src) {
	idStr work;
	work = name;
	work.ToLower();

	idWinVar *var = GetWinVarByName(work, nullptr);
	if (var) {
		for (int i = 0; i < NumRegisterVars; i++) {
			if (idStr::Icmp(work, RegisterVars[i].name) == 0) {
				regList.AddReg(work, RegisterVars[i].type, src, this, var);
				return true;
			}
		}
	}

	// not predefined so just read the next token and add it to the state
	idToken tok;
	idVec4 v;
	idWinInt *vari;
	idWinFloat *varf;
	idWinStr *vars;
	if (src->ReadToken(&tok)) {
		if (var) {
			var->Set(tok);
			return true;
		}
		switch (tok.type) {
		case TT_NUMBER:
			if (tok.subtype & TT_INTEGER) {
				vari = new (TAG_OLD_UI) idWinInt();
				*vari = atoi(tok);
				vari->SetName(work);
				definedVars.Append(vari);
			}
			else if (tok.subtype & TT_FLOAT) {
				varf = new (TAG_OLD_UI) idWinFloat();
				*varf = atof(tok);
				varf->SetName(work);
				definedVars.Append(varf);
			}
			else {
				vars = new (TAG_OLD_UI) idWinStr();
				*vars = tok;
				vars->SetName(work);
				definedVars.Append(vars);
			}
			break;
		default:
			vars = new (TAG_OLD_UI) idWinStr();
			*vars = tok;
			vars->SetName(work);
			definedVars.Append(vars);
			break;
		}
	}

	return true;
}

/*
================
idWindow::SetInitialState
================
*/
void idWindow::SetInitialState(const char *_name) {
	name = _name;
	matScalex = 1.0;
	matScaley = 1.0;
	forceAspectWidth = 640.0;
	forceAspectHeight = 480.0;
	noTime = false;
	visible = true;
	flags = 0;
}

/*
================
idWindow::Parse
================
*/
bool idWindow::Parse(idTokenParser *src, bool rebuild) {
	idToken token, token2, token3, token4, token5, token6, token7;
	idStr work;

	if (rebuild) {
		CleanUp();
	}

	drawWin_t dwt;

	timeLineEvents.Clear();
	transitions.Clear();

	namedEvents.DeleteContents(true);

	src->ExpectTokenType(TT_NAME, 0, &token);

	SetInitialState(token);

	src->ExpectTokenString("{");
	src->ExpectAnyToken(&token);

	bool ret = true;

	while (token != "}") {
		// track what was parsed so we can maintain it for the guieditor
		src->SetMarker();

		if (token == "windowDef" || token == "animationDef") {
			if (token == "animationDef") {
				visible = false;
				rect = idRectangle(0, 0, 0, 0);
			}
			src->ExpectTokenType(TT_NAME, 0, &token);
			token2 = token;
			src->UnreadToken(&token);
			drawWin_t *dw = FindChildByName(token2.c_str());
			if (dw != nullptr && dw->win != nullptr) {
				SaveExpressionParseState();
				dw->win->Parse(src, rebuild);
				RestoreExpressionParseState();
			}
			else {
				idWindow *win = new (TAG_OLD_UI) idWindow(gui);
				SaveExpressionParseState();
				win->Parse(src, rebuild);
				RestoreExpressionParseState();
				win->SetParent(this);
				dwt.simp = nullptr;
				dwt.win = nullptr;
				if (win->IsSimple()) {
					idSimpleWindow *simple = new (TAG_OLD_UI) idSimpleWindow(win);
					dwt.simp = simple;
					drawWindows.Append(dwt);
					delete win;
				}
				else {
					AddChild(win);
					SetFocus(win, false);
					dwt.win = win;
					drawWindows.Append(dwt);
				}
			}
		}
		else if (token == "editDef") {
			idEditWindow *win = new (TAG_OLD_UI) idEditWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "choiceDef") {
			idChoiceWindow *win = new (TAG_OLD_UI) idChoiceWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "sliderDef") {
			idSliderWindow *win = new (TAG_OLD_UI) idSliderWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "bindDef") {
			idBindWindow *win = new (TAG_OLD_UI) idBindWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "listDef") {
			idListWindow *win = new (TAG_OLD_UI) idListWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "fieldDef") {
			idFieldWindow *win = new (TAG_OLD_UI) idFieldWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "renderDef") {
			// D3 could render a 3D model in a subrect of a full screen
			// GUI for the main menus, but we have cut that ability so
			// we don't need to deal with offset viewports on all platforms.
			idRenderWindow *win = new (TAG_OLD_UI) idRenderWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "gameSSDDef") {
			idGameSSDWindow *win = new (TAG_OLD_UI) idGameSSDWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "gameBearShootDef") {
			idGameBearShootWindow *win = new (TAG_OLD_UI) idGameBearShootWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if (token == "gameBustOutDef") {
			idGameBustOutWindow *win = new (TAG_OLD_UI) idGameBustOutWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = nullptr;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		// 
		//  added new onEvent
		else if (token == "onNamedEvent") {
			// Read the event name
			if (!src->ReadToken(&token)) {
				src->Error("Expected event name");
				return false;
			}

			rvNamedEvent* ev = new (TAG_OLD_UI) rvNamedEvent(token);

			src->SetMarker();

			if (!ParseScript(src, *ev->mEvent)) {
				ret = false;
				break;
			}

			namedEvents.Append(ev);
		}
		else if (token == "onTime") {
			idTimeLineEvent *ev = new (TAG_OLD_UI) idTimeLineEvent;

			if (!src->ReadToken(&token)) {
				src->Error("Unexpected end of file");
				return false;
			}
			ev->time = atoi(token.c_str());

			// reset the mark since we dont want it to include the time
			src->SetMarker();

			if (!ParseScript(src, *ev->event, &ev->time)) {
				ret = false;
				break;
			}

			// this is a timeline event
			ev->pending = true;
			timeLineEvents.Append(ev);
		}
		else if (token == "definefloat") {
			src->ReadToken(&token);
			work = token;
			work.ToLower();
			idWinFloat *varf = new (TAG_OLD_UI) idWinFloat();
			varf->SetName(work);
			definedVars.Append(varf);

			// add the float to the editors wrapper dict
			// Set the marker after the float name
			src->SetMarker();

			// Read in the float 
			regList.AddReg(work, idRegister::FLOAT, src, this, varf);
		}
		else if (token == "definevec4") {
			src->ReadToken(&token);
			work = token;
			work.ToLower();
			idWinVec4 *var = new (TAG_OLD_UI) idWinVec4();
			var->SetName(work);

			// set the marker so we can determine what was parsed
			// set the marker after the vec4 name
			src->SetMarker();

			// FIXME: how about we add the var to the desktop instead of this window so it won't get deleted
			//        when this window is destoyed which even happens during parsing with simple windows ?
			//definedVars.Append(var);
			gui->GetDesktop()->definedVars.Append(var);
			gui->GetDesktop()->regList.AddReg(work, idRegister::VEC4, src, gui->GetDesktop(), var);
		}
		else if (token == "float") {
			src->ReadToken(&token);
			work = token;
			work.ToLower();
			idWinFloat *varf = new (TAG_OLD_UI) idWinFloat();
			varf->SetName(work);
			definedVars.Append(varf);

			// add the float to the editors wrapper dict
			// set the marker to after the float name
			src->SetMarker();

			// Parse the float
			regList.AddReg(work, idRegister::FLOAT, src, this, varf);
		}
		else if (ParseScriptEntry(token, src)) {

		}
		else if (ParseInternalVar(token, src)) {

		}
		else {
			ParseRegEntry(token, src);
		}
		if (!src->ReadToken(&token)) {
			src->Error("Unexpected end of file");
			ret = false;
			break;
		}
	}

	if (ret) {
		EvalRegs(-1, true);
	}

	SetupFromState();
	PostParse();

	return ret;
}

/*
================
idWindow::FindSimpleWinByName
================
*/
idSimpleWindow *idWindow::FindSimpleWinByName(const char *_name) {
	int c = drawWindows.Num();
	for (int i = 0; i < c; i++) {
		if (drawWindows[i].simp == nullptr) {
			continue;
		}
		if (idStr::Icmp(drawWindows[i].simp->name, _name) == 0) {
			return drawWindows[i].simp;
		}
	}
	return nullptr;
}

/*
================
idWindow::FindChildByName
================
*/
drawWin_t *idWindow::FindChildByName(const char *_name) {
	static drawWin_t dw;
	if (idStr::Icmp(name, _name) == 0) {
		dw.simp = nullptr;
		dw.win = this;
		return &dw;
	}
	int c = drawWindows.Num();
	for (int i = 0; i < c; i++) {
		if (drawWindows[i].win) {
			if (idStr::Icmp(drawWindows[i].win->name, _name) == 0) {
				return &drawWindows[i];
			}
			drawWin_t *win = drawWindows[i].win->FindChildByName(_name);
			if (win) {
				return win;
			}
		}
		else {
			if (idStr::Icmp(drawWindows[i].simp->name, _name) == 0) {
				return &drawWindows[i];
			}
		}
	}
	return nullptr;
}

/*
================
idWindow::GetStrPtrByName
================
*/
idStr* idWindow::GetStrPtrByName(const char *_name) {
	return nullptr;
}

/*
================
idWindow::AddTransition
================
*/
void idWindow::AddTransition(idWinVar *dest, idVec4 from, idVec4 to, int time, float accelTime, float decelTime) {
	idTransitionData data;
	data.data = dest;
	data.interp.Init(gui->GetTime(), accelTime * time, decelTime * time, time, from, to);
	transitions.Append(data);
}


/*
================
idWindow::StartTransition
================
*/
void idWindow::StartTransition() {
	flags |= WIN_INTRANSITION;
}

/*
================
idWindow::ResetCinematics
================
*/
void idWindow::ResetCinematics() {
	if (background) {
		background->ResetCinematicTime(gui->GetTime());
	}
}

/*
================
idWindow::ResetTime
================
*/
void idWindow::ResetTime(int t) {

	timeLine = gui->GetTime() - t;

	int i, c = timeLineEvents.Num();
	for (i = 0; i < c; i++) {
		if (timeLineEvents[i]->time >= t) {
			timeLineEvents[i]->pending = true;
		}
	}

	noTime = false;

	c = transitions.Num();
	for (i = 0; i < c; i++) {
		idTransitionData *data = &transitions[i];
		if (data->interp.IsDone(gui->GetTime()) && data->data) {
			transitions.RemoveIndex(i);
			i--;
			c--;
		}
	}

}


/*
================
idWindow::RunScriptList
================
*/
bool idWindow::RunScriptList(idGuiScriptList *src) {
	if (src == nullptr) {
		return false;
	}
	src->Execute(this);
	return true;
}

/*
================
idWindow::RunScript
================
*/
bool idWindow::RunScript(int n) {
	if (n >= ON_MOUSEENTER && n < SCRIPT_COUNT) {
		return RunScriptList(scripts[n]);
	}
	return false;
}

/*
================
idWindow::ExpressionConstant
================
*/
int idWindow::ExpressionConstant(float f) {
	int		i;

	for (i = WEXP_REG_NUM_PREDEFINED; i < expressionRegisters.Num(); i++) {
		if (!registerIsTemporary[i] && expressionRegisters[i] == f) {
			return i;
		}
	}
	if (expressionRegisters.Num() == MAX_EXPRESSION_REGISTERS) {
		common->Warning("expressionConstant: gui %s hit MAX_EXPRESSION_REGISTERS", gui->GetSourceFile());
		return 0;
	}

	int c = expressionRegisters.Num();
	if (i > c) {
		while (i > c) {
			expressionRegisters.Append(-9999999);
			i--;
		}
	}

	i = expressionRegisters.Append(f);
	registerIsTemporary[i] = false;
	return i;
}

/*
================
idWindow::ExpressionTemporary
================
*/
int idWindow::ExpressionTemporary() {
	if (expressionRegisters.Num() == MAX_EXPRESSION_REGISTERS) {
		common->Warning("expressionTemporary: gui %s hit MAX_EXPRESSION_REGISTERS", gui->GetSourceFile());
		return 0;
	}
	int i = expressionRegisters.Num();
	registerIsTemporary[i] = true;
	i = expressionRegisters.Append(0);
	return i;
}

/*
================
idWindow::ExpressionOp
================
*/
wexpOp_t *idWindow::ExpressionOp() {
	if (ops.Num() == MAX_EXPRESSION_OPS) {
		common->Warning("expressionOp: gui %s hit MAX_EXPRESSION_OPS", gui->GetSourceFile());
		return &ops[0];
	}
	wexpOp_t wop;
	memset(&wop, 0, sizeof(wexpOp_t));
	int i = ops.Append(wop);
	return &ops[i];
}

/*
================
idWindow::EmitOp
================
*/

int idWindow::EmitOp(int a, int b, wexpOpType_t opType, wexpOp_t **opp) {
	wexpOp_t *op;
	/*
		// optimize away identity operations
		if ( opType == WOP_TYPE_ADD ) {
			if ( !registerIsTemporary[a] && shaderRegisters[a] == 0 ) {
				return b;
			}
			if ( !registerIsTemporary[b] && shaderRegisters[b] == 0 ) {
				return a;
			}
			if ( !registerIsTemporary[a] && !registerIsTemporary[b] ) {
				return ExpressionConstant( shaderRegisters[a] + shaderRegisters[b] );
			}
		}
		if ( opType == WOP_TYPE_MULTIPLY ) {
			if ( !registerIsTemporary[a] && shaderRegisters[a] == 1 ) {
				return b;
			}
			if ( !registerIsTemporary[a] && shaderRegisters[a] == 0 ) {
				return a;
			}
			if ( !registerIsTemporary[b] && shaderRegisters[b] == 1 ) {
				return a;
			}
			if ( !registerIsTemporary[b] && shaderRegisters[b] == 0 ) {
				return b;
			}
			if ( !registerIsTemporary[a] && !registerIsTemporary[b] ) {
				return ExpressionConstant( shaderRegisters[a] * shaderRegisters[b] );
			}
		}
	*/
	op = ExpressionOp();

	op->opType = opType;
	op->a = a;
	op->b = b;
	op->c = ExpressionTemporary();

	if (opp) {
		*opp = op;
	}
	return op->c;
}

/*
================
idWindow::ParseEmitOp
================
*/
int idWindow::ParseEmitOp(idTokenParser *src, int a, wexpOpType_t opType, int priority, wexpOp_t **opp) {
	int b = ParseExpressionPriority(src, priority);
	return EmitOp(a, b, opType, opp);
}


/*
================
idWindow::ParseTerm

Returns a register index
=================
*/
int idWindow::ParseTerm(idTokenParser *src, idWinVar *var, int component) {
	idToken token;
	int		a, b;

	src->ReadToken(&token);

	if (token == "(") {
		a = ParseExpression(src);
		src->ExpectTokenString(")");
		return a;
	}

	if (!token.Icmp("time")) {
		return WEXP_REG_TIME;
	}

	// parse negative numbers
	if (token == "-") {
		src->ReadToken(&token);
		if (token.type == TT_NUMBER || token == ".") {
			return ExpressionConstant(-(float)token.GetFloatValue());
		}
		src->Warning("Bad negative number '%s'", token.c_str());
		return 0;
	}

	if (token.type == TT_NUMBER || token == "." || token == "-") {
		return ExpressionConstant((float)token.GetFloatValue());
	}

	// see if it is a table name
	const idDeclTable *table = static_cast<const idDeclTable *>(declManager->FindType(DECL_TABLE, token.c_str(), false));
	if (table) {
		a = table->Index();
		// parse a table expression
		src->ExpectTokenString("[");
		b = ParseExpression(src);
		src->ExpectTokenString("]");
		return EmitOp(a, b, WOP_TYPE_TABLE);
	}

	if (var == nullptr) {
		var = GetWinVarByName(token, true);
	}
	if (var) {
		a = (int)var;
		//assert(dynamic_cast<idWinVec4*>(var));
		var->Init(token, this);
		b = component;
		if (dynamic_cast<idWinVec4*>(var)) {
			if (src->ReadToken(&token)) {
				if (token == "[") {
					b = ParseExpression(src);
					src->ExpectTokenString("]");
				}
				else {
					src->UnreadToken(&token);
				}
			}
			return EmitOp(a, b, WOP_TYPE_VAR);
		}
		else if (dynamic_cast<idWinFloat*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARF);
		}
		else if (dynamic_cast<idWinInt*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARI);
		}
		else if (dynamic_cast<idWinBool*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARB);
		}
		else if (dynamic_cast<idWinStr*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARS);
		}
		else {
			src->Warning("Var expression not vec4, float or int '%s'", token.c_str());
		}
		return 0;
	}
	else {
		// ugly but used for post parsing to fixup named vars
		char *p = new (TAG_OLD_UI) char[token.Length() + 1];
		strcpy(p, token);
		a = (int)p;
		b = -2;
		return EmitOp(a, b, WOP_TYPE_VAR);
	}

}

/*
=================
idWindow::ParseExpressionPriority

Returns a register index
=================
*/
#define	TOP_PRIORITY 4
int idWindow::ParseExpressionPriority(idTokenParser *src, int priority, idWinVar *var, int component) {
	idToken token;
	int		a;

	if (priority == 0) {
		return ParseTerm(src, var, component);
	}

	a = ParseExpressionPriority(src, priority - 1, var, component);

	if (!src->ReadToken(&token)) {
		// we won't get EOF in a real file, but we can
		// when parsing from generated strings
		return a;
	}

	if (priority == 1 && token == "*") {
		return ParseEmitOp(src, a, WOP_TYPE_MULTIPLY, priority);
	}
	if (priority == 1 && token == "/") {
		return ParseEmitOp(src, a, WOP_TYPE_DIVIDE, priority);
	}
	if (priority == 1 && token == "%") {	// implied truncate both to integer
		return ParseEmitOp(src, a, WOP_TYPE_MOD, priority);
	}
	if (priority == 2 && token == "+") {
		return ParseEmitOp(src, a, WOP_TYPE_ADD, priority);
	}
	if (priority == 2 && token == "-") {
		return ParseEmitOp(src, a, WOP_TYPE_SUBTRACT, priority);
	}
	if (priority == 3 && token == ">") {
		return ParseEmitOp(src, a, WOP_TYPE_GT, priority);
	}
	if (priority == 3 && token == ">=") {
		return ParseEmitOp(src, a, WOP_TYPE_GE, priority);
	}
	if (priority == 3 && token == "<") {
		return ParseEmitOp(src, a, WOP_TYPE_LT, priority);
	}
	if (priority == 3 && token == "<=") {
		return ParseEmitOp(src, a, WOP_TYPE_LE, priority);
	}
	if (priority == 3 && token == "==") {
		return ParseEmitOp(src, a, WOP_TYPE_EQ, priority);
	}
	if (priority == 3 && token == "!=") {
		return ParseEmitOp(src, a, WOP_TYPE_NE, priority);
	}
	if (priority == 4 && token == "&&") {
		return ParseEmitOp(src, a, WOP_TYPE_AND, priority);
	}
	if (priority == 4 && token == "||") {
		return ParseEmitOp(src, a, WOP_TYPE_OR, priority);
	}
	if (priority == 4 && token == "?") {
		wexpOp_t *oop = nullptr;
		int o = ParseEmitOp(src, a, WOP_TYPE_COND, priority, &oop);
		if (!src->ReadToken(&token)) {
			return o;
		}
		if (token == ":") {
			a = ParseExpressionPriority(src, priority - 1, var);
			oop->d = a;
		}
		return o;
	}

	// assume that anything else terminates the expression
	// not too robust error checking...

	src->UnreadToken(&token);

	return a;
}

/*
================
idWindow::ParseExpression

Returns a register index
================
*/
int idWindow::ParseExpression(idTokenParser *src, idWinVar *var, int component) {
	return ParseExpressionPriority(src, TOP_PRIORITY, var);
}

/*
================
idWindow::ParseBracedExpression
================
*/
void idWindow::ParseBracedExpression(idTokenParser *src) {
	src->ExpectTokenString("{");
	ParseExpression(src);
	src->ExpectTokenString("}");
}

/*
===============
idWindow::EvaluateRegisters

Parameters are taken from the localSpace and the renderView,
then all expressions are evaluated, leaving the shader registers
set to their apropriate values.
===============
*/
void idWindow::EvaluateRegisters(float *registers) {
	int		i, b;
	wexpOp_t	*op;
	idVec4 v;

	int erc = expressionRegisters.Num();
	int oc = ops.Num();
	// copy the constants
	for (i = WEXP_REG_NUM_PREDEFINED; i < erc; i++) {
		registers[i] = expressionRegisters[i];
	}

	// copy the local and global parameters
	registers[WEXP_REG_TIME] = gui->GetTime();

	for (i = 0; i < oc; i++) {
		op = &ops[i];
		if (op->b == -2) {
			continue;
		}
		switch (op->opType) {
		case WOP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case WOP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case WOP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case WOP_TYPE_DIVIDE:
			if (registers[op->b] == 0.0f) {
				common->Warning("Divide by zero in window '%s' in %s", GetName(), gui->GetSourceFile());
				registers[op->c] = registers[op->a];
			}
			else {
				registers[op->c] = registers[op->a] / registers[op->b];
			}
			break;
		case WOP_TYPE_MOD:
			b = (int)registers[op->b];
			b = b != 0 ? b : 1;
			registers[op->c] = (int)registers[op->a] % b;
			break;
		case WOP_TYPE_TABLE:
		{
			const idDeclTable *table = static_cast<const idDeclTable *>(declManager->DeclByIndex(DECL_TABLE, op->a));
			registers[op->c] = table->TableLookup(registers[op->b]);
		}
		break;
		case WOP_TYPE_GT:
			registers[op->c] = registers[op->a] > registers[op->b];
			break;
		case WOP_TYPE_GE:
			registers[op->c] = registers[op->a] >= registers[op->b];
			break;
		case WOP_TYPE_LT:
			registers[op->c] = registers[op->a] < registers[op->b];
			break;
		case WOP_TYPE_LE:
			registers[op->c] = registers[op->a] <= registers[op->b];
			break;
		case WOP_TYPE_EQ:
			registers[op->c] = registers[op->a] == registers[op->b];
			break;
		case WOP_TYPE_NE:
			registers[op->c] = registers[op->a] != registers[op->b];
			break;
		case WOP_TYPE_COND:
			registers[op->c] = (registers[op->a]) ? registers[op->b] : registers[op->d];
			break;
		case WOP_TYPE_AND:
			registers[op->c] = registers[op->a] && registers[op->b];
			break;
		case WOP_TYPE_OR:
			registers[op->c] = registers[op->a] || registers[op->b];
			break;
		case WOP_TYPE_VAR:
			if (!op->a) {
				registers[op->c] = 0.0f;
				break;
			}
			if (op->b >= 0 && registers[op->b] >= 0 && registers[op->b] < 4) {
				// grabs vector components
				idWinVec4 *var = (idWinVec4 *)(op->a);
				registers[op->c] = ((idVec4&)var)[registers[op->b]];
			}
			else {
				registers[op->c] = ((idWinVar*)(op->a))->x();
			}
			break;
		case WOP_TYPE_VARS:
			if (op->a) {
				idWinStr *var = (idWinStr*)(op->a);
				registers[op->c] = atof(var->c_str());
			}
			else {
				registers[op->c] = 0;
			}
			break;
		case WOP_TYPE_VARF:
			if (op->a) {
				idWinFloat *var = (idWinFloat*)(op->a);
				registers[op->c] = *var;
			}
			else {
				registers[op->c] = 0;
			}
			break;
		case WOP_TYPE_VARI:
			if (op->a) {
				idWinInt *var = (idWinInt*)(op->a);
				registers[op->c] = *var;
			}
			else {
				registers[op->c] = 0;
			}
			break;
		case WOP_TYPE_VARB:
			if (op->a) {
				idWinBool *var = (idWinBool*)(op->a);
				registers[op->c] = *var;
			}
			else {
				registers[op->c] = 0;
			}
			break;
		default:
			common->FatalError("R_EvaluateExpression: bad opcode");
		}
	}

}

/*
================
idWindow::ReadFromDemoFile
================
*/
void idWindow::ReadFromDemoFile(class idDemoFile *f, bool rebuild) {

	// should never hit unless we re-enable WRITE_GUIS
#ifndef WRITE_GUIS
	assert(false);
#else

	if (rebuild) {
		CommonInit();
	}

	f->SetLog(true, "window1");
	backGroundName = f->ReadHashString();
	f->SetLog(true, backGroundName);
	if (backGroundName[0]) {
		background = declManager->FindMaterial(backGroundName);
	}
	else {
		background = nullptr;
	}
	f->ReadUnsignedChar(cursor);
	f->ReadUnsignedInt(flags);
	f->ReadInt(timeLine);
	f->ReadInt(lastTimeRun);
	idRectangle rct = rect;
	f->ReadFloat(rct.x);
	f->ReadFloat(rct.y);
	f->ReadFloat(rct.w);
	f->ReadFloat(rct.h);
	f->ReadFloat(drawRect.x);
	f->ReadFloat(drawRect.y);
	f->ReadFloat(drawRect.w);
	f->ReadFloat(drawRect.h);
	f->ReadFloat(clientRect.x);
	f->ReadFloat(clientRect.y);
	f->ReadFloat(clientRect.w);
	f->ReadFloat(clientRect.h);
	f->ReadFloat(textRect.x);
	f->ReadFloat(textRect.y);
	f->ReadFloat(textRect.w);
	f->ReadFloat(textRect.h);
	f->ReadFloat(xOffset);
	f->ReadFloat(yOffset);
	int i, c;

	idStr work;
	if (rebuild) {
		f->SetLog(true, (work + "-scripts"));
		for (i = 0; i < SCRIPT_COUNT; i++) {
			bool b;
			f->ReadBool(b);
			if (b) {
				delete scripts[i];
				scripts[i] = new (TAG_OLD_UI) idGuiScriptList;
				scripts[i]->ReadFromDemoFile(f);
			}
		}

		f->SetLog(true, (work + "-timelines"));
		f->ReadInt(c);
		for (i = 0; i < c; i++) {
			idTimeLineEvent *tl = new (TAG_OLD_UI) idTimeLineEvent;
			f->ReadInt(tl->time);
			f->ReadBool(tl->pending);
			tl->event->ReadFromDemoFile(f);
			if (rebuild) {
				timeLineEvents.Append(tl);
			}
			else {
				assert(i < timeLineEvents.Num());
				timeLineEvents[i]->time = tl->time;
				timeLineEvents[i]->pending = tl->pending;
			}
		}
	}

	f->SetLog(true, (work + "-transitions"));
	f->ReadInt(c);
	for (i = 0; i < c; i++) {
		idTransitionData td;
		td.data = nullptr;
		f->ReadInt(td.offset);

		float startTime, accelTime, linearTime, decelTime;
		idVec4 startValue, endValue;
		f->ReadFloat(startTime);
		f->ReadFloat(accelTime);
		f->ReadFloat(linearTime);
		f->ReadFloat(decelTime);
		f->ReadVec4(startValue);
		f->ReadVec4(endValue);
		td.interp.Init(startTime, accelTime, decelTime, accelTime + linearTime + decelTime, startValue, endValue);

		// read this for correct data padding with the win32 savegames
		// the extrapolate is correctly initialized through the above Init call
		int extrapolationType;
		float duration;
		idVec4 baseSpeed, speed;
		float currentTime;
		idVec4 currentValue;
		f->ReadInt(extrapolationType);
		f->ReadFloat(startTime);
		f->ReadFloat(duration);
		f->ReadVec4(startValue);
		f->ReadVec4(baseSpeed);
		f->ReadVec4(speed);
		f->ReadFloat(currentTime);
		f->ReadVec4(currentValue);

		transitions.Append(td);
	}

	f->SetLog(true, (work + "-regstuff"));
	if (rebuild) {
		f->ReadInt(c);
		for (i = 0; i < c; i++) {
			wexpOp_t w;
			f->ReadInt((int&)w.opType);
			f->ReadInt(w.a);
			f->ReadInt(w.b);
			f->ReadInt(w.c);
			f->ReadInt(w.d);
			ops.Append(w);
		}

		f->ReadInt(c);
		for (i = 0; i < c; i++) {
			float ff;
			f->ReadFloat(ff);
			expressionRegisters.Append(ff);
		}

		regList.ReadFromDemoFile(f);

	}
	f->SetLog(true, (work + "-children"));
	f->ReadInt(c);
	for (i = 0; i < c; i++) {
		if (rebuild) {
			idWindow *win = new (TAG_OLD_UI) idWindow(dc, gui);
			win->ReadFromDemoFile(f);
			AddChild(win);
		}
		else {
			for (int j = 0; j < c; j++) {
				if (children[j]->childID == i) {
					children[j]->ReadFromDemoFile(f, rebuild);
					break;
				}
				else {
					continue;
				}
			}
		}
	}
#endif /* WRITE_GUIS */
}

/*
================
idWindow::WriteToDemoFile
================
*/
void idWindow::WriteToDemoFile(class idDemoFile *f) {
	// should never hit unless we re-enable WRITE_GUIS
#ifndef WRITE_GUIS
	assert(false);
#else

	f->SetLog(true, "window");
	f->WriteHashString(backGroundName);
	f->SetLog(true, backGroundName);
	f->WriteUnsignedChar(cursor);
	f->WriteUnsignedInt(flags);
	f->WriteInt(timeLine);
	f->WriteInt(lastTimeRun);
	idRectangle rct = rect;
	f->WriteFloat(rct.x);
	f->WriteFloat(rct.y);
	f->WriteFloat(rct.w);
	f->WriteFloat(rct.h);
	f->WriteFloat(drawRect.x);
	f->WriteFloat(drawRect.y);
	f->WriteFloat(drawRect.w);
	f->WriteFloat(drawRect.h);
	f->WriteFloat(clientRect.x);
	f->WriteFloat(clientRect.y);
	f->WriteFloat(clientRect.w);
	f->WriteFloat(clientRect.h);
	f->WriteFloat(textRect.x);
	f->WriteFloat(textRect.y);
	f->WriteFloat(textRect.w);
	f->WriteFloat(textRect.h);
	f->WriteFloat(xOffset);
	f->WriteFloat(yOffset);
	idStr work;
	f->SetLog(true, work);

	int i, c;

	f->SetLog(true, (work + "-transitions"));
	c = transitions.Num();
	f->WriteInt(c);
	for (i = 0; i < c; i++) {
		f->WriteInt(0);
		f->WriteInt(transitions[i].offset);

		f->WriteFloat(transitions[i].interp.GetStartTime());
		f->WriteFloat(transitions[i].interp.GetAccelTime());
		f->WriteFloat(transitions[i].interp.GetLinearTime());
		f->WriteFloat(transitions[i].interp.GetDecelTime());
		f->WriteVec4(transitions[i].interp.GetStartValue());
		f->WriteVec4(transitions[i].interp.GetEndValue());

		// write to keep win32 render demo format compatiblity - we don't actually read them back anymore
		f->WriteInt(transitions[i].interp.GetExtrapolate()->GetExtrapolationType());
		f->WriteFloat(transitions[i].interp.GetExtrapolate()->GetStartTime());
		f->WriteFloat(transitions[i].interp.GetExtrapolate()->GetDuration());
		f->WriteVec4(transitions[i].interp.GetExtrapolate()->GetStartValue());
		f->WriteVec4(transitions[i].interp.GetExtrapolate()->GetBaseSpeed());
		f->WriteVec4(transitions[i].interp.GetExtrapolate()->GetSpeed());
		f->WriteFloat(transitions[i].interp.GetExtrapolate()->GetCurrentTime());
		f->WriteVec4(transitions[i].interp.GetExtrapolate()->GetCurrentValue());
	}

	f->SetLog(true, (work + "-regstuff"));

	f->SetLog(true, (work + "-children"));
	c = children.Num();
	f->WriteInt(c);
	for (i = 0; i < c; i++) {
		for (int j = 0; j < c; j++) {
			if (children[j]->childID == i) {
				children[j]->WriteToDemoFile(f);
				break;
			}
			else {
				continue;
			}
		}
	}
#endif /* WRITE_GUIS */
}

/*
===============
idWindow::WriteString
===============
*/
void idWindow::WriteSaveGameString(const char *string, idFile *savefile) {
	int len = strlen(string);

	savefile->Write(&len, sizeof(len));
	savefile->Write(string, len);
}

/*
===============
idWindow::WriteSaveGameTransition
===============
*/
void idWindow::WriteSaveGameTransition(idTransitionData &trans, idFile *savefile) {
	drawWin_t dw, *fdw;
	idStr winName("");
	dw.simp = nullptr;
	dw.win = nullptr;
	int offset = gui->GetDesktop()->GetWinVarOffset(trans.data, &dw);
	if (dw.win || dw.simp) {
		winName = (dw.win) ? dw.win->GetName() : dw.simp->name.c_str();
	}
	fdw = gui->GetDesktop()->FindChildByName(winName);
	if (offset != -1 && fdw != nullptr && (fdw->win != nullptr || fdw->simp != nullptr)) {
		savefile->Write(&offset, sizeof(offset));
		WriteSaveGameString(winName, savefile);
		savefile->Write(&trans.interp, sizeof(trans.interp));
	}
	else {
		offset = -1;
		savefile->Write(&offset, sizeof(offset));
	}
}

/*
===============
idWindow::ReadSaveGameTransition
===============
*/
void idWindow::ReadSaveGameTransition(idTransitionData &trans, idFile *savefile) {
	int offset;

	savefile->Read(&offset, sizeof(offset));
	if (offset != -1) {
		idStr winName;
		ReadSaveGameString(winName, savefile);
		savefile->Read(&trans.interp, sizeof(trans.interp));
		trans.data = nullptr;
		trans.offset = offset;
		if (winName.Length()) {
			idWinStr *strVar = new (TAG_OLD_UI) idWinStr();
			strVar->Set(winName);
			trans.data = dynamic_cast<idWinVar*>(strVar);
		}
	}
}

/*
===============
idWindow::WriteToSaveGame
===============
*/
void idWindow::WriteToSaveGame(idFile *savefile) {
	int i;

	WriteSaveGameString(cmd, savefile);

	savefile->Write(&actualX, sizeof(actualX));
	savefile->Write(&actualY, sizeof(actualY));
	savefile->Write(&childID, sizeof(childID));
	savefile->Write(&flags, sizeof(flags));
	savefile->Write(&lastTimeRun, sizeof(lastTimeRun));
	savefile->Write(&drawRect, sizeof(drawRect));
	savefile->Write(&clientRect, sizeof(clientRect));
	savefile->Write(&origin, sizeof(origin));
	savefile->Write(&timeLine, sizeof(timeLine));
	savefile->Write(&xOffset, sizeof(xOffset));
	savefile->Write(&yOffset, sizeof(yOffset));
	savefile->Write(&cursor, sizeof(cursor));
	savefile->Write(&forceAspectWidth, sizeof(forceAspectWidth));
	savefile->Write(&forceAspectHeight, sizeof(forceAspectHeight));
	savefile->Write(&matScalex, sizeof(matScalex));
	savefile->Write(&matScaley, sizeof(matScaley));
	savefile->Write(&borderSize, sizeof(borderSize));
	savefile->Write(&textAlign, sizeof(textAlign));
	savefile->Write(&textAlignx, sizeof(textAlignx));
	savefile->Write(&textAligny, sizeof(textAligny));
	savefile->Write(&textShadow, sizeof(textShadow));
	savefile->Write(&shear, sizeof(shear));

	savefile->WriteString(font->GetName());

	WriteSaveGameString(name, savefile);
	WriteSaveGameString(comment, savefile);

	// WinVars
	noTime.WriteToSaveGame(savefile);
	visible.WriteToSaveGame(savefile);
	rect.WriteToSaveGame(savefile);
	backColor.WriteToSaveGame(savefile);
	matColor.WriteToSaveGame(savefile);
	foreColor.WriteToSaveGame(savefile);
	hoverColor.WriteToSaveGame(savefile);
	borderColor.WriteToSaveGame(savefile);
	textScale.WriteToSaveGame(savefile);
	noEvents.WriteToSaveGame(savefile);
	rotate.WriteToSaveGame(savefile);
	text.WriteToSaveGame(savefile);
	backGroundName.WriteToSaveGame(savefile);
	hideCursor.WriteToSaveGame(savefile);

	// Defined Vars
	for (i = 0; i < definedVars.Num(); i++) {
		definedVars[i]->WriteToSaveGame(savefile);
	}

	savefile->Write(&textRect, sizeof(textRect));

	// Window pointers saved as the child ID of the window
	int winID;

	winID = focusedChild ? focusedChild->childID : -1;
	savefile->Write(&winID, sizeof(winID));

	winID = captureChild ? captureChild->childID : -1;
	savefile->Write(&winID, sizeof(winID));

	winID = overChild ? overChild->childID : -1;
	savefile->Write(&winID, sizeof(winID));


	// Scripts
	for (i = 0; i < SCRIPT_COUNT; i++) {
		if (scripts[i]) {
			scripts[i]->WriteToSaveGame(savefile);
		}
	}

	// TimeLine Events
	for (i = 0; i < timeLineEvents.Num(); i++) {
		if (timeLineEvents[i]) {
			savefile->Write(&timeLineEvents[i]->pending, sizeof(timeLineEvents[i]->pending));
			savefile->Write(&timeLineEvents[i]->time, sizeof(timeLineEvents[i]->time));
			if (timeLineEvents[i]->event) {
				timeLineEvents[i]->event->WriteToSaveGame(savefile);
			}
		}
	}

	// Transitions
	int num = transitions.Num();

	savefile->Write(&num, sizeof(num));
	for (i = 0; i < transitions.Num(); i++) {
		WriteSaveGameTransition(transitions[i], savefile);
	}


	// Named Events
	for (i = 0; i < namedEvents.Num(); i++) {
		if (namedEvents[i]) {
			WriteSaveGameString(namedEvents[i]->mName, savefile);
			if (namedEvents[i]->mEvent) {
				namedEvents[i]->mEvent->WriteToSaveGame(savefile);
			}
		}
	}

	// regList
	regList.WriteToSaveGame(savefile);

	if (background) {
		savefile->WriteInt(background->GetCinematicStartTime());
	}
	else {
		savefile->WriteInt(-1);
	}

	// Save children
	for (i = 0; i < drawWindows.Num(); i++) {
		drawWin_t	window = drawWindows[i];

		if (window.simp) {
			window.simp->WriteToSaveGame(savefile);
		}
		else if (window.win) {
			window.win->WriteToSaveGame(savefile);
		}
	}
}

/*
===============
idWindow::ReadSaveGameString
===============
*/
void idWindow::ReadSaveGameString(idStr &string, idFile *savefile) {
	int len;

	savefile->Read(&len, sizeof(len));
	if (len < 0) {
		common->Warning("idWindow::ReadSaveGameString: invalid length");
	}

	string.Fill(' ', len);
	savefile->Read(&string[0], len);
}

/*
===============
idWindow::ReadFromSaveGame
===============
*/
void idWindow::ReadFromSaveGame(idFile *savefile) {
	int i;

	transitions.Clear();

	ReadSaveGameString(cmd, savefile);

	savefile->Read(&actualX, sizeof(actualX));
	savefile->Read(&actualY, sizeof(actualY));
	savefile->Read(&childID, sizeof(childID));
	savefile->Read(&flags, sizeof(flags));
	savefile->Read(&lastTimeRun, sizeof(lastTimeRun));
	savefile->Read(&drawRect, sizeof(drawRect));
	savefile->Read(&clientRect, sizeof(clientRect));
	savefile->Read(&origin, sizeof(origin));
	/*	if ( savefile->GetFileVersion() < BUILD_NUMBER_8TH_ANNIVERSARY_1 ) {
			unsigned char fontNum;
			savefile->Read( &fontNum, sizeof( fontNum ) );
			font = renderSystem->RegisterFont( "" );
		}*/
	savefile->Read(&timeLine, sizeof(timeLine));
	savefile->Read(&xOffset, sizeof(xOffset));
	savefile->Read(&yOffset, sizeof(yOffset));
	savefile->Read(&cursor, sizeof(cursor));
	savefile->Read(&forceAspectWidth, sizeof(forceAspectWidth));
	savefile->Read(&forceAspectHeight, sizeof(forceAspectHeight));
	savefile->Read(&matScalex, sizeof(matScalex));
	savefile->Read(&matScaley, sizeof(matScaley));
	savefile->Read(&borderSize, sizeof(borderSize));
	savefile->Read(&textAlign, sizeof(textAlign));
	savefile->Read(&textAlignx, sizeof(textAlignx));
	savefile->Read(&textAligny, sizeof(textAligny));
	savefile->Read(&textShadow, sizeof(textShadow));
	savefile->Read(&shear, sizeof(shear));

	//	if ( savefile->GetFileVersion() >= BUILD_NUMBER_8TH_ANNIVERSARY_1 ) {
	idStr fontName;
	savefile->ReadString(fontName);
	font = renderSystem->RegisterFont(fontName);
	//	} 

	ReadSaveGameString(name, savefile);
	ReadSaveGameString(comment, savefile);

	// WinVars
	noTime.ReadFromSaveGame(savefile);
	visible.ReadFromSaveGame(savefile);
	rect.ReadFromSaveGame(savefile);
	backColor.ReadFromSaveGame(savefile);
	matColor.ReadFromSaveGame(savefile);
	foreColor.ReadFromSaveGame(savefile);
	hoverColor.ReadFromSaveGame(savefile);
	borderColor.ReadFromSaveGame(savefile);
	textScale.ReadFromSaveGame(savefile);
	noEvents.ReadFromSaveGame(savefile);
	rotate.ReadFromSaveGame(savefile);
	text.ReadFromSaveGame(savefile);
	backGroundName.ReadFromSaveGame(savefile);
	hideCursor.ReadFromSaveGame(savefile);

	// Defined Vars
	for (i = 0; i < definedVars.Num(); i++) {
		definedVars[i]->ReadFromSaveGame(savefile);
	}

	savefile->Read(&textRect, sizeof(textRect));

	// Window pointers saved as the child ID of the window
	int winID = -1;

	savefile->Read(&winID, sizeof(winID));
	for (i = 0; i < children.Num(); i++) {
		if (children[i]->childID == winID) {
			focusedChild = children[i];
		}
	}
	savefile->Read(&winID, sizeof(winID));
	for (i = 0; i < children.Num(); i++) {
		if (children[i]->childID == winID) {
			captureChild = children[i];
		}
	}
	savefile->Read(&winID, sizeof(winID));
	for (i = 0; i < children.Num(); i++) {
		if (children[i]->childID == winID) {
			overChild = children[i];
		}
	}

	// Scripts
	for (i = 0; i < SCRIPT_COUNT; i++) {
		if (scripts[i]) {
			scripts[i]->ReadFromSaveGame(savefile);
		}
	}

	// TimeLine Events
	for (i = 0; i < timeLineEvents.Num(); i++) {
		if (timeLineEvents[i]) {
			savefile->Read(&timeLineEvents[i]->pending, sizeof(timeLineEvents[i]->pending));
			savefile->Read(&timeLineEvents[i]->time, sizeof(timeLineEvents[i]->time));
			if (timeLineEvents[i]->event) {
				timeLineEvents[i]->event->ReadFromSaveGame(savefile);
			}
		}
	}


	// Transitions
	int num;
	savefile->Read(&num, sizeof(num));
	for (i = 0; i < num; i++) {
		idTransitionData trans;
		trans.data = nullptr;
		ReadSaveGameTransition(trans, savefile);
		if (trans.data) {
			transitions.Append(trans);
		}
	}


	// Named Events
	for (i = 0; i < namedEvents.Num(); i++) {
		if (namedEvents[i]) {
			ReadSaveGameString(namedEvents[i]->mName, savefile);
			if (namedEvents[i]->mEvent) {
				namedEvents[i]->mEvent->ReadFromSaveGame(savefile);
			}
		}
	}

	// regList
	regList.ReadFromSaveGame(savefile);

	int cinematicStartTime = 0;
	savefile->ReadInt(cinematicStartTime);
	if (background) {
		background->ResetCinematicTime(cinematicStartTime);
	}

	// Read children
	for (i = 0; i < drawWindows.Num(); i++) {
		drawWin_t	window = drawWindows[i];

		if (window.simp) {
			window.simp->ReadFromSaveGame(savefile);
		}
		else if (window.win) {
			window.win->ReadFromSaveGame(savefile);
		}
	}

	if (flags & WIN_DESKTOP) {
		FixupTransitions();
	}
}

/*
===============
idWindow::NumTransitions
===============
*/
int idWindow::NumTransitions() {
	int c = transitions.Num();
	for (int i = 0; i < children.Num(); i++) {
		c += children[i]->NumTransitions();
	}
	return c;
}


/*
===============
idWindow::FixupTransitions
===============
*/
void idWindow::FixupTransitions() {
	int i, c = transitions.Num();
	for (i = 0; i < c; i++) {
		drawWin_t *dw = gui->GetDesktop()->FindChildByName(((idWinStr*)transitions[i].data)->c_str());
		delete transitions[i].data;
		transitions[i].data = nullptr;
		if (dw != nullptr && (dw->win != nullptr || dw->simp != nullptr)) {
			if (dw->win != nullptr) {
				if (transitions[i].offset == (int)&((idWindow *)0)->rect) {
					transitions[i].data = &dw->win->rect;
				}
				else if (transitions[i].offset == (int)&((idWindow *)0)->backColor) {
					transitions[i].data = &dw->win->backColor;
				}
				else if (transitions[i].offset == (int)&((idWindow *)0)->matColor) {
					transitions[i].data = &dw->win->matColor;
				}
				else if (transitions[i].offset == (int)&((idWindow *)0)->foreColor) {
					transitions[i].data = &dw->win->foreColor;
				}
				else if (transitions[i].offset == (int)&((idWindow *)0)->borderColor) {
					transitions[i].data = &dw->win->borderColor;
				}
				else if (transitions[i].offset == (int)&((idWindow *)0)->textScale) {
					transitions[i].data = &dw->win->textScale;
				}
				else if (transitions[i].offset == (int)&((idWindow *)0)->rotate) {
					transitions[i].data = &dw->win->rotate;
				}
			}
			else {
				if (transitions[i].offset == (int)&((idSimpleWindow *)0)->rect) {
					transitions[i].data = &dw->simp->rect;
				}
				else if (transitions[i].offset == (int)&((idSimpleWindow *)0)->backColor) {
					transitions[i].data = &dw->simp->backColor;
				}
				else if (transitions[i].offset == (int)&((idSimpleWindow *)0)->matColor) {
					transitions[i].data = &dw->simp->matColor;
				}
				else if (transitions[i].offset == (int)&((idSimpleWindow *)0)->foreColor) {
					transitions[i].data = &dw->simp->foreColor;
				}
				else if (transitions[i].offset == (int)&((idSimpleWindow *)0)->borderColor) {
					transitions[i].data = &dw->simp->borderColor;
				}
				else if (transitions[i].offset == (int)&((idSimpleWindow *)0)->textScale) {
					transitions[i].data = &dw->simp->textScale;
				}
				else if (transitions[i].offset == (int)&((idSimpleWindow *)0)->rotate) {
					transitions[i].data = &dw->simp->rotate;
				}
			}
		}
		if (transitions[i].data == nullptr) {
			transitions.RemoveIndex(i);
			i--;
			c--;
		}
	}
	for (c = 0; c < children.Num(); c++) {
		children[c]->FixupTransitions();
	}
}


/*
===============
idWindow::AddChild
===============
*/
void idWindow::AddChild(idWindow *win) {
	win->childID = children.Append(win);
}

/*
================
idWindow::FixupParms
================
*/
void idWindow::FixupParms() {
	int i;
	int c = children.Num();
	for (i = 0; i < c; i++) {
		children[i]->FixupParms();
	}
	for (i = 0; i < SCRIPT_COUNT; i++) {
		if (scripts[i]) {
			scripts[i]->FixupParms(this);
		}
	}

	c = timeLineEvents.Num();
	for (i = 0; i < c; i++) {
		timeLineEvents[i]->event->FixupParms(this);
	}

	c = namedEvents.Num();
	for (i = 0; i < c; i++) {
		namedEvents[i]->mEvent->FixupParms(this);
	}

	c = ops.Num();
	for (i = 0; i < c; i++) {
		if (ops[i].b == -2) {
			// need to fix this up
			const char *p = (const char*)(ops[i].a);
			idWinVar *var = GetWinVarByName(p, true);
			delete[]p;
			ops[i].a = (int)var;
			ops[i].b = -1;
		}
	}


	if (flags & WIN_DESKTOP) {
		CalcRects(0, 0);
	}

}

/*
================
idWindow::IsSimple
================
*/
bool idWindow::IsSimple() {

	if (ops.Num()) {
		return false;
	}
	if (flags & (WIN_HCENTER | WIN_VCENTER)) {
		return false;
	}
	if (children.Num() || drawWindows.Num()) {
		return false;
	}
	for (int i = 0; i < SCRIPT_COUNT; i++) {
		if (scripts[i]) {
			return false;
		}
	}
	if (timeLineEvents.Num()) {
		return false;
	}

	if (namedEvents.Num()) {
		return false;
	}

	return true;
}

/*
================
idWindow::ContainsStateVars
================
*/
bool idWindow::ContainsStateVars() {
	if (updateVars.Num()) {
		return true;
	}
	int c = children.Num();
	for (int i = 0; i < c; i++) {
		if (children[i]->ContainsStateVars()) {
			return true;
		}
	}
	return false;
}

/*
================
idWindow::Interactive
================
*/
bool idWindow::Interactive() {
	if (scripts[ON_ACTION]) {
		return true;
	}
	int c = children.Num();
	for (int i = 0; i < c; i++) {
		if (children[i]->Interactive()) {
			return true;
		}
	}
	return false;
}

/*
================
idWindow::SetChildWinVarVal
================
*/
void idWindow::SetChildWinVarVal(const char *name, const char *var, const char *val) {
	drawWin_t *dw = FindChildByName(name);
	idWinVar *wv = nullptr;
	if (dw != nullptr && dw->simp != nullptr) {
		wv = dw->simp->GetWinVarByName(var);
	}
	else if (dw != nullptr && dw->win != nullptr) {
		wv = dw->win->GetWinVarByName(var);
	}
	if (wv) {
		wv->Set(val);
		wv->SetEval(false);
	}
}


/*
================
idWindow::FindChildByPoint

Finds the window under the given point
================
*/
idWindow* idWindow::FindChildByPoint(float x, float y, idWindow** below) {
	int c = children.Num();

	// If we are looking for a window below this one then
	// the next window should be good, but this one wasnt it
	if (*below == this) {
		*below = nullptr;
		return nullptr;
	}

	if (!Contains(drawRect, x, y)) {
		return nullptr;
	}

	for (int i = c - 1; i >= 0; i--) {
		idWindow* found = children[i]->FindChildByPoint(x, y, below);
		if (found) {
			if (*below) {
				continue;
			}

			return found;
		}
	}

	return this;
}

/*
================
idWindow::FindChildByPoint
================
*/
idWindow* idWindow::FindChildByPoint(float x, float y, idWindow* below)
{
	return FindChildByPoint(x, y, &below);
}

/*
================
idWindow::GetChildCount

Returns the number of children
================
*/
int idWindow::GetChildCount()
{
	return drawWindows.Num();
}

/*
================
idWindow::GetChild

Returns the child window at the given index
================
*/
idWindow* idWindow::GetChild(int index)
{
	return drawWindows[index].win;
}

/*
================
idWindow::GetChildIndex

Returns the index of the given child window
================
*/
int idWindow::GetChildIndex(idWindow* window) {
	int find;
	for (find = 0; find < drawWindows.Num(); find++) {
		if (drawWindows[find].win == window) {
			return find;
		}
	}
	return -1;
}

/*
================
idWindow::RemoveChild

Removes the child from the list of children.   Note that the child window being
removed must still be deallocated by the caller
================
*/
void idWindow::RemoveChild(idWindow *win) {
	int find;

	// Remove the child window
	children.Remove(win);

	for (find = 0; find < drawWindows.Num(); find++)
	{
		if (drawWindows[find].win == win)
		{
			drawWindows.RemoveIndex(find);
			break;
		}
	}
}

/*
================
idWindow::InsertChild

Inserts the given window as a child into the given location in the zorder.
================
*/
bool idWindow::InsertChild(idWindow *win, idWindow* before)
{
	AddChild(win);

	win->parent = this;

	drawWin_t dwt;
	dwt.simp = nullptr;
	dwt.win = win;

	// If not inserting before anything then just add it at the end
	if (before) {
		int index;
		index = GetChildIndex(before);
		if (index != -1) {
			drawWindows.Insert(dwt, index);
			return true;
		}
	}

	drawWindows.Append(dwt);
	return true;
}

/*
================
idWindow::ScreenToClient
================
*/
void idWindow::ScreenToClient(idRectangle* r) {
	int		  x;
	int		  y;
	idWindow* p;

	for (p = this, x = 0, y = 0; p; p = p->parent) {
		x += p->rect.x();
		y += p->rect.y();
	}

	r->x -= x;
	r->y -= y;
}

/*
================
idWindow::ClientToScreen
================
*/
void idWindow::ClientToScreen(idRectangle* r) {
	int		  x;
	int		  y;
	idWindow* p;

	for (p = this, x = 0, y = 0; p; p = p->parent) {
		x += p->rect.x();
		y += p->rect.y();
	}

	r->x += x;
	r->y += y;
}

/*
================
idWindow::SetDefaults

Set the window do a default window with no text, no background and
default colors, etc..
================
*/
void idWindow::SetDefaults() {
	forceAspectWidth = 640.0f;
	forceAspectHeight = 480.0f;
	matScalex = 1;
	matScaley = 1;
	borderSize = 0;
	noTime = false;
	visible = true;
	textAlign = 0;
	textAlignx = 0;
	textAligny = 0;
	noEvents = false;
	rotate = 0;
	shear.Zero();
	textScale = 0.35f;
	backColor.Zero();
	foreColor = idVec4(1, 1, 1, 1);
	hoverColor = idVec4(1, 1, 1, 1);
	matColor = idVec4(1, 1, 1, 1);
	borderColor.Zero();
	text = "";

	background = nullptr;
	backGroundName = "";
}

/*
================
idWindow::UpdateFromDictionary

The editor only has a dictionary to work with so the easiest way to push the
values of the dictionary onto the window is for the window to interpret the
dictionary as if were a file being parsed.
================
*/
bool idWindow::UpdateFromDictionary(idDict& dict) {
	const idKeyValue*	kv;
	int					i;

	SetDefaults();

	// Clear all registers since they will get recreated
	regList.Reset();
	expressionRegisters.Clear();
	ops.Clear();

	for (i = 0; i < dict.GetNumKeyVals(); i++) {
		kv = dict.GetKeyVal(i);

		// Special case name
		if (!kv->GetKey().Icmp("name")) {
			name = kv->GetValue();
			continue;
		}

		idParser src(kv->GetValue().c_str(), kv->GetValue().Length(), "",
			LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT);
		idTokenParser src2;
		src2.LoadFromParser(src, "temp");
		src2.StartParsing("temp");
		if (!ParseInternalVar(kv->GetKey(), &src2)) {
			// Kill the old register since the parse reg entry will add a new one
			if (!ParseRegEntry(kv->GetKey(), &src2)) {
				continue;
			}
		}
	}

	EvalRegs(-1, true);

	SetupFromState();
	PostParse();

	return true;
}

#ifndef __WINDOW_H__
#define __WINDOW_H__
#include "..\Global.h"
#include "Rectangle.h"
#include "DeviceContext.h"
#include "RegExp.h"
#include "WinVar.h"
#include "GuiScript.h"
#include "SimpleWindow.h"

const int WIN_CHILD = 0x00000001;
const int WIN_CAPTION = 0x00000002;
const int WIN_BORDER = 0x00000004;
const int WIN_SIZABLE = 0x00000008;
const int WIN_MOVABLE = 0x00000010;
const int WIN_FOCUS = 0x00000020;
const int WIN_CAPTURE = 0x00000040;
const int WIN_HCENTER = 0x00000080;
const int WIN_VCENTER = 0x00000100;
const int WIN_MODAL = 0x00000200;
const int WIN_INTRANSITION = 0x00000400;
const int WIN_CANFOCUS = 0x00000800;
const int WIN_SELECTED = 0x00001000;
const int WIN_TRANSFORM = 0x00002000;
const int WIN_HOLDCAPTURE = 0x00004000;
const int WIN_NOWRAP = 0x00008000;
const int WIN_NOCLIP = 0x00010000;
const int WIN_INVERTRECT = 0x00020000;
const int WIN_NATURALMAT = 0x00040000;
const int WIN_NOCURSOR = 0x00080000;
const int WIN_MENUGUI = 0x00100000;
const int WIN_ACTIVE = 0x00200000;
const int WIN_SHOWCOORDS = 0x00400000;
const int WIN_SHOWTIME = 0x00800000;
const int WIN_WANTENTER = 0x01000000;

const int WIN_DESKTOP = 0x10000000;

const char CAPTION_HEIGHT[] = "16.0";
const char SCROLLER_SIZE[] = "16.0";
const int SCROLLBAR_SIZE = 16;

const int MAX_WINDOW_NAME = 32;
const int MAX_LIST_ITEMS = 1024;

const char DEFAULT_BACKCOLOR[] = "1 1 1 1";
const char DEFAULT_FORECOLOR[] = "0 0 0 1";
const char DEFAULT_BORDERCOLOR[] = "0 0 0 1";
const char DEFAULT_TEXTSCALE[] = "0.4";

typedef enum {
	WOP_TYPE_ADD,
	WOP_TYPE_SUBTRACT,
	WOP_TYPE_MULTIPLY,
	WOP_TYPE_DIVIDE,
	WOP_TYPE_MOD,
	WOP_TYPE_TABLE,
	WOP_TYPE_GT,
	WOP_TYPE_GE,
	WOP_TYPE_LT,
	WOP_TYPE_LE,
	WOP_TYPE_EQ,
	WOP_TYPE_NE,
	WOP_TYPE_AND,
	WOP_TYPE_OR,
	WOP_TYPE_VAR,
	WOP_TYPE_VARS,
	WOP_TYPE_VARF,
	WOP_TYPE_VARI,
	WOP_TYPE_VARB,
	WOP_TYPE_COND
} wexpOpType_t;

typedef enum {
	WEXP_REG_TIME,
	WEXP_REG_NUM_PREDEFINED
} wexpRegister_t;

typedef struct {
	wexpOpType_t opType;
	int	a, b, c, d;
} wexpOp_t;

struct RegEntry
{
	const char *name;
	Register::REGTYPE type;
	int index;
};

//class rvGEWindowWrapper;
class Window;

struct TimeLineEvent
{
	TimeLineEvent() { event = new GuiScriptList(); }
	~TimeLineEvent() { delete event; }
	int time;
	GuiScriptList *event;
	bool pending;
	size_t Size() { return sizeof(*this) + event->Size(); }
};

class rvNamedEvent
{
public:
	rvNamedEvent(const char *name_) { event = new GuiScriptList(); name = new string(name_); }
	~rvNamedEvent() { delete event; }
	size_t Size() { return sizeof(*this) + event->Size(); }
	string *name;
	GuiScriptList *event;
};

struct TransitionData
{
	WinVar *data;
	int	offset;
	//InterpolateAccelDecelLinear<vec4> interp;
};

class UserInterfaceLocal;
class Window {
public:
	Window(UserInterfaceLocal *gui);
	virtual ~Window();

	enum {
		ON_MOUSEENTER = 0,
		ON_MOUSEEXIT,
		ON_ACTION,
		ON_ACTIVATE,
		ON_DEACTIVATE,
		ON_ESC,
		ON_FRAME,
		ON_TRIGGER,
		ON_ACTIONRELEASE,
		ON_ENTER,
		ON_ENTERRELEASE,
		SCRIPT_COUNT
	};

	enum {
		ADJUST_MOVE = 0,
		ADJUST_TOP,
		ADJUST_RIGHT,
		ADJUST_BOTTOM,
		ADJUST_LEFT,
		ADJUST_TOPLEFT,
		ADJUST_BOTTOMRIGHT,
		ADJUST_TOPRIGHT,
		ADJUST_BOTTOMLEFT
	};

	static const char *ScriptNames[SCRIPT_COUNT];

	static const RegEntry RegisterVars[];
	static const int NumRegisterVars;

	Window *SetFocus(Window *w, bool scripts = true);

	Window *SetCapture(Window *w);
	void SetParent(Window *w);
	void SetFlag(unsigned int f);
	void ClearFlag(unsigned int f);
	unsigned GetFlags() { return _flags; };
	void Move(float x, float y);
	void BringToTop(Window *w);
	void Adjust(float xd, float yd);
	void SetAdjustMode(Window *child);
	void Size(float x, float y, float w, float h);
	void SetupFromState();
	void SetupBackground();
	drawWin_t *FindChildByName(const char *name);
	SimpleWindow *FindSimpleWinByName(const char *name);
	Window *GetParent() { return _parent; }
	UserInterfaceLocal *GetGui() { return _gui; };
	bool Contains(float x, float y);
	size_t Size();
	virtual size_t Allocated();
	string *GetStrPtrByName(const char *name);

	virtual WinVar *GetWinVarByName(const char *name, bool winLookup = false, drawWin_t **owner = nullptr);

	int GetWinVarOffset(WinVar *wv, drawWin_t *dw);
	float GetMaxCharHeight();
	float GetMaxCharWidth();
	void SetFont();
	void SetInitialState(const char *_name);
	void AddChild(Window *win);
	void DebugDraw(int time, float x, float y);
	void CalcClientRect(float xofs, float yofs);
	void CommonInit();
	void CleanUp();
	void DrawBorderAndCaption(const Rectangle &drawRect);
	void DrawCaption(int time, float x, float y);
	void SetupTransforms(float x, float y);
	bool Contains(const Rectangle &sr, float x, float y);
	const char *GetName() { return _name.c_str(); };

	virtual bool Parse(TokenParser *src, bool rebuild = true);
	virtual const char *HandleEvent(const Event_t *event, bool *updateVisuals);
	void CalcRects(float x, float y);
	virtual void Redraw(float x, float y, bool hud);

	virtual void ArchiveToDictionary(Dict *dict, bool useNames = true);
	virtual void InitFromDictionary(Dict *dict, bool byName = true);
	virtual void PostParse();
	virtual void Activate(bool activate, string &act);
	virtual void Trigger();
	virtual void GainFocus();
	virtual void LoseFocus();
	virtual void GainCapture();
	virtual void LoseCapture();
	virtual void Sized();
	virtual void Moved();
	virtual void Draw(int time, float x, float y);
	virtual void MouseExit();
	virtual void MouseEnter();
	virtual void DrawBackground(const Rectangle &drawRect);
	virtual Window * GetChildWithOnAction(float xd, float yd);
	virtual const char *RouteMouseCoords(float xd, float yd);
	virtual void SetBuddy(Window *buddy) {};
	virtual void HandleBuddyUpdate(Window *buddy) {};
	virtual void StateChanged(bool redraw);

	virtual void HasAction() {};
	virtual void HasScripts() {};

	void FixupParms();
	void GetScriptString(const char *name, string &out);
	void SetScriptParams();
	bool HasOps() { return (_ops.size() > 0); };
	float EvalRegs(int test = -1, bool force = false);
	void StartTransition();
	void AddTransition(WinVar *dest, vec4 from, vec4 to, int time, float accelTime, float decelTime);
	void ResetTime(int time);
	void ResetCinematics();

	int NumTransitions();

	bool ParseScript(TokenParser *src, GuiScriptList &list, int *timeParm = nullptr, bool allowIf = false);
	bool RunScript(int n);
	bool RunScriptList(GuiScriptList *src);
	void SetRegs(const char *key, const char *val);
	int ParseExpression(TokenParser *src, WinVar *var = nullptr, int component = 0);
	int ExpressionConstant(float f);
	RegisterList *RegList() { return &_regList; }
	void AddCommand(const char *cmd);
	void AddUpdateVar(WinVar *var);
	bool Interactive();
	bool ContainsStateVars();
	void SetChildWinVarVal(const char *name, const char *var, const char *val);
	Window *GetFocusedChild();
	Window *GetCaptureChild();
	const char *GetComment() { return _comment.c_str(); }
	void SetComment(const char * p) { _comment = p; }

	string _cmd;

	virtual void RunNamedEvent(const char *eventName);

	void AddDefinedVar(WinVar* var);

	Window *FindChildByPoint(float x, float y, Window *below = nullptr);
	int GetChildIndex(Window *window);
	int GetChildCount();
	Window *GetChild(int index);
	void RemoveChild(Window *win);
	bool InsertChild(Window *win, Window *before);

	void ScreenToClient(Rectangle *rect);
	void ClientToScreen(Rectangle *rect);

	bool UpdateFromDictionary(Dict &dict);

protected:
	//friend class rvGEWindowWrapper;

	Window *FindChildByPoint(float x, float y, Window **below);
	void SetDefaults();

	friend class SimpleWindow;
	friend class UserInterfaceLocal;
	bool IsSimple();
	void UpdateWinVars();
	void DisableRegister(const char *name);
	void Transition();
	void Time();
	bool RunTimeEvents(int time);
	void Dump();

	int ExpressionTemporary();
	wexpOp_t *ExpressionOp();
	int EmitOp(int a, int b, wexpOpType_t opType, wexpOp_t **opp = NULL);
	int ParseEmitOp(TokenParser *src, int a, wexpOpType_t opType, int priority, wexpOp_t **opp = NULL);
	int ParseTerm(TokenParser *src, WinVar *var = nullptr, int component = 0);
	int ParseExpressionPriority(TokenParser *src, int priority, WinVar *var = nullptr, int component = 0);
	void EvaluateRegisters(float *registers);
	void SaveExpressionParseState();
	void RestoreExpressionParseState();
	void ParseBracedExpression(TokenParser *src);
	bool ParseScriptEntry(const char *name, TokenParser *src);
	bool ParseRegEntry(const char *name, TokenParser *src);
	virtual bool ParseInternalVar(const char *name, TokenParser *src);
	void ParseString(TokenParser *src, string &out);
	void ParseVec4(TokenParser *src, vec4 &out);
	void ConvertRegEntry(const char *name, TokenParser *src, string &out, int tabs);

	float _actualX;					// physical coords
	float _actualY;					// ''
	int _childID;					// this childs id
	unsigned int _flags;             // visible, focus, mouseover, cursor, border, etc.. 
	int _lastTimeRun;				//
	Rectangle _drawRect;			// overall rect
	Rectangle _clientRect;			// client area
	vec2 _origin;

	int _timeLine;					// time stamp used for various fx
	float _xOffset;
	float _yOffset;
	float _forceAspectWidth;
	float _forceAspectHeight;
	float _matScalex;
	float _matScaley;
	float _borderSize;
	float _textAlignx;
	float _textAligny;
	string _name;
	string _comment;
	vec2 _shear;

	class Font *_font;
	signed char	_textShadow;
	unsigned char _cursor;
	signed char	_textAlign;

	WinBool	_noTime;
	WinBool	_visible;
	WinBool	_noEvents;
	WinRectangle _rect;				// overall rect
	WinVec4	_backColor;
	WinVec4	_matColor;
	WinVec4	_foreColor;
	WinVec4	_hoverColor;
	WinVec4	_borderColor;
	WinFloat _textScale;
	WinFloat _rotate;
	WinStr _text;
	WinBackground _backGroundName;

	vector<WinVar *> _definedVars;
	vector<WinVar *> _updateVars;

	Rectangle _textRect; // text extented rect
	const Material *_background; // background asset  

	Window *_parent;				// parent window
	vector<Window *> _children;		// child windows	
	vector<drawWin_t> _drawWindows;

	Window *_focusedChild;			// if a child window has the focus
	Window *_captureChild;			// if a child window has mouse capture
	Window *_overChild;				// if a child window has mouse capture
	bool _hover;

	UserInterfaceLocal *_gui;

	static int c_gui_debug; // CVAR
	static int c_gui_edit; // CVAR

	GuiScriptList *_scripts[SCRIPT_COUNT];
	bool *_saveTemps;

	vector<TimeLineEvent *> _timeLineEvents;
	vector<TransitionData> _transitions;

	static bool _registerIsTemporary[MAX_EXPRESSION_REGISTERS]; // statics to assist during parsing

	vector<wexpOp_t> _ops;			   			// evaluate to make expressionRegisters
	vector<float> _expressionRegisters;
	vector<wexpOp_t> *_saveOps;			   		// evaluate to make expressionRegisters
	vector<rvNamedEvent *> _namedEvents;		//  added named events
	vector<float> *_saveRegs;

	RegisterList _regList;

	WinBool _hideCursor;
};

__inline void Window::AddDefinedVar(WinVar *var)
{
	_definedVars.AddUnique(var);
}

#endif /* !__WINDOW_H__ */

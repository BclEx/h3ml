#ifndef __GUISCRIPT_H
#define __GUISCRIPT_H

#include "..\Global.h"
#include "Window.h"
#include "WinVar.h"

struct GSWinVar {
	GSWinVar() {
		var = nullptr;
		own = false;
	}
	WinVar *var;
	bool own;
};

class GuiScriptList;
class GuiScript {
	friend class GuiScriptList;
	friend class Window;
public:
	GuiScript();
	~GuiScript();

	bool Parse(TokenParser *src);
	void Execute(Window *win) {
		if (handler)
			handler(win, &_parms);
	}
	void FixupParms(Window *win);
	size_t Size() {
		int sz = sizeof(*this);
		for (int i = 0; i < _parms.size(); i++)
			sz += _parms[i].var->Size();
		return sz;
	}

protected:
	int _conditionReg;
	GuiScriptList *_ifList;
	GuiScriptList *_elseList;
	vector<GSWinVar> _parms;
	void(*handler)(Window *window, vector<GSWinVar> *src);
};

class GuiScriptList {
	vector<GuiScript *> _list;
public:
	GuiScriptList() { /*_list.SetGranularity(4);*/ };
	~GuiScriptList() { /*_list.DeleteContents(true);*/ };
	void Execute(Window *win);
	void Append(GuiScript *gs) {
		_list.push_back(gs);
	}
	size_t Size() {
		int sz = sizeof(*this);
		for (int i = 0; i < _list.size(); i++)
			sz += _list[i]->Size();
		return sz;
	}
	void FixupParms(Window *win);
};

#endif // __GUISCRIPT_H
